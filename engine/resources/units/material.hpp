#pragma once

#include <cstdint>
#include <optional>
#include <variant>
#include <string>
#include <unordered_map>
#include <utility>

#include <glm/glm.hpp>

#include "../resource.hpp"
#include "../handle.hpp"
#include "shader.hpp"
#include "texture.hpp"

namespace resources
{
    /// @enum MaterialTextureSlot
    /// @brief Standard PBR texture slots supported by materials
    enum class MaterialTextureSlot
    {
        BaseColor,         ///< Base color / albedo texture
        Normal,            ///< Normal map for surface detail
        MetallicRoughness, ///< Metallic (R) and roughness (G) in packed texture
        Occlusion,         ///< Ambient occlusion texture
        Emissive,          ///< Emissive/self-illumination texture
        Displacement,      ///< Displacement/height map for vertex deformation
        Skybox             ///< Cubemap skybox texture
    };

    struct MaterialTextureSlotHash
    {
        std::size_t operator()(MaterialTextureSlot slot) const noexcept
        {
            return static_cast<std::size_t>(slot);
        }
    };

    /// @typedef MaterialPropertyValue
    /// @brief Variant holding material property values
    /// @details Supports bool, int32, uint32, float, and GLM vector types (vec2, vec3, vec4)
    using MaterialPropertyValue = std::variant<bool, int32_t, uint32_t, float, glm::vec2, glm::vec3, glm::vec4>;

    /**
     * @class Material
     * @brief Represents a material resource with shader, textures, and properties
     * @details
     * A Material is a container for rendering state including:
     * - Shader to be used for rendering
     * - Texture maps assigned to standard slots (base color, normal, etc)
     * - Material properties edited per-material instance (colors, roughness, metallic, etc)
     * - Render state flags (double-sided, alpha blending)
     *
     * Materials are loaded from JSON files that specify shader references and properties.
     */
    class Material : public Resource
    {
    public:
        Material(uint32_t id, const std::string &path = "") : Resource(id, path) {}

        /// @name Shader binding
        /// @{

        /// @brief Set the file path to the shader used by this material
        void SetShaderPath(std::string shaderPath)
        {
            m_shaderPath = std::move(shaderPath);
        }

        /// @brief Get the shader file path
        const std::string &GetShaderPath() const noexcept
        {
            return m_shaderPath;
        }

        /// @brief Set the resolved shader handle (after async loading)
        void SetShaderHandle(Handle<Shader> shaderHandle) noexcept
        {
            m_shaderHandle = shaderHandle;
        }

        /// @brief Get the resolved shader handle
        Handle<Shader> GetShaderHandle() const noexcept
        {
            return m_shaderHandle;
        }

        /// @}

        /// @name Texture binding
        /// @{

        /// @brief Register a texture file path for the given slot
        /// @param slot The texture slot (e.g., BaseColor, Normal)
        /// @param path File path to the texture asset
        void SetTexturePath(MaterialTextureSlot slot, std::string path)
        {
            m_texturePaths[slot] = std::move(path);
        }

        /// @brief Check if a texture slot has a registered path
        bool HasTexturePath(MaterialTextureSlot slot) const
        {
            return m_texturePaths.find(slot) != m_texturePaths.end();
        }

        /// @brief Get the texture file path for a slot, if assigned
        std::optional<std::string> GetTexturePath(MaterialTextureSlot slot) const
        {
            auto iterator = m_texturePaths.find(slot);
            if (iterator == m_texturePaths.end())
            {
                return std::nullopt;
            }

            return iterator->second;
        }

        /// @brief Get all registered texture paths
        const std::unordered_map<MaterialTextureSlot, std::string, MaterialTextureSlotHash> &GetTexturePaths() const noexcept
        {
            return m_texturePaths;
        }

        /// @brief Set the resolved texture handle for a slot (after async loading)
        void SetTextureHandle(MaterialTextureSlot slot, Handle<Texture> textureHandle) noexcept
        {
            m_textureHandles[slot] = textureHandle;
        }

        /// @brief Get the resolved texture handle for a slot
        std::optional<Handle<Texture>> GetTextureHandle(MaterialTextureSlot slot) const
        {
            auto iterator = m_textureHandles.find(slot);
            if (iterator == m_textureHandles.end())
            {
                return std::nullopt;
            }

            return iterator->second;
        }

        /// @}

        /// @name Material properties
        /// @{

        /// @brief Set a named material property
        /// @details Properties can be float, int, bool, vec2, vec3, or vec4
        /// Common properties: baseColor, roughness, metallic, emissive
        void SetProperty(std::string name, MaterialPropertyValue value)
        {
            m_properties[std::move(name)] = std::move(value);
        }

        /// @brief Check if a named property exists
        bool HasProperty(const std::string &name) const
        {
            return m_properties.find(name) != m_properties.end();
        }

        /// @brief Get all material properties
        const std::unordered_map<std::string, MaterialPropertyValue> &GetProperties() const noexcept
        {
            return m_properties;
        }

        /// @}

        /// @name Render state
        /// @{

        /// @brief Enable/disable double-sided rendering
        void SetDoubleSided(bool value) noexcept { m_doubleSided = value; }
        /// @brief Check if material should render both front and back faces
        bool IsDoubleSided() const noexcept { return m_doubleSided; }

        /// @brief Enable/disable alpha blending
        void SetAlphaBlend(bool value) noexcept { m_alphaBlend = value; }
        /// @brief Check if material uses alpha blending
        bool UsesAlphaBlend() const noexcept { return m_alphaBlend; }

        /// @}

    private:
        std::string m_shaderPath;
        Handle<Shader> m_shaderHandle;

        std::unordered_map<MaterialTextureSlot, std::string, MaterialTextureSlotHash> m_texturePaths;
        std::unordered_map<MaterialTextureSlot, Handle<Texture>, MaterialTextureSlotHash> m_textureHandles;

        std::unordered_map<std::string, MaterialPropertyValue> m_properties;

        bool m_doubleSided = false;
        bool m_alphaBlend = false;
    };
}