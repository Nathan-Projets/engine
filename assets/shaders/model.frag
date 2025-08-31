#version 460 core

in VS_OUT {
    mat3 TBN;
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    float use_tbn;
} fs_in;

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

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Light light;

uniform vec3 viewPos;

out vec4 FragColor;

void main() {

    // ----------------
    // Diffuse color
    // ----------------
    vec3 diffuseColor = vec3(1.0); // fallback white
    if(material.diffuseCount > 0) {
        diffuseColor = vec3(0.0);
        for(int i = 0; i < material.diffuseCount; i++) {
            diffuseColor += texture(material.diffuse[i], fs_in.TexCoords).rgb;
        }
        diffuseColor /= float(material.diffuseCount);
    }

    // ----------------
    // Specular color
    // ----------------
    vec3 specularColor = vec3(1.0); // fallback white (so specular works without a map)
    if(material.specularCount > 0) {
        specularColor = vec3(0.0);
        for(int i = 0; i < material.specularCount; i++) {
            specularColor += texture(material.specular[i], fs_in.TexCoords).rgb;
        }
        specularColor /= float(material.specularCount);
    }

    // ----------------
    // Normals
    // ----------------
    vec3 norm;
    if(material.normalCount > 0 && fs_in.use_tbn > 0.0) {
        vec3 tangentNormal = vec3(0.0);
        for(int i = 0; i < material.normalCount; i++) {
            tangentNormal += texture(material.normal[i], fs_in.TexCoords).rgb * 2.0 - 1.0;
        }
        tangentNormal = normalize(tangentNormal / float(material.normalCount));
        norm = normalize(fs_in.TBN * tangentNormal);
    } else {
        // fallback: use mesh normal (Z axis of TBN basis is usually the interpolated normal)
        norm = normalize(fs_in.Normal);
    }

    // ----------------
    // Lighting
    // ----------------

    // lighting directions (world space)
    vec3 lightDir = normalize(light.position - fs_in.FragPos);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);

    // ambient
    vec3 ambient = light.ambient * diffuseColor;

    // diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * diffuseColor;

    // specular
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * specularColor;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}