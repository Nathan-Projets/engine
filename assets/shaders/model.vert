#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

out VS_OUT {
    mat3 TBN;
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    float use_tbn;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float use_tbn;

void main() {
    // normal matrix
    mat3 normalMatrix = transpose(inverse(mat3(model)));

    // world space positions
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);

    T = normalize(T - dot(T, N) * N);
    B = cross(N, T);

    vec3 fragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.FragPos = fragPos;
    vs_out.TexCoords = aTexCoords;
    vs_out.use_tbn = use_tbn;
    if (use_tbn > 0.0) {
        vs_out.TBN = mat3(T, B, N);
    } else {
        vs_out.Normal = mat3(transpose(inverse(model))) * aNormal;
    }

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}