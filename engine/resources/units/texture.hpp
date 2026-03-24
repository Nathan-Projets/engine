#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>

#include "../resource.hpp"

namespace resources
{
    enum class TextureType
    {
        Texture2D,
        Cubemap
    };

    enum class TextureFormat
    {
        Unknown,
        R8,
        RG8,
        RGB8,
        RGBA8,
        R32F,
        RGB32F,
        RGBA32F
    };

    enum class TextureColorSpace
    {
        Linear,
        SRGB
    };

    /**
     * @class Texture
     * @brief Represents a texture resource
     */
    class Texture : public Resource
    {
    public:
        Texture(uint32_t id, const std::string &path = "") : Resource(id, path) {}

        void SetPixels(
            int32_t width,
            int32_t height,
            int32_t channels,
            TextureFormat format,
            TextureColorSpace colorSpace,
            std::vector<uint8_t> pixels,
            bool isHdr)
        {
            m_width = width;
            m_height = height;
            m_channels = channels;
            m_format = format;
            m_colorSpace = colorSpace;
            m_pixels = std::move(pixels);
            m_isHdr = isHdr;
        }

        int32_t GetWidth() const noexcept { return m_width; }
        int32_t GetHeight() const noexcept { return m_height; }
        int32_t GetChannels() const noexcept { return m_channels; }

        TextureFormat GetFormat() const noexcept { return m_format; }
        TextureColorSpace GetColorSpace() const noexcept { return m_colorSpace; }

        bool IsHdr() const noexcept { return m_isHdr; }
        bool IsEmpty() const noexcept { return m_pixels.empty(); }

        const std::vector<uint8_t> &GetPixels() const noexcept { return m_pixels; }

        void SetTextureId(uint32_t textureId) noexcept { m_textureId = textureId; }
        uint32_t GetTextureId() const noexcept { return m_textureId; }
        bool IsGpuReady() const noexcept { return m_textureId != 0; }

        void SetTextureType(TextureType type) noexcept { m_type = type; }
        TextureType GetTextureType() const noexcept { return m_type; }
        bool IsCubemap() const noexcept { return m_type == TextureType::Cubemap; }

        void SetCubemapFacePaths(std::array<std::string, 6> facePaths)
        {
            m_cubemapFacePaths = std::move(facePaths);
            m_type = TextureType::Cubemap;
        }

        const std::array<std::string, 6> &GetCubemapFacePaths() const noexcept
        {
            return m_cubemapFacePaths;
        }

    private:
        int32_t m_width = 0;
        int32_t m_height = 0;
        int32_t m_channels = 0;

        TextureFormat m_format = TextureFormat::Unknown;
        TextureColorSpace m_colorSpace = TextureColorSpace::Linear;

        std::vector<uint8_t> m_pixels;
        bool m_isHdr = false;
        uint32_t m_textureId = 0;
        TextureType m_type = TextureType::Texture2D;
        std::array<std::string, 6> m_cubemapFacePaths = {};
    };
}