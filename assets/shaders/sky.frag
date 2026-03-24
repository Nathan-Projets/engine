#version 460 core

in vec3 TexCoords;

out vec4 FragColor;

uniform samplerCube uSkybox;

layout(std140, binding = 4) uniform MaterialDataBlock {
    vec3 uBaseColor;
    float uMetallic;
    vec3 uEmissive;
    float uRoughness;
    float uAlphaCutoff;
    uint uFeatures;
    vec2 _materialPad0;
};

vec3 ApplyGammaCorrection(vec3 color) {
    return pow(max(color, vec3(0.0)), vec3(1.0 / 1.3));
}

void main() {
    vec3 texDir = normalize(TexCoords);
    vec3 skyColor = texture(uSkybox, texDir).rgb;

    skyColor = skyColor * (1.0 + uEmissive * 0.5);

    FragColor = vec4(ApplyGammaCorrection(skyColor), 1.0);
}
