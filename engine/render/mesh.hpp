#pragma once

#include <print>
#include <vector>
#include <string>

#include "shader.hpp"

#define MAX_BONE_INFLUENCE 4

struct Vertex_t
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];
};

struct Texture_t
{
    unsigned int id;
    std::string type;
    std::string path;
};

class Mesh
{
public:
    std::vector<Vertex_t> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture_t> textures;

    Mesh(std::vector<Vertex_t> vertices, std::vector<unsigned int> indices, std::vector<Texture_t> textures);
    void Draw(Shader &shader) const;

private:
    unsigned int VAO, VBO, EBO;

    void setupMesh();
};