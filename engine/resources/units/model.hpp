#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

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
        uint32_t nodeIndex = std::numeric_limits<uint32_t>::max();
        std::string name;
        glm::mat4 localTransform{1.0f};
        bool usesSkinning = false;
    };

    struct SkeletonNode
    {
        std::string name;
        uint32_t parentIndex = std::numeric_limits<uint32_t>::max();
        std::vector<uint32_t> children;
        glm::vec3 localTranslation{0.0f, 0.0f, 0.0f};
        glm::quat localRotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 localScale{1.0f, 1.0f, 1.0f};
        glm::mat4 localBindTransform{1.0f};
        glm::mat4 globalBindTransform{1.0f};
    };

    struct BoneInfo
    {
        std::string name;
        uint32_t nodeIndex = std::numeric_limits<uint32_t>::max();
        glm::mat4 inverseBindMatrix{1.0f};
    };

    struct PositionKey
    {
        double timeTicks = 0.0;
        glm::vec3 value{0.0f, 0.0f, 0.0f};
    };

    struct RotationKey
    {
        double timeTicks = 0.0;
        glm::quat value{1.0f, 0.0f, 0.0f, 0.0f};
    };

    struct ScaleKey
    {
        double timeTicks = 0.0;
        glm::vec3 value{1.0f, 1.0f, 1.0f};
    };

    struct AnimationChannel
    {
        uint32_t nodeIndex = std::numeric_limits<uint32_t>::max();
        std::vector<PositionKey> positionKeys;
        std::vector<RotationKey> rotationKeys;
        std::vector<ScaleKey> scaleKeys;
    };

    struct AnimationClip
    {
        std::string name;
        double durationTicks = 0.0;
        double ticksPerSecond = 25.0;
        std::vector<AnimationChannel> channels;
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

    struct ImportedMaterialProperties
    {
        glm::vec4 baseColorFactor{0.8f, 0.8f, 0.8f, 1.0f};
        glm::vec3 emissiveFactor{0.0f, 0.0f, 0.0f};
        float metallicFactor = 0.0f;
        float roughnessFactor = 0.5f;
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
        bool HasSkinning() const noexcept { return m_mesh.HasSkinning(); }

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

        void SetSkeletonData(
            std::vector<SkeletonNode> skeletonNodes,
            std::vector<BoneInfo> bones,
            std::vector<AnimationClip> animationClips,
            uint32_t rootNodeIndex = 0)
        {
            m_skeletonNodes = std::move(skeletonNodes);
            m_bones = std::move(bones);
            m_animationClips = std::move(animationClips);
            m_rootNodeIndex = rootNodeIndex;

            m_nodeNameToIndex.clear();
            m_nodeNameToIndex.reserve(m_skeletonNodes.size());
            for (uint32_t nodeIndex = 0; nodeIndex < static_cast<uint32_t>(m_skeletonNodes.size()); ++nodeIndex)
            {
                m_nodeNameToIndex.emplace(m_skeletonNodes[nodeIndex].name, nodeIndex);
            }

            m_animationNameToIndex.clear();
            m_animationNameToIndex.reserve(m_animationClips.size());
            for (uint32_t clipIndex = 0; clipIndex < static_cast<uint32_t>(m_animationClips.size()); ++clipIndex)
            {
                if (!m_animationClips[clipIndex].name.empty())
                {
                    m_animationNameToIndex.emplace(m_animationClips[clipIndex].name, clipIndex);
                }
            }
        }

        bool HasSkeleton() const noexcept { return !m_skeletonNodes.empty() && !m_bones.empty(); }
        bool HasAnimations() const noexcept { return !m_animationClips.empty(); }

        const std::vector<SkeletonNode> &GetSkeletonNodes() const noexcept { return m_skeletonNodes; }
        const std::vector<BoneInfo> &GetBones() const noexcept { return m_bones; }
        const std::vector<AnimationClip> &GetAnimationClips() const noexcept { return m_animationClips; }
        uint32_t GetRootNodeIndex() const noexcept { return m_rootNodeIndex; }

        const AnimationClip *GetAnimationClip(uint32_t clipIndex) const noexcept
        {
            if (clipIndex >= m_animationClips.size())
                return nullptr;
            return &m_animationClips[clipIndex];
        }

        uint32_t GetAnimationClipCount() const noexcept
        {
            return static_cast<uint32_t>(m_animationClips.size());
        }

        std::optional<uint32_t> FindAnimationClipIndex(const std::string &clipName) const
        {
            auto clipIt = m_animationNameToIndex.find(clipName);
            if (clipIt == m_animationNameToIndex.end())
                return std::nullopt;
            return clipIt->second;
        }

        std::optional<uint32_t> FindNodeIndex(const std::string &nodeName) const
        {
            auto nodeIt = m_nodeNameToIndex.find(nodeName);
            if (nodeIt == m_nodeNameToIndex.end())
                return std::nullopt;
            return nodeIt->second;
        }

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

        void SetImportedMaterialProperties(uint32_t materialIndex, ImportedMaterialProperties properties)
        {
            m_importedMaterialProperties[materialIndex] = std::move(properties);
        }

        std::optional<ImportedMaterialProperties> GetImportedMaterialProperties(uint32_t materialIndex) const
        {
            auto matIt = m_importedMaterialProperties.find(materialIndex);
            if (matIt == m_importedMaterialProperties.end())
                return std::nullopt;
            return matIt->second;
        }

    private:
        Mesh m_mesh;
        std::vector<MeshPrimitiveInstance> m_primitiveInstances;
        std::unordered_map<uint32_t, MeshMaterialTextureInfo> m_importedMaterialTextures;
        std::unordered_map<uint32_t, ImportedMaterialProperties> m_importedMaterialProperties;
        std::vector<SkeletonNode> m_skeletonNodes;
        std::vector<BoneInfo> m_bones;
        std::vector<AnimationClip> m_animationClips;
        std::unordered_map<std::string, uint32_t> m_nodeNameToIndex;
        std::unordered_map<std::string, uint32_t> m_animationNameToIndex;
        uint32_t m_rootNodeIndex = 0;
    };
}
