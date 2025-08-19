#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

out VS_OUT {
    mat3 TBN;
    vec3 FragPos;
    vec2 TexCoords;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    // normal matrix
    mat3 normalMatrix = transpose(inverse(mat3(model)));

    // world space positions
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);

    vec3 fragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.FragPos = fragPos;
    vs_out.TexCoords = aTexCoords;
    vs_out.TBN = transpose(mat3(T, B, N));

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}