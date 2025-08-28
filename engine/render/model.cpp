#include "model.hpp"

Model::Model()
{
}

Model::Model(const char *path)
{
    loadModel(path);
}

Model::~Model()
{
}

void Model::Load(const char *path)
{
    loadModel(path);
}

void Model::Draw(Shader &shader, const glm::mat4 &modelTransform)
{
    DrawNode(rootNode, shader, modelTransform);
}

void Model::DrawNode(Node &node, Shader &shader, const glm::mat4 &parentTransform)
{
    glm::mat4 globalTransform = parentTransform * node.localTransform;

    shader.Upload("model", globalTransform);
    for (Mesh &mesh : node.meshes)
    {
        mesh.Draw(shader);
    }

    for (Node &child : node.children)
    {
        DrawNode(child, shader, globalTransform);
    }
}

void Model::loadModel(std::string path)
{
    Assimp::Importer import;
    const aiScene *scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::println("Error: could not import 3D model, reason: {}", import.GetErrorString());
        return;
    }
    directory = path.substr(0, path.find_last_of('/'));

    rootNode = processNode(scene->mRootNode, scene);
}

Node Model::processNode(aiNode *node, const aiScene *scene)
{
    Node newNode;
    newNode.name = node->mName.C_Str();

    // convert Assimp transform to glm
    aiMatrix4x4 transformation = node->mTransformation;
    newNode.localTransform = glm::transpose(glm::make_mat4(&transformation.a1));

    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
        newNode.meshes.push_back(processMesh(mesh, scene));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        newNode.children.push_back(processNode(node->mChildren[i], scene));
    }

    return newNode;
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene)
{
    std::vector<Vertex_t> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture_t> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex_t vertex;
        glm::vec3 vector;
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;
        if (mesh->HasNormals())
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        }
        if (mesh->mTextureCoords[0])
        {
            glm::vec2 vec;
            // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
            // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
            if (mesh->mTangents)
            {
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
            }
            if (mesh->mBitangents)
            {
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
        }
        else
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);

        vertices.push_back(vertex);
    }
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

    // diffuse maps
    std::vector<Texture_t> diffuseMaps = loadMaterialTextures(scene, material, aiTextureType_DIFFUSE, "diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    // specular maps
    std::vector<Texture_t> specularMaps = loadMaterialTextures(scene, material, aiTextureType_SPECULAR, "specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    // normal maps
    std::vector<Texture_t> normalMaps = loadMaterialTextures(scene, material, aiTextureType_HEIGHT, "normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

    return Mesh(vertices, indices, textures);
}

std::vector<Texture_t> Model::loadMaterialTextures(const aiScene *scene, aiMaterial *mat, aiTextureType type, std::string typeName)
{
    std::vector<Texture_t> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        bool skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++)
        {
            if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
            {
                textures.push_back(textures_loaded[j]);
                skip = true;
                break;
            }
        }
        // don't load already loaded texture
        if (!skip)
        {
            std::optional<unsigned int> textureID = str.C_Str()[0] != '*' ? TextureFromFile(str.C_Str(), this->directory, scene) : TextureFromEmbedded(str.C_Str(), this->directory, scene);
            if (textureID)
            {
                Texture_t texture;
                texture.id = *textureID;
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
    }
    return textures;
}

std::optional<unsigned int> Model::TextureFromFile(const char *path, const std::string &directory, const aiScene *scene, bool gamma)
{
    std::string filename = std::string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    stbi_set_flip_vertically_on_load(true);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);
    }
    else
    {
        std::println("Error: failed to load texture, param: {}", filename);
        stbi_image_free(data);
        return {};
    }

    return textureID;
}

std::optional<unsigned int> Model::TextureFromEmbedded(const char *path, const std::string &directory, const aiScene *scene, bool gamma)
{
    unsigned int textureID;
    int texIndex = atoi(path + 1);
    if (texIndex < scene->mNumTextures)
    {
        aiTexture *texture = scene->mTextures[texIndex];
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        if (texture->mHeight == 0)
        {
            // compressed (like PNG/JPG)
            stbi_set_flip_vertically_on_load(false);

            int width, height, nrComponents;
            unsigned char *imageData = stbi_load_from_memory(reinterpret_cast<unsigned char *>(texture->pcData), texture->mWidth, &width, &height, &nrComponents, 0);

            if (imageData)
            {
                GLenum format;
                if (nrComponents == 1)
                    format = GL_RED;
                else if (nrComponents == 3)
                    format = GL_RGB;
                else if (nrComponents == 4)
                    format = GL_RGBA;

                glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, imageData);
                glGenerateMipmap(GL_TEXTURE_2D);
                stbi_image_free(imageData);
            }
        }
        else
        {
            // uncompressed RGBA data
            GLenum format = GL_RGBA;
            glTexImage2D(GL_TEXTURE_2D, 0, format, texture->mWidth, texture->mHeight, 0, format, GL_UNSIGNED_BYTE, texture->pcData);
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);
        return textureID;
    }

    return {};
}
