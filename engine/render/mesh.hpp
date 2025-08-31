#pragma once

#include <print>
#include <vector>
#include <string>

#include "shader.hpp"

#define MAX_BONE_INFLUENCE 4

struct Vertex
{
    glm::vec3 position = {};
    glm::vec3 normal = {};
    glm::vec2 tcoords = {};
    glm::vec3 tangent = {};
    glm::vec3 bitangent = {};
};

enum class TextureType
{
    UNKNOWN,
    AMBIENT,
    DIFFUSE,
    NORMAL,
    SPECULAR,
};

struct Texture
{
    unsigned int id = 0; // invalid buffer id by default
    TextureType type = TextureType::UNKNOWN;
    std::string path = "";
};

struct Material
{
    glm::vec3 ambient_color = glm::vec3(-1.0f);
    glm::vec3 diffuse_color = glm::vec3(-1.0f);
    glm::vec3 emissive_color = glm::vec3(-1.0f);
    glm::vec3 specular_color = glm::vec3(-1.0f);
    float specular_exponent = -1.0f;
    float optic_density = -1.0f;
    float alpha = -1.0f;

    Texture texture_ambiant;
    Texture texture_diffuse;
    Texture texture_specular;
    Texture texture_normal;
    Texture texture_disp;
    Texture texture_stencil;

    std::string name = "";
};

class Mesh
{
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Material> materials;
    std::string name;

    Mesh();
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, bool computeTangents = false);

    void Draw(Shader &shader) const;
    void AddMaterials(const std::vector<Material> &iMaterials);
    void AddMaterial(const Material &iMaterial);
    void SetupMesh(bool computeTangents = false);
    void SetupTangents();

private:
    unsigned int VAO, VBO, EBO;
    bool computedTangents;
};