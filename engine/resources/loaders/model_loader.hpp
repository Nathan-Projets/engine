#pragma once

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "../units/model.hpp"
#include "../resource_loader.hpp"

namespace resources
{
    namespace detail
    {
        inline glm::mat4 ToGlmMatrix(const aiMatrix4x4 &m)
        {
            glm::mat4 r(1.0f);
            r[0][0] = m.a1;
            r[1][0] = m.a2;
            r[2][0] = m.a3;
            r[3][0] = m.a4;
            r[0][1] = m.b1;
            r[1][1] = m.b2;
            r[2][1] = m.b3;
            r[3][1] = m.b4;
            r[0][2] = m.c1;
            r[1][2] = m.c2;
            r[2][2] = m.c3;
            r[3][2] = m.c4;
            r[0][3] = m.d1;
            r[1][3] = m.d2;
            r[2][3] = m.d3;
            r[3][3] = m.d4;
            return r;
        }

        inline void AppendNodePrimitives(
            const aiNode *node,
            const glm::mat4 &parentTransform,
            const std::vector<MeshPrimitive> &meshTemplates,
            const std::vector<uint32_t> &meshMaterialIndices,
            std::vector<MeshPrimitiveInstance> &outInstances)
        {
            if (!node)
                return;
            const glm::mat4 nodeTransform = parentTransform * ToGlmMatrix(node->mTransformation);

            for (uint32_t ni = 0; ni < node->mNumMeshes; ++ni)
            {
                const uint32_t mi = node->mMeshes[ni];
                if (mi >= meshTemplates.size())
                    continue;

                MeshPrimitiveInstance inst;
                inst.primitiveIndex = mi;
                inst.materialIndex = mi < meshMaterialIndices.size() ? meshMaterialIndices[mi] : 0;
                inst.name = meshTemplates[mi].name;
                inst.localTransform = nodeTransform;
                if (inst.name.empty())
                    inst.name = node->mName.C_Str();
                outInstances.push_back(std::move(inst));
            }

            for (uint32_t ci = 0; ci < node->mNumChildren; ++ci)
                AppendNodePrimitives(node->mChildren[ci], nodeTransform, meshTemplates, meshMaterialIndices, outInstances);
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
                return std::nullopt;
            aiString texturePath;
            unsigned int uvIndex = 0;
            if (material->GetTexture(textureType, 0, &texturePath, nullptr, &uvIndex) != aiReturn_SUCCESS)
                return std::nullopt;
            const std::string texStr = texturePath.C_Str();
            if (texStr.empty())
                return std::nullopt;

            ResolvedMaterialTexture result;
            result.uvIndex = uvIndex;
            if (texStr[0] == '*')
            {
                result.path = std::string("embedded://") + meshPath + "#" + texStr.substr(1);
                return result;
            }
            result.path = (meshDirectory / texStr).string();
            return result;
        }

        inline std::optional<ResolvedMaterialTexture> ResolveFirstAvailableTexturePath(
            const aiMaterial *material,
            std::initializer_list<aiTextureType> types,
            const std::filesystem::path &meshDirectory,
            const std::string &meshPath)
        {
            for (aiTextureType t : types)
                if (auto r = ResolveMaterialTexturePath(material, t, meshDirectory, meshPath))
                    return r;
            return std::nullopt;
        }
    }

