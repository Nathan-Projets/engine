#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include <glm/glm.hpp>

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

    class Mesh
    {
    public:
        Mesh() = default;

        void SetData(
            std::vector<MeshVertex> vertices,
            std::vector<uint32_t> indices,
            std::vector<MeshPrimitive> primitives = {})
        {
            m_vertices = std::move(vertices);
            m_indices = std::move(indices);
            m_primitives = std::move(primitives);

            if (m_primitives.empty() && !m_indices.empty())
                m_primitives.push_back({0, static_cast<uint32_t>(m_indices.size()), "default"});

            m_hasNormals = std::any_of(m_vertices.begin(), m_vertices.end(),
                                       [](const MeshVertex &v)
                                       { return v.normal != glm::vec3(0.0f); });
            m_hasTexCoords = std::any_of(m_vertices.begin(), m_vertices.end(),
                                         [](const MeshVertex &v)
                                         { return v.uv0 != glm::vec2(0.0f); });
            m_hasTangents = std::any_of(m_vertices.begin(), m_vertices.end(),
                                        [](const MeshVertex &v)
                                        { return v.tangent != glm::vec3(0.0f); });

            RecalculateBounds();
        }

        const std::vector<MeshVertex> &GetVertices() const noexcept { return m_vertices; }
        const std::vector<uint32_t> &GetIndices() const noexcept { return m_indices; }
        const std::vector<MeshPrimitive> &GetPrimitives() const noexcept { return m_primitives; }

        bool IsEmpty() const noexcept { return m_vertices.empty() || m_indices.empty(); }
        bool HasNormals() const noexcept { return m_hasNormals; }
        bool HasTexCoords() const noexcept { return m_hasTexCoords; }
        bool HasTangents() const noexcept { return m_hasTangents; }

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
                m_boundsMin = m_boundsMax = glm::vec3(0.0f);
                return;
            }
            glm::vec3 minV(std::numeric_limits<float>::max());
            glm::vec3 maxV(std::numeric_limits<float>::lowest());
            for (const MeshVertex &v : m_vertices)
            {
                minV = glm::min(minV, v.position);
                maxV = glm::max(maxV, v.position);
            }
            m_boundsMin = minV;
            m_boundsMax = maxV;
        }

        std::vector<MeshVertex> m_vertices;
        std::vector<uint32_t> m_indices;
        std::vector<MeshPrimitive> m_primitives;

        bool m_hasNormals = false;
        bool m_hasTexCoords = false;
        bool m_hasTangents = false;

        glm::vec3 m_boundsMin{0.0f};
        glm::vec3 m_boundsMax{0.0f};

        uint32_t m_vao = 0;
        uint32_t m_vbo = 0;
        uint32_t m_ebo = 0;
    };
}
