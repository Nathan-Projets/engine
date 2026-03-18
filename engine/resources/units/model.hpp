#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "../resource.hpp"
#include "mesh.hpp"
#include "material.hpp"

namespace resources
{
    // One instance of a geometry primitive placed in the scene hierarchy.
    struct MeshPrimitiveInstance
    {
        uint32_t primitiveIndex = 0;
        uint32_t materialIndex = 0;
        std::string name;
        glm::mat4 localTransform{1.0f};
    };

    struct ImportedMaterialTextureInfo
    {
        std::string path;
        uint32_t uvIndex = 0;
    };

    struct MeshMaterialTextureInfo
    {
        std::unordered_map<MaterialTextureSlot, ImportedMaterialTextureInfo, MaterialTextureSlotHash> textures;
    };

    // Scene-level resource: owns the geometry (via an embedded Mesh), the node hierarchy (MeshPrimitiveInstance list), and imported material texture data.
    class Model : public Resource
    {
    public:
        Model(uint32_t id, const std::string &path = "") : Resource(id, path) {}

        void SetData(
            std::vector<MeshVertex> vertices,
            std::vector<uint32_t> indices,
            std::vector<MeshPrimitive> primitives = {},
            std::vector<MeshPrimitiveInstance> primitiveInstances = {})
        {
            m_mesh.SetData(std::move(vertices), std::move(indices), std::move(primitives));
            m_primitiveInstances = std::move(primitiveInstances);

            if (m_primitiveInstances.empty())
            {
                const auto &prims = m_mesh.GetPrimitives();
                m_primitiveInstances.reserve(prims.size());
                for (uint32_t i = 0; i < static_cast<uint32_t>(prims.size()); ++i)
                {
                    MeshPrimitiveInstance inst;
                    inst.primitiveIndex = i;
                    inst.name = prims[i].name;
                    m_primitiveInstances.push_back(std::move(inst));
                }
            }
        }

        const std::vector<MeshVertex> &GetVertices() const noexcept { return m_mesh.GetVertices(); }
        const std::vector<uint32_t> &GetIndices() const noexcept { return m_mesh.GetIndices(); }
        const std::vector<MeshPrimitive> &GetPrimitives() const noexcept { return m_mesh.GetPrimitives(); }

        bool IsEmpty() const noexcept { return m_mesh.IsEmpty(); }
        bool HasNormals() const noexcept { return m_mesh.HasNormals(); }
        bool HasTexCoords() const noexcept { return m_mesh.HasTexCoords(); }
        bool HasTangents() const noexcept { return m_mesh.HasTangents(); }

        const glm::vec3 &GetBoundsMin() const noexcept { return m_mesh.GetBoundsMin(); }
        const glm::vec3 &GetBoundsMax() const noexcept { return m_mesh.GetBoundsMax(); }

        void SetGpuHandles(uint32_t vao, uint32_t vbo, uint32_t ebo) noexcept { m_mesh.SetGpuHandles(vao, vbo, ebo); }
        uint32_t GetVao() const noexcept { return m_mesh.GetVao(); }
        uint32_t GetVbo() const noexcept { return m_mesh.GetVbo(); }
        uint32_t GetEbo() const noexcept { return m_mesh.GetEbo(); }
        bool IsGpuReady() const noexcept { return m_mesh.IsGpuReady(); }

        Mesh &GetMesh() noexcept { return m_mesh; }
        const Mesh &GetMesh() const noexcept { return m_mesh; }

        const std::vector<MeshPrimitiveInstance> &GetPrimitiveInstances() const noexcept { return m_primitiveInstances; }

        void SetImportedMaterialTextureInfo(uint32_t materialIndex, MaterialTextureSlot slot, std::string texturePath, uint32_t uvIndex = 0)
        {
            m_importedMaterialTextures[materialIndex].textures[slot] =
                ImportedMaterialTextureInfo{std::move(texturePath), uvIndex};
        }

        std::optional<std::string> GetImportedMaterialTexturePath(uint32_t materialIndex, MaterialTextureSlot slot) const
        {
            auto matIt = m_importedMaterialTextures.find(materialIndex);
            if (matIt == m_importedMaterialTextures.end())
                return std::nullopt;
            auto texIt = matIt->second.textures.find(slot);
            if (texIt == matIt->second.textures.end())
                return std::nullopt;
            return texIt->second.path;
        }

        uint32_t GetImportedMaterialTextureUvIndex(uint32_t materialIndex, MaterialTextureSlot slot) const
        {
            auto matIt = m_importedMaterialTextures.find(materialIndex);
            if (matIt == m_importedMaterialTextures.end())
                return 0;
            auto texIt = matIt->second.textures.find(slot);
            if (texIt == matIt->second.textures.end())
                return 0;
            return texIt->second.uvIndex;
        }

    private:
        Mesh m_mesh;
        std::vector<MeshPrimitiveInstance> m_primitiveInstances;
        std::unordered_map<uint32_t, MeshMaterialTextureInfo> m_importedMaterialTextures;
    };
}
