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
    float range;

    vec3 ambient;
    float _padAmbient;

    vec3 diffuse;
    float _padDiffuse;

    vec3 specular;
    float _padSpecular;

    vec3 direction;
    float spotFalloff;

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
    vec2 _materialPad0;
};

out vec4 FragColor;

vec2 SelectTexCoords(int uvIndex) {
    if (uvIndex == 1) {
        return fs_in.TexCoords1;
    }

    return fs_in.TexCoords0;
}

void main() {
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

    vec3 specularColor = vec3(1.0);
    if(uSpecularCount > 0) {
        specularColor = vec3(0.0);
        for(int i = 0; i < uSpecularCount; i++) {
            specularColor += texture(uSpecular[i], SelectTexCoords(uSpecularUV)).rgb;
        }
        specularColor /= float(uSpecularCount);
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

    float roughness = clamp(uRoughness, 0.02, 1.0);
    float shininess = max(1.0, (2.0 / (roughness * roughness)) - 2.0);
    vec3 ambientSum = vec3(0.0);
    vec3 diffuseSum = vec3(0.0);
    vec3 specularSum = vec3(0.0);

    for(int i = 0; i < 128; ++i) {
        if(uLights[i].intensity <= 0.0) {
            continue;
        }

        vec3 lightDir = normalize(uLights[i].position - fs_in.FragPos);
        vec3 lightColor = uLights[i].color * uLights[i].intensity;

        ambientSum += uLights[i].ambient * diffuseColor * lightColor;

        float diff = max(dot(norm, lightDir), 0.0);
        diffuseSum += uLights[i].diffuse * diff * diffuseColor * lightColor;

        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
        specularSum += uLights[i].specular * spec * specularColor * lightColor;
    }

    vec3 finalColor = ambientSum + diffuseSum + specularSum + uEmissive;
    FragColor = vec4(finalColor, 1.0);
}
