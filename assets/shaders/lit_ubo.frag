#version 460 core

in VS_OUT {
    mat3 TBN;
    vec3 FragPos;
    vec4 FragPosLightSpace;
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
uniform sampler2D uEmissiveTex[16];
uniform int uEmissiveTexCount;
uniform int uEmissiveUV;
uniform sampler2D uShadowMap;
uniform int uShadowEnabled;
uniform samplerCube uPointShadowMaps[4];
uniform int uPointShadowEnabled;
uniform int uPointShadowCount;
uniform vec3 uPointShadowLightPositions[4];
uniform float uPointShadowFarPlanes[4];

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

vec3 ApplyGammaCorrection(vec3 color) {
    return pow(max(color, vec3(0.0)), vec3(1.0 / 1.3));
}

float ComputeAttenuation(vec3 fragPos, vec3 lightPos, float constant, float linear, float quadratic) {
    float distance = length(lightPos - fragPos);
    float attenuationDenominator = constant + linear * distance + quadratic * (distance * distance);
    return 1.0 / max(attenuationDenominator, 0.0001);
}

// ref. https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
float CalculateDirectionalShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / max(fragPosLightSpace.w, 0.0001);
    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
    float ndotl = max(dot(normalize(normal), normalize(lightDir)), 0.0);
    if(ndotl <= 0.0) {
        return 0.0;
    }

    float slopeBias = 0.0025 * (1.0 - ndotl);
    float normalBias = 0.00035;
    float texelBias = max(texelSize.x, texelSize.y) * 0.75;
    float bias = max(slopeBias, normalBias) + texelBias;

    float shadow = 0.0;
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
        }
    }

    return shadow / 9.0;
}

float SamplePointShadowDepth(int shadowIndex, vec3 direction) {
    if(shadowIndex == 0) {
        return texture(uPointShadowMaps[0], direction).r;
    }
    if(shadowIndex == 1) {
        return texture(uPointShadowMaps[1], direction).r;
    }
    if(shadowIndex == 2) {
        return texture(uPointShadowMaps[2], direction).r;
    }
    if(shadowIndex == 3) {
        return texture(uPointShadowMaps[3], direction).r;
    }

    return 1.0;
}

