#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
layout(location = 5) in vec2 aTexCoords1;
layout(location = 6) in uvec4 aBoneIndices;
layout(location = 7) in vec4 aBoneWeights;

out VS_OUT {
    mat3 TBN;
    vec3 FragPos;
    vec4 FragPosLightSpace;
    vec3 Normal;
    vec2 TexCoords0;
    vec2 TexCoords1;
    float use_tbn;
} vs_out;

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

layout(std140, binding = 2) uniform ObjectDataBlock {
    mat4 uModel;
    mat4 uNormalMatrix;
    uint uObjectId;
    uint uMaterialIndex;
    vec2 _objectPad0;
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

uniform sampler2D uDisplacement;
uniform int uDisplacementCount;
uniform int uDisplacementUV;
uniform int uIsSkinned;
uniform int uBoneCount;
uniform mat4 uBoneMatrices[128];

uniform float use_tbn;
uniform mat4 uLightSpaceMatrix;

vec2 SelectTexCoords(int uvIndex) {
    if (uvIndex == 1) {
        return aTexCoords1;
    }
    return aTexCoords;
}

mat4 ComputeSkinMatrix() {
    if (uIsSkinned == 0 || uBoneCount <= 0) {
        return mat4(1.0);
    }

    mat4 skinMatrix = mat4(0.0);
    float totalWeight = 0.0;

    for (int influenceIndex = 0; influenceIndex < 4; ++influenceIndex) {
        float weight = aBoneWeights[influenceIndex];
        if (weight <= 0.0) {
            continue;
        }

        uint boneIndex = aBoneIndices[influenceIndex];
        if (boneIndex >= uint(uBoneCount)) {
            continue;
        }

        skinMatrix += uBoneMatrices[boneIndex] * weight;
        totalWeight += weight;
    }

    if (totalWeight <= 0.0) {
        return mat4(1.0);
    }

    return skinMatrix;
}

void main() {
    mat4 skinMatrix = ComputeSkinMatrix();
    vec4 localPosition = skinMatrix * vec4(aPos, 1.0);
    vec3 localNormal = normalize(mat3(skinMatrix) * aNormal);
    vec3 localTangent = normalize(mat3(skinMatrix) * aTangent);
    vec3 localBitangent = normalize(mat3(skinMatrix) * aBitangent);

    vec3 displacedPos = localPosition.xyz;
    
    // Apply displacement if available
    if (uDisplacementCount > 0 && uDisplacementStrength > 0.0) {
        float heightSample = texture(uDisplacement, SelectTexCoords(uDisplacementUV)).r;
        displacedPos += localNormal * (heightSample - 0.5) * 2.0 * uDisplacementStrength;
    }
    
    mat3 normalMatrix = transpose(inverse(mat3(uModel)));

    vec3 T = normalize(normalMatrix * localTangent);
    vec3 B = normalize(normalMatrix * localBitangent);
    vec3 N = normalize(normalMatrix * localNormal);

    T = normalize(T - dot(T, N) * N);
    B = cross(N, T);

    vec3 fragPos = vec3(uModel * vec4(displacedPos, 1.0));
    vs_out.FragPos = fragPos;
    vs_out.FragPosLightSpace = uLightSpaceMatrix * vec4(fragPos, 1.0);
    vs_out.TexCoords0 = aTexCoords;
    vs_out.TexCoords1 = aTexCoords1;
    vs_out.use_tbn = use_tbn;
    vs_out.Normal = normalize(normalMatrix * localNormal);
    if (use_tbn > 0.0) {
        vs_out.TBN = mat3(T, B, N);
    }

    gl_Position = uProjection * uView * uModel * vec4(displacedPos, 1.0);
}
