#version 460 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D uDiffuse[16];
uniform int uDiffuseCount;

layout(std140, binding = 4) uniform MaterialDataBlock {
    vec3 uBaseColor;
    float uMetallic;
    vec3 uEmissive;
    float uRoughness;
    float uAlphaCutoff;
    uint uFeatures;
    vec2 _materialPad0;
};

void main() {
    vec3 baseColor = uBaseColor;

    if (uDiffuseCount > 0) {
        baseColor = texture(uDiffuse[0], TexCoords).rgb;
    }

    FragColor = vec4(baseColor + uEmissive, 1.0);
}
