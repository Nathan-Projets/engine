#version 460 core

in VS_OUT {
    mat3 TBN;
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords0;
    vec2 TexCoords1;
    float use_tbn;
} fs_in;

uniform sampler2D uAmbient[16];
uniform int uAmbientCount;
uniform int uAmbientUV;
uniform sampler2D uDiffuse[16];
uniform int uDiffuseCount;
uniform int uDiffuseUV;
uniform sampler2D uSpecular[16];
uniform int uSpecularCount;
uniform int uSpecularUV;
uniform sampler2D uNormal[16];
uniform int uNormalCount;
uniform int uNormalUV;

// Padding members keep this GLSL struct layout-compatible with the C++ LightData struct.
// In std140, vec3 values are aligned to 16 bytes, so we add explicit pad fields to avoid
// subtle CPU/GPU offset mismatches when uploading arrays of lights through the UBO.
struct GpuLight {
    vec3 position;
    float intensity;

    vec3 color;
    float constant;

    vec3 ambient;
    float linear;

    vec3 diffuse;
    float quadratic;

    vec3 specular;
    float spotInnerCutoff;

    vec3 direction;
    float spotOuterCutoff;

    uint type;
    uint flags;
    vec2 _pad0;
};

layout(std140, binding = 1) uniform CameraDataBlock {
    mat4 uView;
    mat4 uProjection;
    mat4 uViewProjection;
    mat4 uInvViewProjection;
    vec3 uCameraPosition;
    float uNearPlane;
    vec2 _cameraPad0;
    float uFarPlane;
};

layout(std140, binding = 3) uniform LightDataBlock {
    GpuLight uLights[128];
};

layout(std140, binding = 4) uniform MaterialDataBlock {
    vec3 uBaseColor;
    float uMetallic;
    vec3 uEmissive;
    float uRoughness;
    float uAlphaCutoff;
    uint uFeatures;
    float uDisplacementStrength;
    float _materialPad0;
};

out vec4 FragColor;

vec2 SelectTexCoords(int uvIndex) {
    if(uvIndex == 1) {
        return fs_in.TexCoords1;
    }

    return fs_in.TexCoords0;
}

void main() {
    int debugViewMode = int(uFeatures & 0xFu);

    vec3 diffuseColor = vec3(1.0);
    if(uDiffuseCount > 0) {
        diffuseColor = vec3(0.0);
        for(int i = 0; i < uDiffuseCount; i++) {
            diffuseColor += texture(uDiffuse[i], SelectTexCoords(uDiffuseUV)).rgb;
        }
        diffuseColor /= float(uDiffuseCount);
    } else {
        diffuseColor = uBaseColor;
    }

    float roughnessFromTexture = 1.0;
    float metallicFromTexture = 0.0;
    vec3 specularColor = vec3(1.0);
    if(uSpecularCount > 0) {
        vec2 roughnessMetallic = vec2(0.0);
        for(int i = 0; i < uSpecularCount; i++) {
            vec4 sampleValue = texture(uSpecular[i], SelectTexCoords(uSpecularUV));
            // GLTF metallic-roughness is typically packed as G=roughness, B=metallic.
            roughnessMetallic += vec2(sampleValue.g, sampleValue.b);
        }
        roughnessMetallic /= float(uSpecularCount);
        roughnessFromTexture = max(roughnessMetallic.x, 0.02);
        metallicFromTexture = clamp(roughnessMetallic.y, 0.0, 1.0);
        float metallic = clamp(max(uMetallic, metallicFromTexture), 0.0, 1.0);
        float specularStrength = mix(0.04, 1.0, metallic);
        specularColor = vec3(specularStrength);
    }

    float aoFactor = 1.0;
    if(uAmbientCount > 0) {
        aoFactor = 0.0;
        for(int i = 0; i < uAmbientCount; i++) {
            aoFactor += texture(uAmbient[i], SelectTexCoords(uAmbientUV)).r;
        }
        aoFactor /= float(uAmbientCount);
    }

    vec3 norm;
    if(uNormalCount > 0 && fs_in.use_tbn > 0.0) {
        vec3 tangentNormal = vec3(0.0);
        for(int i = 0; i < uNormalCount; i++) {
            tangentNormal += texture(uNormal[i], SelectTexCoords(uNormalUV)).rgb * 2.0 - 1.0;
        }
        tangentNormal = normalize(tangentNormal / float(uNormalCount));
        norm = normalize(fs_in.TBN * tangentNormal);
    } else {
        norm = normalize(fs_in.Normal);
    }

    vec3 viewDir = normalize(uCameraPosition - fs_in.FragPos);

    float roughness = clamp(uRoughness * roughnessFromTexture, 0.02, 1.0);
    float shininess = max(1.0, (2.0 / (roughness * roughness)) - 2.0);
    vec3 ambientSum = vec3(0.0);
    vec3 diffuseSum = vec3(0.0);
    vec3 specularSum = vec3(0.0);

    for(int i = 0; i < 128; ++i) {
        if(uLights[i].intensity <= 0.0) {
            continue;
        }

        vec3 lightDir;
        float attenuation = 1.0;
        // these lights are implement thanks to https://learnopengl.com/Lighting/Light-casters
        if(uLights[i].type == 1u) {
            // Directional light: direction is the world-space direction the light travels.
            // Negate it to get the direction FROM the surface TO the light source.
            lightDir = normalize(-uLights[i].direction);
        } else {
            // Point light: direction from fragment to light position.
            vec3 lightOffset = uLights[i].position - fs_in.FragPos;
            float distance = length(lightOffset);
            lightDir = normalize(lightOffset);

            float attenuationDenominator = uLights[i].constant +
                                           uLights[i].linear * distance +
                                           uLights[i].quadratic * (distance * distance);
            attenuation = 1.0 / max(attenuationDenominator, 0.0001);

            if(uLights[i].type == 2u) {
                // Spotlight cone intensity (smooth edge between outer and inner cutoff).
                float theta = dot(lightDir, normalize(-uLights[i].direction));
                float epsilon = max(uLights[i].spotInnerCutoff - uLights[i].spotOuterCutoff, 0.0001);
                float intensity = clamp((theta - uLights[i].spotOuterCutoff) / epsilon, 0.0, 1.0);
                attenuation *= intensity;
            }
        }
        vec3 lightColor = uLights[i].color * uLights[i].intensity * attenuation;

        ambientSum += uLights[i].ambient * diffuseColor * lightColor;

        float diff = max(dot(norm, lightDir), 0.0);
        diffuseSum += uLights[i].diffuse * diff * diffuseColor * lightColor;

        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
        specularSum += uLights[i].specular * spec * specularColor * lightColor;
    }

    vec3 finalColor = ambientSum + diffuseSum + specularSum + uEmissive;

    if(debugViewMode == 1) {
        FragColor = vec4(diffuseColor, 1.0);
        return;
    }
    if(debugViewMode == 2) {
        FragColor = vec4(norm * 0.5 + 0.5, 1.0);
        return;
    }
    if(debugViewMode == 3) {
        FragColor = vec4(vec3(roughness), 1.0);
        return;
    }
    if(debugViewMode == 4) {
        float metallic = clamp(max(uMetallic, metallicFromTexture), 0.0, 1.0);
        FragColor = vec4(vec3(metallic), 1.0);
        return;
    }
    if(debugViewMode == 5) {
        FragColor = vec4(vec3(aoFactor), 1.0);
        return;
    }

    FragColor = vec4(finalColor, 1.0);
}
