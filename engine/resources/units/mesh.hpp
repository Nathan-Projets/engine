#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "../resource.hpp"
#include "material.hpp"

namespace resources
{
    struct MeshVertex
    {
        glm::vec3 position{0.0f, 0.0f, 0.0f};
        glm::vec3 normal{0.0f, 0.0f, 0.0f};
        glm::vec2 uv0{0.0f, 0.0f};
        glm::vec2 uv1{0.0f, 0.0f};
        glm::vec3 tangent{0.0f, 0.0f, 0.0f};
        glm::vec3 bitangent{0.0f, 0.0f, 0.0f};
    };

    struct MeshPrimitive
    {
        uint32_t indexOffset = 0;
        uint32_t indexCount = 0;
        std::string name;
    };

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

    /**
     * @class Mesh
     * @brief Represents a 3D mesh resource
     */
    class Mesh : public Resource
    {
    public:
        Mesh(uint32_t id, const std::string &path = "") : Resource(id, path) {}

        void SetData(
            std::vector<MeshVertex> vertices,
            std::vector<uint32_t> indices,
            std::vector<MeshPrimitive> primitives = {},
            std::vector<MeshPrimitiveInstance> primitiveInstances = {})
        {
            m_vertices = std::move(vertices);
            m_indices = std::move(indices);
            m_primitives = std::move(primitives);
            m_primitiveInstances = std::move(primitiveInstances);

            if (m_primitives.empty() && !m_indices.empty())
            {
                m_primitives.push_back({0, static_cast<uint32_t>(m_indices.size()), "default"});
            }

            if (m_primitiveInstances.empty() && !m_primitives.empty())
            {
                m_primitiveInstances.reserve(m_primitives.size());
                for (uint32_t primitiveIndex = 0; primitiveIndex < static_cast<uint32_t>(m_primitives.size()); ++primitiveIndex)
                {
                    MeshPrimitiveInstance instance;
                    instance.primitiveIndex = primitiveIndex;
                    instance.name = m_primitives[primitiveIndex].name;
                    m_primitiveInstances.push_back(std::move(instance));
                }
            }

            m_hasNormals = std::any_of(
                m_vertices.begin(),
                m_vertices.end(),
                [](const MeshVertex &vertex)
                {
                    return vertex.normal != glm::vec3(0.0f);
                });

            m_hasTexCoords = std::any_of(
                m_vertices.begin(),
                m_vertices.end(),
                [](const MeshVertex &vertex)
                {
                    return vertex.uv0 != glm::vec2(0.0f);
                });

            m_hasTangents = std::any_of(
                m_vertices.begin(),
                m_vertices.end(),
                [](const MeshVertex &vertex)
                {
                    return vertex.tangent != glm::vec3(0.0f);
                });

            RecalculateBounds();
        }

        const std::vector<MeshVertex> &GetVertices() const noexcept { return m_vertices; }
        const std::vector<uint32_t> &GetIndices() const noexcept { return m_indices; }
        const std::vector<MeshPrimitive> &GetPrimitives() const noexcept { return m_primitives; }
        const std::vector<MeshPrimitiveInstance> &GetPrimitiveInstances() const noexcept { return m_primitiveInstances; }

        bool IsEmpty() const noexcept
        {
            return m_vertices.empty() || m_indices.empty();
        }

        bool HasNormals() const noexcept { return m_hasNormals; }
        bool HasTexCoords() const noexcept { return m_hasTexCoords; }
        bool HasTangents() const noexcept { return m_hasTangents; }

        void SetImportedMaterialTextureInfo(uint32_t materialIndex, MaterialTextureSlot slot, std::string texturePath, uint32_t uvIndex = 0)
        {
            m_importedMaterialTextures[materialIndex].textures[slot] = ImportedMaterialTextureInfo{std::move(texturePath), uvIndex};
        }

        std::optional<std::string> GetImportedMaterialTexturePath(uint32_t materialIndex, MaterialTextureSlot slot) const
        {
            auto materialIt = m_importedMaterialTextures.find(materialIndex);
            if (materialIt == m_importedMaterialTextures.end())
            {
                return std::nullopt;
            }

            auto textureIt = materialIt->second.textures.find(slot);
            if (textureIt == materialIt->second.textures.end())
            {
                return std::nullopt;
            }

            return textureIt->second.path;
        }

        uint32_t GetImportedMaterialTextureUvIndex(uint32_t materialIndex, MaterialTextureSlot slot) const
        {
            auto materialIt = m_importedMaterialTextures.find(materialIndex);
            if (materialIt == m_importedMaterialTextures.end())
            {
                return 0;
            }

            auto textureIt = materialIt->second.textures.find(slot);
            if (textureIt == materialIt->second.textures.end())
            {
                return 0;
            }

            return textureIt->second.uvIndex;
        }

        const glm::vec3 &GetBoundsMin() const noexcept { return m_boundsMin; }
        const glm::vec3 &GetBoundsMax() const noexcept { return m_boundsMax; }

        void SetGpuHandles(uint32_t vao, uint32_t vbo, uint32_t ebo) noexcept
        {
            m_vao = vao;
            m_vbo = vbo;
            m_ebo = ebo;
        }

        uint32_t GetVao() const noexcept { return m_vao; }
        uint32_t GetVbo() const noexcept { return m_vbo; }
        uint32_t GetEbo() const noexcept { return m_ebo; }
        bool IsGpuReady() const noexcept { return m_vao != 0; }

    private:
        void RecalculateBounds()
        {
            if (m_vertices.empty())
            {
                m_boundsMin = glm::vec3(0.0f);
                m_boundsMax = glm::vec3(0.0f);
                return;
            }

            glm::vec3 minValue(std::numeric_limits<float>::max());
            glm::vec3 maxValue(std::numeric_limits<float>::lowest());

            for (const MeshVertex &vertex : m_vertices)
            {
                minValue = glm::min(minValue, vertex.position);
                maxValue = glm::max(maxValue, vertex.position);
            }

            m_boundsMin = minValue;
            m_boundsMax = maxValue;
        }

        std::vector<MeshVertex> m_vertices;
        std::vector<uint32_t> m_indices;
        std::vector<MeshPrimitive> m_primitives;
        std::vector<MeshPrimitiveInstance> m_primitiveInstances;

        bool m_hasNormals = false;
        bool m_hasTexCoords = false;
        bool m_hasTangents = false;

        glm::vec3 m_boundsMin{0.0f, 0.0f, 0.0f};
        glm::vec3 m_boundsMax{0.0f, 0.0f, 0.0f};

        std::unordered_map<uint32_t, MeshMaterialTextureInfo> m_importedMaterialTextures;

        uint32_t m_vao = 0;
        uint32_t m_vbo = 0;
        uint32_t m_ebo = 0;
    };
}