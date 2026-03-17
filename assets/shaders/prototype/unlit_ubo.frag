#version 460 core

in vec2 TexCoords;
out vec4 FragColor;

struct Material {
    sampler2D ambient[16];
    sampler2D diffuse[16];
    sampler2D specular[16];
    sampler2D normal[16];
    int ambientCount;
    int diffuseCount;
    int specularCount;
    int normalCount;
    float shininess;
};

uniform Material material;
uniform vec3 color;

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

    if (material.diffuseCount > 0) {
        baseColor = texture(material.diffuse[0], TexCoords).rgb;
    }

    if (length(color) > 0.0) {
        baseColor *= color;
    }

    FragColor = vec4(baseColor + uEmissive, 1.0);
}