// ref. https://learnopengl.com/Advanced-Lighting/Shadows/Point-Shadows
float CalculatePointShadow(int shadowIndex, vec3 fragPos, vec3 normal, vec3 lightDir, vec3 lightPos, float farPlane) {
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    if(currentDepth >= farPlane) {
        return 0.0;
    }

    float ndotl = max(dot(normalize(normal), normalize(lightDir)), 0.0);
    if(ndotl <= 0.0) {
        return 0.0;
    }

    float bias = max(0.03 * (1.0 - ndotl), 0.003);

    vec3 sampleOffsetDirections[20] = vec3[](vec3(1.0, 1.0, 1.0), vec3(1.0, -1.0, 1.0), vec3(-1.0, -1.0, 1.0), vec3(-1.0, 1.0, 1.0), vec3(1.0, 1.0, -1.0), vec3(1.0, -1.0, -1.0), vec3(-1.0, -1.0, -1.0), vec3(-1.0, 1.0, -1.0), vec3(1.0, 1.0, 0.0), vec3(1.0, -1.0, 0.0), vec3(-1.0, -1.0, 0.0), vec3(-1.0, 1.0, 0.0), vec3(1.0, 0.0, 1.0), vec3(-1.0, 0.0, 1.0), vec3(1.0, 0.0, -1.0), vec3(-1.0, 0.0, -1.0), vec3(0.0, 1.0, 1.0), vec3(0.0, -1.0, 1.0), vec3(0.0, -1.0, -1.0), vec3(0.0, 1.0, -1.0));

    float viewDistance = length(uCameraPosition - fragPos);
    float diskRadius = (1.0 + (viewDistance / max(farPlane, 0.0001))) / 25.0;

    float shadow = 0.0;
    for(int i = 0; i < 20; ++i) {
        float closestDepth = SamplePointShadowDepth(shadowIndex, fragToLight + sampleOffsetDirections[i] * diskRadius);
        if(closestDepth >= 0.9999) {
            continue;
        }
        closestDepth *= farPlane;
        shadow += (currentDepth - bias > closestDepth) ? 1.0 : 0.0;
    }

    return shadow / 20.0;
}

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

    vec3 emissiveColor = uEmissive;
    if(uEmissiveTexCount > 0) {
        vec3 emissiveTextureColor = vec3(0.0);
        for(int i = 0; i < uEmissiveTexCount; i++) {
            emissiveTextureColor += texture(uEmissiveTex[i], SelectTexCoords(uEmissiveUV)).rgb;
        }
        emissiveTextureColor /= float(uEmissiveTexCount);

        vec3 emissiveFactor = emissiveColor;
        if(length(emissiveFactor) <= 0.0001) {
            emissiveFactor = vec3(1.0);
        }
        emissiveColor = emissiveFactor * emissiveTextureColor;
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
        float spotFactor = 1.0;
        // these lights are implement thanks to https://learnopengl.com/Lighting/Light-casters
        if(uLights[i].type == 1u) {
            // Directional light: direction is the world-space direction the light travels.
            // Negate it to get the direction FROM the surface TO the light source.
            lightDir = normalize(-uLights[i].direction);
        } else {
            // Point light: direction from fragment to light position.
            vec3 lightOffset = uLights[i].position - fs_in.FragPos;
            lightDir = normalize(lightOffset);

            attenuation = ComputeAttenuation(fs_in.FragPos, uLights[i].position, uLights[i].constant, uLights[i].linear, uLights[i].quadratic);

            if(uLights[i].type == 2u) {
                // Spotlight cone intensity (smooth edge between outer and inner cutoff).
                float theta = dot(lightDir, normalize(-uLights[i].direction));
                float epsilon = max(uLights[i].spotInnerCutoff - uLights[i].spotOuterCutoff, 0.0001);
                spotFactor = clamp((theta - uLights[i].spotOuterCutoff) / epsilon, 0.0, 1.0);
            }
        }
        vec3 lightColor = uLights[i].color * uLights[i].intensity;
        vec3 attenuatedLightColor = lightColor * attenuation * spotFactor;
        float shadow = 0.0;
        if(uShadowEnabled > 0 && uLights[i].type == 1u && (uLights[i].flags & 1u) != 0u) {
            shadow = CalculateDirectionalShadow(fs_in.FragPosLightSpace, norm, lightDir);
        } else if(uPointShadowEnabled > 0 && uLights[i].type == 0u && uPointShadowCount > 0 && (uLights[i].flags & 1u) != 0u) {
            int shadowIndex = -1;
            for(int pointIndex = 0; pointIndex < uPointShadowCount; ++pointIndex) {
                if(distance(uPointShadowLightPositions[pointIndex], uLights[i].position) < 0.05) {
                    shadowIndex = pointIndex;
                    break;
                }
            }

            if(shadowIndex >= 0) {
                shadow = CalculatePointShadow(shadowIndex, fs_in.FragPos, norm, lightDir, uLights[i].position, uPointShadowFarPlanes[shadowIndex]);
            }
        }

        ambientSum += uLights[i].ambient * diffuseColor * lightColor;

        float diff = max(dot(norm, lightDir), 0.0);
        diffuseSum += uLights[i].diffuse * diff * diffuseColor * attenuatedLightColor * (1.0 - shadow);

        vec3 halfwayDir = normalize(lightDir + viewDir); // blinn-phong
        float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
        specularSum += uLights[i].specular * spec * specularColor * attenuatedLightColor * (1.0 - shadow);
    }

    vec3 finalColor = ambientSum + diffuseSum + specularSum + emissiveColor;

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

    FragColor = vec4(ApplyGammaCorrection(finalColor), 1.0);
}
