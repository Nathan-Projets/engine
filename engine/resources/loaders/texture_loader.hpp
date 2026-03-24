#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <stb_image.h>

#include <cctype>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <filesystem>

#include "../units/texture.hpp"
#include "../resource_loader.hpp"

namespace resources
{
    namespace detail
    {
        inline bool TryParseEmbeddedTextureUri(const std::string &path, std::string &outMeshPath, uint32_t &outTextureIndex)
        {
            constexpr const char *prefix = "embedded://";
            if (path.rfind(prefix, 0) != 0)
            {
                return false;
            }

            const std::string payload = path.substr(std::char_traits<char>::length(prefix));
            const std::string::size_type separator = payload.find_last_of('#');
            if (separator == std::string::npos || separator + 1 >= payload.size())
            {
                return false;
            }

            outMeshPath = payload.substr(0, separator);
            const std::string indexText = payload.substr(separator + 1);
            for (char character : indexText)
            {
                if (!std::isdigit(static_cast<unsigned char>(character)))
                {
                    return false;
                }
            }

            outTextureIndex = static_cast<uint32_t>(std::stoul(indexText));
            return true;
        }

        inline TextureColorSpace InferEmbeddedColorSpace(const std::string &formatHint)
        {
            std::string normalized = formatHint;
            for (char &character : normalized)
            {
                character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
            }

            if (normalized == "png" || normalized == "jpg" || normalized == "jpeg" || normalized == "tga")
            {
                return TextureColorSpace::SRGB;
            }

            return TextureColorSpace::Linear;
        }

        inline void LoadEmbeddedTexturePixels(
            const std::string &path,
            int &outWidth,
            int &outHeight,
            int &outChannels,
            TextureFormat &outFormat,
            TextureColorSpace &outColorSpace,
            std::vector<uint8_t> &outPixelStorage,
            bool &outIsHdr)
        {
            std::string meshPath;
            uint32_t textureIndex = 0;
            if (!TryParseEmbeddedTextureUri(path, meshPath, textureIndex))
            {
                throw std::runtime_error("Invalid embedded texture URI: " + path);
            }

            Assimp::Importer importer;
            const aiScene *scene = importer.ReadFile(meshPath, aiProcess_ValidateDataStructure);
            if (!scene)
            {
                throw std::runtime_error("Failed to reload mesh for embedded texture: " + meshPath);
            }

            if (textureIndex >= scene->mNumTextures)
            {
                throw std::runtime_error("Embedded texture index out of range for mesh: " + meshPath);
            }

            const aiTexture *embeddedTexture = scene->mTextures[textureIndex];
            if (!embeddedTexture)
            {
                throw std::runtime_error("Embedded texture data is null for mesh: " + meshPath);
            }

            outIsHdr = false;

            if (embeddedTexture->mHeight == 0)
            {
                int width = 0;
                int height = 0;
                int channels = 0;
                const unsigned char *compressedBytes = reinterpret_cast<const unsigned char *>(embeddedTexture->pcData);
                stbi_uc *pixelData = stbi_load_from_memory(
                    compressedBytes,
                    static_cast<int>(embeddedTexture->mWidth),
                    &width,
                    &height,
                    &channels,
                    0);

                if (!pixelData)
                {
                    throw std::runtime_error("Failed to decode embedded texture bytes for mesh: " + meshPath);
                }

                const size_t payloadSize = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(channels);
                outPixelStorage.resize(payloadSize);
                std::memcpy(outPixelStorage.data(), pixelData, payloadSize);
                stbi_image_free(pixelData);

                outWidth = width;
                outHeight = height;
                outChannels = channels;
                switch (channels)
                {
                case 1:
                    outFormat = TextureFormat::R8;
                    break;
                case 2:
                    outFormat = TextureFormat::RG8;
                    break;
                case 3:
                    outFormat = TextureFormat::RGB8;
                    break;
                case 4:
                    outFormat = TextureFormat::RGBA8;
                    break;
                default:
                    outFormat = TextureFormat::Unknown;
                    break;
                }
                outColorSpace = InferEmbeddedColorSpace(embeddedTexture->achFormatHint);
                return;
            }

            outWidth = static_cast<int>(embeddedTexture->mWidth);
            outHeight = static_cast<int>(embeddedTexture->mHeight);
            outChannels = 4;
            outFormat = TextureFormat::RGBA8;
            outColorSpace = TextureColorSpace::SRGB;

            const size_t texelCount = static_cast<size_t>(embeddedTexture->mWidth) * static_cast<size_t>(embeddedTexture->mHeight);
            outPixelStorage.resize(texelCount * 4);

            for (size_t index = 0; index < texelCount; ++index)
            {
                const aiTexel &source = embeddedTexture->pcData[index];
                const size_t outputOffset = index * 4;
                outPixelStorage[outputOffset + 0] = source.r;
                outPixelStorage[outputOffset + 1] = source.g;
                outPixelStorage[outputOffset + 2] = source.b;
                outPixelStorage[outputOffset + 3] = source.a;
            }
        }

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