    template <>
    inline std::unique_ptr<Model> ResourceLoader<Model>::Load(const std::string &path)
    {
        auto model = std::make_unique<Model>(0, path);
        model->SetState(ResourceState::Loading);

        const std::filesystem::path meshPath(path);
        const std::filesystem::path meshDirectory = meshPath.parent_path();
        const std::string ext = meshPath.extension().string();
        const bool needsUvFlip = (ext == ".glb" || ext == ".gltf" ||
                                  ext == ".GLB" || ext == ".GLTF");

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
            throw std::runtime_error("Failed to load model at path: " + path);

        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<MeshPrimitive> meshTemplates;
        std::vector<uint32_t> meshMaterialIndices;
        meshTemplates.reserve(scene->mNumMeshes);
        meshMaterialIndices.reserve(scene->mNumMeshes);

        uint32_t vertexBase = 0;
        for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
        {
            const aiMesh *src = scene->mMeshes[meshIndex];
            if (!src)
                continue;

            const uint32_t primitiveIndexOffset = static_cast<uint32_t>(indices.size());

            for (uint32_t i = 0; i < src->mNumVertices; ++i)
            {
                MeshVertex v;
                const aiVector3D &p = src->mVertices[i];
                v.position = glm::vec3(p.x, p.y, p.z);
                if (src->HasNormals())
                {
                    const aiVector3D &n = src->mNormals[i];
                    v.normal = glm::vec3(n.x, n.y, n.z);
                }
                if (src->HasTextureCoords(0))
                {
                    const aiVector3D &uv = src->mTextureCoords[0][i];
                    v.uv0 = glm::vec2(uv.x, uv.y);
                }
                if (src->HasTextureCoords(1))
                {
                    const aiVector3D &uv = src->mTextureCoords[1][i];
                    v.uv1 = glm::vec2(uv.x, uv.y);
                }
                if (src->HasTangentsAndBitangents())
                {
                    const aiVector3D &t = src->mTangents[i];
                    const aiVector3D &bt = src->mBitangents[i];
                    v.tangent = glm::vec3(t.x, t.y, t.z);
                    v.bitangent = glm::vec3(bt.x, bt.y, bt.z);
                }
                vertices.push_back(v);
            }

            for (uint32_t f = 0; f < src->mNumFaces; ++f)
            {
                const aiFace &face = src->mFaces[f];
                if (face.mNumIndices < 3)
                    continue;
                for (uint32_t li = 0; li < face.mNumIndices; ++li)
                    indices.push_back(vertexBase + face.mIndices[li]);
            }

            MeshPrimitive prim;
            prim.indexOffset = primitiveIndexOffset;
            prim.indexCount = static_cast<uint32_t>(indices.size()) - primitiveIndexOffset;
            prim.name = src->mName.C_Str();
            meshTemplates.push_back(std::move(prim));
            meshMaterialIndices.push_back(src->mMaterialIndex);
            vertexBase += src->mNumVertices;
        }

        if (vertices.empty() || indices.empty())
            throw std::runtime_error("Model file contains no renderable geometry: " + path);

        std::vector<MeshPrimitiveInstance> primitiveInstances;
        if (scene->mRootNode)
            detail::AppendNodePrimitives(scene->mRootNode, glm::mat4(1.0f),
                                         meshTemplates, meshMaterialIndices, primitiveInstances);

        model->SetData(std::move(vertices), std::move(indices),
                       std::move(meshTemplates), std::move(primitiveInstances));

        if (scene->HasMaterials())
        {
            for (uint32_t mi = 0; mi < scene->mNumMaterials; ++mi)
            {
                const aiMaterial *src = scene->mMaterials[mi];
                if (!src)
                    continue;

                if (auto t = detail::ResolveFirstAvailableTexturePath(src,
                                                                      {aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE},
                                                                      meshDirectory, path))
                    model->SetImportedMaterialTextureInfo(mi, MaterialTextureSlot::BaseColor, t->path, t->uvIndex);

                if (auto t = detail::ResolveFirstAvailableTexturePath(src,
                                                                      {aiTextureType_NORMALS, aiTextureType_HEIGHT},
                                                                      meshDirectory, path))
                    model->SetImportedMaterialTextureInfo(mi, MaterialTextureSlot::Normal, t->path, t->uvIndex);

                if (auto t = detail::ResolveFirstAvailableTexturePath(src,
                                                                      {aiTextureType_DIFFUSE_ROUGHNESS, aiTextureType_METALNESS,
                                                                       aiTextureType_SPECULAR, aiTextureType_UNKNOWN},
                                                                      meshDirectory, path))
                    model->SetImportedMaterialTextureInfo(mi, MaterialTextureSlot::MetallicRoughness, t->path, t->uvIndex);

                if (auto t = detail::ResolveFirstAvailableTexturePath(src,
                                                                      {aiTextureType_AMBIENT_OCCLUSION, aiTextureType_LIGHTMAP,
                                                                       aiTextureType_AMBIENT},
                                                                      meshDirectory, path))
                    model->SetImportedMaterialTextureInfo(mi, MaterialTextureSlot::Occlusion, t->path, t->uvIndex);

                if (auto t = detail::ResolveFirstAvailableTexturePath(src,
                                                                      {aiTextureType_EMISSIVE, aiTextureType_EMISSION_COLOR},
                                                                      meshDirectory, path))
                    model->SetImportedMaterialTextureInfo(mi, MaterialTextureSlot::Emissive, t->path, t->uvIndex);
            }
        }

        return model;
    }
}
