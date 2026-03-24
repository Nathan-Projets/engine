#pragma once

#include <simdjson.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <string>

#include "../units/material.hpp"
#include "../resource_loader.hpp"

namespace resources
{
    namespace detail
    {
        inline std::string ReadMaterialFile(const std::string &path)
        {
            std::ifstream file(path);
            if (!file.is_open())
            {
                throw std::runtime_error("Unable to open material file: " + path);
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }

        inline MaterialTextureSlot TextureSlotFromName(const std::string_view name)
        {
            if (name == "baseColor" || name == "albedo")
            {
                return MaterialTextureSlot::BaseColor;
            }
            if (name == "normal")
            {
                return MaterialTextureSlot::Normal;
            }
            if (name == "metallicRoughness" || name == "orm")
            {
                return MaterialTextureSlot::MetallicRoughness;
            }
            if (name == "occlusion")
            {
                return MaterialTextureSlot::Occlusion;
            }
            if (name == "emissive")
            {
                return MaterialTextureSlot::Emissive;
            }
            if (name == "skybox")
            {
                return MaterialTextureSlot::Skybox;
            }

            throw std::runtime_error("Unknown material texture slot: " + std::string(name));
        }

        inline MaterialPropertyValue ParsePropertyValue(const simdjson::dom::element &value)
        {
            const simdjson::dom::element_type valueType = value.type();

            if (valueType == simdjson::dom::element_type::BOOL)
            {
                return static_cast<bool>(bool(value));
            }

            if (valueType == simdjson::dom::element_type::INT64)
            {
                const int64_t signedValue = int64_t(value);
                if (signedValue >= 0)
                {
                    return static_cast<uint32_t>(signedValue);
                }

                return static_cast<int32_t>(signedValue);
            }

            if (valueType == simdjson::dom::element_type::UINT64)
            {
                return static_cast<uint32_t>(uint64_t(value));
            }

            if (valueType == simdjson::dom::element_type::DOUBLE)
            {
                return static_cast<float>(double(value));
            }

            if (valueType == simdjson::dom::element_type::ARRAY)
            {
                simdjson::dom::array array = value.get_array();
                std::vector<float> values;
                values.reserve(4);

                for (const simdjson::dom::element arrayValue : array)
                {
                    values.push_back(static_cast<float>(double(arrayValue)));
                    if (values.size() == 4)
                    {
                        break;
                    }
                }

                if (values.size() == 2)
                {
                    return glm::vec2(values[0], values[1]);
                }
                if (values.size() == 3)
                {
                    return glm::vec3(values[0], values[1], values[2]);
                }
                if (values.size() == 4)
                {
                    return glm::vec4(values[0], values[1], values[2], values[3]);
                }
            }

            throw std::runtime_error("Unsupported material property value type");
        }
    }

    template <>
    inline std::unique_ptr<Material> ResourceLoader<Material>::Load(const std::string &path)
    {
        auto material = std::make_unique<Material>(0, path);
        material->SetState(ResourceState::Loading);

        const std::string json = detail::ReadMaterialFile(path);
        const std::filesystem::path materialDirectory = std::filesystem::path(path).parent_path();

        simdjson::dom::parser parser;
        simdjson::dom::element document = parser.parse(json);

        simdjson::dom::object root = document.get_object();

        simdjson::dom::element shaderElement;
        if (!root.at_key("shader").get(shaderElement))
        {
            const std::filesystem::path shaderPath = materialDirectory / std::string(std::string_view(shaderElement));
            material->SetShaderPath(shaderPath.string());
        }

        simdjson::dom::element doubleSidedElement;
        if (!root.at_key("doubleSided").get(doubleSidedElement))
        {
            material->SetDoubleSided(bool(doubleSidedElement));
        }

        simdjson::dom::element alphaBlendElement;
        if (!root.at_key("alphaBlend").get(alphaBlendElement))
        {
            material->SetAlphaBlend(bool(alphaBlendElement));
        }

        simdjson::dom::element texturesElement;
        if (!root.at_key("textures").get(texturesElement))
        {
            for (const auto [slotNameRaw, textureElement] : simdjson::dom::object(texturesElement))
            {
                const std::string_view slotName = slotNameRaw;
                const std::string_view texturePath = textureElement;

                const std::filesystem::path resolvedPath = materialDirectory / std::string(texturePath);
                material->SetTexturePath(detail::TextureSlotFromName(slotName), resolvedPath.string());
            }
        }

        simdjson::dom::element propertiesElement;
        if (!root.at_key("properties").get(propertiesElement))
        {
            for (const auto [keyRaw, propertyElement] : simdjson::dom::object(propertiesElement))
            {
                const std::string key(keyRaw);
                material->SetProperty(key, detail::ParsePropertyValue(propertyElement));
            }
        }

        return material;
    }
}