        inline bool ResolveCubemapFacePaths(const std::string &directoryPath, std::array<std::string, 6> &outFacePaths)
        {
            const std::filesystem::path directory(directoryPath);
            if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
            {
                return false;
            }

            auto resolveFace = [&](size_t index, const std::vector<std::string> &candidates) -> bool
            {
                for (const std::string &candidate : candidates)
                {
                    const std::filesystem::path facePath = directory / candidate;
                    if (std::filesystem::exists(facePath) && std::filesystem::is_regular_file(facePath))
                    {
                        outFacePaths[index] = facePath.string();
                        return true;
                    }
                }
                return false;
            };

            const bool okRight = resolveFace(0, {"right.jpg", "right.png", "right.hdr", "posx.jpg", "posx.png", "px.jpg", "px.png"});
            const bool okLeft = resolveFace(1, {"left.jpg", "left.png", "left.hdr", "negx.jpg", "negx.png", "nx.jpg", "nx.png"});
            const bool okTop = resolveFace(2, {"top.jpg", "top.png", "top.hdr", "posy.jpg", "posy.png", "py.jpg", "py.png"});
            const bool okBottom = resolveFace(3, {"bottom.jpg", "bottom.png", "bottom.hdr", "negy.jpg", "negy.png", "ny.jpg", "ny.png"});
            const bool okFront = resolveFace(4, {"front.jpg", "front.png", "front.hdr", "posz.jpg", "posz.png", "pz.jpg", "pz.png"});
            const bool okBack = resolveFace(5, {"back.jpg", "back.png", "back.hdr", "negz.jpg", "negz.png", "nz.jpg", "nz.png"});

            return okRight && okLeft && okTop && okBottom && okFront && okBack;
        }
    }

    template <>
    inline std::unique_ptr<Texture> ResourceLoader<Texture>::Load(const std::string &path)
    {
        auto texture = std::make_unique<Texture>(0, path);
        texture->SetState(ResourceState::Loading);

        std::array<std::string, 6> cubemapFacePaths = {};
        if (detail::ResolveCubemapFacePaths(path, cubemapFacePaths))
        {
            texture->SetTextureType(TextureType::Cubemap);
            texture->SetCubemapFacePaths(std::move(cubemapFacePaths));
            return texture;
        }

        int width = 0;
        int height = 0;
        int channels = 0;

        bool isHdr = stbi_is_hdr(path.c_str()) != 0;
        TextureFormat format = TextureFormat::Unknown;
        TextureColorSpace colorSpace = TextureColorSpace::Linear;

        std::vector<uint8_t> pixelStorage;

        std::string embeddedMeshPath;
        uint32_t embeddedTextureIndex = 0;
        if (detail::TryParseEmbeddedTextureUri(path, embeddedMeshPath, embeddedTextureIndex))
        {
            detail::LoadEmbeddedTexturePixels(path, width, height, channels, format, colorSpace, pixelStorage, isHdr);
        }
        else if (isHdr)
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
            colorSpace = detail::InferColorSpace(path);
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
            colorSpace = detail::InferColorSpace(path);
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
            colorSpace,
            std::move(pixelStorage),
            isHdr);

        return texture;
    }
}