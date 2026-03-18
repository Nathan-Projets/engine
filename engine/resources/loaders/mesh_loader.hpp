#pragma once

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <memory>
#include <string>
#include <vector>

#include "../units/mesh.hpp"
#include "../resource_loader.hpp"

namespace resources
{
    namespace detail
    {
        inline glm::mat4 ToGlmMatrix(const aiMatrix4x4 &matrix)
        {
            glm::mat4 result(1.0f);
            result[0][0] = matrix.a1;
            result[1][0] = matrix.a2;
            result[2][0] = matrix.a3;
            result[3][0] = matrix.a4;
            result[0][1] = matrix.b1;
            result[1][1] = matrix.b2;
            result[2][1] = matrix.b3;
            result[3][1] = matrix.b4;
            result[0][2] = matrix.c1;
            result[1][2] = matrix.c2;
            result[2][2] = matrix.c3;
            result[3][2] = matrix.c4;
            result[0][3] = matrix.d1;
            result[1][3] = matrix.d2;
            result[2][3] = matrix.d3;
            result[3][3] = matrix.d4;
            return result;
        }

        inline void AppendNodePrimitives(
            const aiNode *node,
            const glm::mat4 &parentTransform,
            const std::vector<MeshPrimitive> &meshTemplates,
            const std::vector<uint32_t> &meshMaterialIndices,
            std::vector<MeshPrimitiveInstance> &outPrimitiveInstances)
        {
            if (!node)
            {
                return;
            }

            const glm::mat4 nodeTransform = parentTransform * ToGlmMatrix(node->mTransformation);

            for (uint32_t nodeMeshIndex = 0; nodeMeshIndex < node->mNumMeshes; ++nodeMeshIndex)
            {
                const uint32_t meshIndex = node->mMeshes[nodeMeshIndex];
                if (meshIndex >= meshTemplates.size())
                {
                    continue;
                }

                MeshPrimitiveInstance primitive;
                primitive.primitiveIndex = meshIndex;
                primitive.materialIndex = meshIndex < meshMaterialIndices.size() ? meshMaterialIndices[meshIndex] : 0;
                primitive.name = meshTemplates[meshIndex].name;
                primitive.localTransform = nodeTransform;
                if (primitive.name.empty())
                {
                    primitive.name = node->mName.C_Str();
                }
                outPrimitiveInstances.push_back(std::move(primitive));
            }

            for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
            {
                AppendNodePrimitives(node->mChildren[childIndex], nodeTransform, meshTemplates, meshMaterialIndices, outPrimitiveInstances);
            }
        }

        struct ResolvedMaterialTexture
        {
            std::string path;
            uint32_t uvIndex = 0;
        };

        inline std::optional<ResolvedMaterialTexture> ResolveMaterialTexturePath(
            const aiMaterial *material,
            aiTextureType textureType,
            const std::filesystem::path &meshDirectory,
            const std::string &meshPath)
        {
            if (!material || material->GetTextureCount(textureType) == 0)
            {
                return std::nullopt;
            }

            aiString texturePath;
            unsigned int uvIndex = 0;
            if (material->GetTexture(textureType, 0, &texturePath, nullptr, &uvIndex) != aiReturn_SUCCESS)
            {
                return std::nullopt;
            }

            const std::string texturePathValue = texturePath.C_Str();
            if (texturePathValue.empty())
            {
                return std::nullopt;
            }

            ResolvedMaterialTexture resolvedTexture;
            resolvedTexture.uvIndex = uvIndex;
            if (texturePathValue[0] == '*')
            {
                resolvedTexture.path = std::string("embedded://") + meshPath + "#" + texturePathValue.substr(1);
                return resolvedTexture;
            }

            const std::filesystem::path resolvedPath = meshDirectory / texturePathValue;
            resolvedTexture.path = resolvedPath.string();
            return resolvedTexture;
        }

        inline std::optional<ResolvedMaterialTexture> ResolveFirstAvailableTexturePath(
            const aiMaterial *material,
            std::initializer_list<aiTextureType> textureTypes,
            const std::filesystem::path &meshDirectory,
            const std::string &meshPath)
        {
            for (aiTextureType textureType : textureTypes)
            {
                if (std::optional<ResolvedMaterialTexture> texturePath = ResolveMaterialTexturePath(material, textureType, meshDirectory, meshPath))
                {
                    return texturePath;
                }
            }

            return std::nullopt;
        }
    }

