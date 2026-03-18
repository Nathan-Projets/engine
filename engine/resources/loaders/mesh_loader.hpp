#pragma once

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <stdexcept>
#include <memory>
#include <string>
#include <vector>

#include "../units/mesh.hpp"
#include "../resource_loader.hpp"

namespace resources
{
    template <>
    inline std::unique_ptr<Mesh> ResourceLoader<Mesh>::Load(const std::string &path)
    {
        auto mesh = std::make_unique<Mesh>(0, path);
        mesh->SetState(ResourceState::Loading);

        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(
            path,
            aiProcess_Triangulate |
                aiProcess_JoinIdenticalVertices |
                aiProcess_GenNormals |
                aiProcess_CalcTangentSpace |
                aiProcess_ImproveCacheLocality |
                aiProcess_SortByPType);

        if (!scene || !scene->HasMeshes())
        {
            throw std::runtime_error("Failed to load mesh at path: " + path);
        }

        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<MeshPrimitive> primitives;

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
            primitive.materialIndex = sourceMesh->mMaterialIndex;
            primitive.name = sourceMesh->mName.C_Str();
            primitives.push_back(std::move(primitive));

            vertexBase += sourceMesh->mNumVertices;
        }

        if (vertices.empty() || indices.empty())
        {
            throw std::runtime_error("Mesh file contains no renderable geometry: " + path);
        }

        mesh->SetData(std::move(vertices), std::move(indices), std::move(primitives));

        return mesh;
    }
}