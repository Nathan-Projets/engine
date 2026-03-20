#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
layout(location = 5) in vec2 aTexCoords1;

out VS_OUT {
    mat3 TBN;
    vec3 FragPos;
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

uniform float use_tbn;

vec2 SelectTexCoords(int uvIndex) {
    if (uvIndex == 1) {
        return aTexCoords1;
    }
    return aTexCoords;
}

void main() {
    vec3 displacedPos = aPos;
    
    // Apply displacement if available
    if (uDisplacementCount > 0 && uDisplacementStrength > 0.0) {
        float heightSample = texture(uDisplacement, SelectTexCoords(uDisplacementUV)).r;
        displacedPos += aNormal * (heightSample - 0.5) * 2.0 * uDisplacementStrength;
    }
    
    mat3 normalMatrix = transpose(inverse(mat3(uModel)));

    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);

    T = normalize(T - dot(T, N) * N);
    B = cross(N, T);

    vec3 fragPos = vec3(uModel * vec4(displacedPos, 1.0));
    vs_out.FragPos = fragPos;
    vs_out.TexCoords0 = aTexCoords;
    vs_out.TexCoords1 = aTexCoords1;
    vs_out.use_tbn = use_tbn;
    vs_out.Normal = normalize(normalMatrix * aNormal);
    if (use_tbn > 0.0) {
        vs_out.TBN = mat3(T, B, N);
    }

    gl_Position = uProjection * uView * uModel * vec4(displacedPos, 1.0);
}