    template <>
    inline std::unique_ptr<Mesh> ResourceLoader<Mesh>::Load(const std::string &path)
    {
        auto mesh = std::make_unique<Mesh>(0, path);
        mesh->SetState(ResourceState::Loading);

        const std::filesystem::path meshPath(path);
        const std::filesystem::path meshDirectory = meshPath.parent_path();

        const std::string ext = meshPath.extension().string();
        const bool needsUvFlip = (ext == ".glb" || ext == ".gltf" || ext == ".GLB" || ext == ".GLTF");

        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(
            path,
            aiProcess_Triangulate |
                aiProcess_JoinIdenticalVertices |
                aiProcess_GenNormals |
                aiProcess_CalcTangentSpace |
                aiProcess_ImproveCacheLocality |
                (needsUvFlip ? aiProcess_FlipUVs : 0u) |
                aiProcess_SortByPType);

        if (!scene || !scene->HasMeshes())
        {
            throw std::runtime_error("Failed to load mesh at path: " + path);
        }

        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<MeshPrimitive> meshTemplates;
        std::vector<uint32_t> meshMaterialIndices;
        meshTemplates.reserve(scene->mNumMeshes);
        meshMaterialIndices.reserve(scene->mNumMeshes);

        uint32_t vertexBase = 0;
        for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
        {
            const aiMesh *sourceMesh = scene->mMeshes[meshIndex];
            if (!sourceMesh)
            {
                continue;
            }

            const uint32_t primitiveIndexOffset = static_cast<uint32_t>(indices.size());

            for (uint32_t vertexIndex = 0; vertexIndex < sourceMesh->mNumVertices; ++vertexIndex)
            {
                MeshVertex vertex;

                const aiVector3D &position = sourceMesh->mVertices[vertexIndex];
                vertex.position = glm::vec3(position.x, position.y, position.z);

                if (sourceMesh->HasNormals())
                {
                    const aiVector3D &normal = sourceMesh->mNormals[vertexIndex];
                    vertex.normal = glm::vec3(normal.x, normal.y, normal.z);
                }

                if (sourceMesh->HasTextureCoords(0))
                {
                    const aiVector3D &uv = sourceMesh->mTextureCoords[0][vertexIndex];
                    vertex.uv0 = glm::vec2(uv.x, uv.y);
                }

                if (sourceMesh->HasTextureCoords(1))
                {
                    const aiVector3D &uv = sourceMesh->mTextureCoords[1][vertexIndex];
                    vertex.uv1 = glm::vec2(uv.x, uv.y);
                }

                if (sourceMesh->HasTangentsAndBitangents())
                {
                    const aiVector3D &tangent = sourceMesh->mTangents[vertexIndex];
                    const aiVector3D &bitangent = sourceMesh->mBitangents[vertexIndex];
                    vertex.tangent = glm::vec3(tangent.x, tangent.y, tangent.z);
                    vertex.bitangent = glm::vec3(bitangent.x, bitangent.y, bitangent.z);
                }

                vertices.push_back(vertex);
            }

            for (uint32_t faceIndex = 0; faceIndex < sourceMesh->mNumFaces; ++faceIndex)
            {
                const aiFace &face = sourceMesh->mFaces[faceIndex];
                if (face.mNumIndices < 3)
                {
                    continue;
                }

                for (uint32_t localIndex = 0; localIndex < face.mNumIndices; ++localIndex)
                {
                    indices.push_back(vertexBase + face.mIndices[localIndex]);
                }
            }

            MeshPrimitive primitive;
            primitive.indexOffset = primitiveIndexOffset;
            primitive.indexCount = static_cast<uint32_t>(indices.size()) - primitiveIndexOffset;
            primitive.name = sourceMesh->mName.C_Str();
            meshTemplates.push_back(std::move(primitive));
            meshMaterialIndices.push_back(sourceMesh->mMaterialIndex);

            vertexBase += sourceMesh->mNumVertices;
        }

        if (vertices.empty() || indices.empty())
        {
            throw std::runtime_error("Mesh file contains no renderable geometry: " + path);
        }

        std::vector<MeshPrimitiveInstance> primitiveInstances;
        if (scene->mRootNode)
        {
            detail::AppendNodePrimitives(scene->mRootNode, glm::mat4(1.0f), meshTemplates, meshMaterialIndices, primitiveInstances);
        }

        mesh->SetData(std::move(vertices), std::move(indices), std::move(meshTemplates), std::move(primitiveInstances));

        if (scene->HasMaterials())
        {
            for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex)
            {
                const aiMaterial *sourceMaterial = scene->mMaterials[materialIndex];
                if (!sourceMaterial)
                {
                    continue;
                }

                if (const std::optional<detail::ResolvedMaterialTexture> baseColorPath = detail::ResolveFirstAvailableTexturePath(
                        sourceMaterial,
                        {aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE},
                        meshDirectory,
                        path))
                {
                    mesh->SetImportedMaterialTextureInfo(materialIndex, MaterialTextureSlot::BaseColor, baseColorPath->path, baseColorPath->uvIndex);
                }

                if (const std::optional<detail::ResolvedMaterialTexture> normalPath = detail::ResolveFirstAvailableTexturePath(
                        sourceMaterial,
                        {aiTextureType_NORMALS, aiTextureType_HEIGHT},
                        meshDirectory,
                        path))
                {
                    mesh->SetImportedMaterialTextureInfo(materialIndex, MaterialTextureSlot::Normal, normalPath->path, normalPath->uvIndex);
                }

                if (const std::optional<detail::ResolvedMaterialTexture> metallicRoughnessPath = detail::ResolveFirstAvailableTexturePath(
                        sourceMaterial,
                        {aiTextureType_DIFFUSE_ROUGHNESS, aiTextureType_METALNESS, aiTextureType_SPECULAR, aiTextureType_UNKNOWN},
                        meshDirectory,
                        path))
                {
                    mesh->SetImportedMaterialTextureInfo(materialIndex, MaterialTextureSlot::MetallicRoughness, metallicRoughnessPath->path, metallicRoughnessPath->uvIndex);
                }

                if (const std::optional<detail::ResolvedMaterialTexture> occlusionPath = detail::ResolveFirstAvailableTexturePath(
                        sourceMaterial,
                        {aiTextureType_AMBIENT_OCCLUSION, aiTextureType_LIGHTMAP, aiTextureType_AMBIENT},
                        meshDirectory,
                        path))
                {
                    mesh->SetImportedMaterialTextureInfo(materialIndex, MaterialTextureSlot::Occlusion, occlusionPath->path, occlusionPath->uvIndex);
                }

                if (const std::optional<detail::ResolvedMaterialTexture> emissivePath = detail::ResolveFirstAvailableTexturePath(
                        sourceMaterial,
                        {aiTextureType_EMISSIVE, aiTextureType_EMISSION_COLOR},
                        meshDirectory,
                        path))
                {
                    mesh->SetImportedMaterialTextureInfo(materialIndex, MaterialTextureSlot::Emissive, emissivePath->path, emissivePath->uvIndex);
                }
            }
        }

        return mesh;
    }
}