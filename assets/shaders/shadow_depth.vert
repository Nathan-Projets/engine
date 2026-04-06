#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 6) in uvec4 aBoneIndices;
layout(location = 7) in vec4 aBoneWeights;

uniform mat4 uLightSpaceMatrix;
uniform mat4 uModel;
uniform int uIsSkinned;
uniform int uBoneCount;
uniform mat4 uBoneMatrices[128];

mat4 ComputeSkinMatrix()
{
    if (uIsSkinned == 0 || uBoneCount <= 0)
        return mat4(1.0);

    mat4 skinMatrix = mat4(0.0);
    float totalWeight = 0.0;
    for (int influenceIndex = 0; influenceIndex < 4; ++influenceIndex)
    {
        float weight = aBoneWeights[influenceIndex];
        if (weight <= 0.0)
            continue;

        uint boneIndex = aBoneIndices[influenceIndex];
        if (boneIndex >= uint(uBoneCount))
            continue;

        skinMatrix += uBoneMatrices[boneIndex] * weight;
        totalWeight += weight;
    }

    if (totalWeight <= 0.0)
        return mat4(1.0);

    return skinMatrix;
}

void main()
{
    vec4 localPosition = ComputeSkinMatrix() * vec4(aPos, 1.0);
    gl_Position = uLightSpaceMatrix * uModel * localPosition;
}
