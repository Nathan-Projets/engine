#pragma once

#include <print>
#include <string>
#include <vector>
#include <optional>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <stb_image.h>

#include "shader.hpp"
#include "mesh.hpp"

struct Node
{
    std::string name;
    glm::mat4 localTransform;
    std::vector<Mesh> meshes;
    std::vector<Node> children;
};

class Model
{
public:
    Model();
    Model(const char *path);
    ~Model();

    const std::vector<Mesh> &GetMeshes() { return meshes; }
    const Mesh &GetMesh() { return meshes[0]; }

    void Load(const char *path);
    void Draw(Shader &shader, const glm::mat4 &modelTransform);
    void DrawNode(Node &node, Shader &shader, const glm::mat4 &parentTransform);

private:
    // model data
    std::vector<Texture_t> textures_loaded;
    std::vector<Mesh> meshes;
    std::string directory;
    Node rootNode;

    void loadModel(std::string path);
    Node processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    std::vector<Texture_t> loadMaterialTextures(const aiScene *scene, aiMaterial *mat, aiTextureType type, std::string typeName);
    std::optional<unsigned int> TextureFromFile(const char *path, const std::string &directory, const aiScene *scene, bool gamma = false);
    std::optional<unsigned int> TextureFromEmbedded(const char *path, const std::string &directory, const aiScene *scene, bool gamma = false);
};