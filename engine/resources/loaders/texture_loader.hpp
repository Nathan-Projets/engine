#pragma once

#include <stb_image.h>

#include <cctype>
#include <cstring>
#include <stdexcept>
#include <memory>
#include <string>
#include <vector>

#include "../units/texture.hpp"
#include "../resource_loader.hpp"

namespace resources
{
    namespace detail
    {
        inline TextureFormat TextureFormatFromChannels(int channels, bool isHdr)
        {
            if (isHdr)
            {
                switch (channels)
                {
                case 1:
                    return TextureFormat::R32F;
                case 3:
                    return TextureFormat::RGB32F;
                case 4:
                    return TextureFormat::RGBA32F;
                default:
                    return TextureFormat::Unknown;
                }
            }

            switch (channels)
            {
            case 1:
                return TextureFormat::R8;
            case 2:
                return TextureFormat::RG8;
            case 3:
                return TextureFormat::RGB8;
            case 4:
                return TextureFormat::RGBA8;
            default:
                return TextureFormat::Unknown;
            }
        }

        inline TextureColorSpace InferColorSpace(const std::string &path)
        {
            const std::string::size_type dotIndex = path.find_last_of('.');
            if (dotIndex == std::string::npos)
            {
                return TextureColorSpace::Linear;
            }

            std::string extension = path.substr(dotIndex + 1);
            for (char &character : extension)
            {
                character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
            }

            if (extension == "png" || extension == "jpg" || extension == "jpeg" || extension == "tga")
            {
                return TextureColorSpace::SRGB;
            }

            return TextureColorSpace::Linear;
        }
    }

    template <>
    inline std::unique_ptr<Texture> ResourceLoader<Texture>::Load(const std::string &path)
    {
        auto texture = std::make_unique<Texture>(0, path);
        texture->SetState(ResourceState::Loading);

        int width = 0;
        int height = 0;
        int channels = 0;

        const bool isHdr = stbi_is_hdr(path.c_str()) != 0;
        TextureFormat format = TextureFormat::Unknown;

        std::vector<uint8_t> pixelStorage;

        if (isHdr)
        {
            float *pixelData = stbi_loadf(path.c_str(), &width, &height, &channels, 0);
            if (!pixelData)
            {
                throw std::runtime_error("Failed to load HDR texture at path: " + path);
            }

            const size_t elementCount = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(channels);
            const size_t payloadSize = elementCount * sizeof(float);
            pixelStorage.resize(payloadSize);
            std::memcpy(pixelStorage.data(), pixelData, payloadSize);
            stbi_image_free(pixelData);

            format = detail::TextureFormatFromChannels(channels, true);
        }
        else
        {
            stbi_uc *pixelData = stbi_load(path.c_str(), &width, &height, &channels, 0);
            if (!pixelData)
            {
                throw std::runtime_error("Failed to load texture at path: " + path);
            }

            const size_t payloadSize = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(channels);
            pixelStorage.resize(payloadSize);
            std::memcpy(pixelStorage.data(), pixelData, payloadSize);
            stbi_image_free(pixelData);

            format = detail::TextureFormatFromChannels(channels, false);
        }

        if (format == TextureFormat::Unknown)
        {
            throw std::runtime_error("Unsupported channel layout for texture at path: " + path);
        }

        texture->SetPixels(
            width,
            height,
            channels,
            format,
            detail::InferColorSpace(path),
            std::move(pixelStorage),
            isHdr);

        return texture;
    }
}