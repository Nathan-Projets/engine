#pragma once

#include <array>
#include <cstdint>
#include <limits>

#include <glm/glm.hpp>

#include "shader_domain.hpp"
#include "../resources/units/mesh.hpp"
#include "../resources/units/shader.hpp"
#include "../resources/units/material.hpp"

class RenderUnit
{
public:
    static constexpr size_t TextureSlotCount = 5;

    resources::Mesh *resourceMesh = nullptr;
    resources::Shader *resourceShader = nullptr;
    resources::Material *resourceMaterial = nullptr;
    uint32_t primitiveIndex = std::numeric_limits<uint32_t>::max();
    uint32_t materialIndex = 0;
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 localTransform = glm::mat4(1.0f);
    std::array<resources::Handle<resources::Texture>, TextureSlotCount> textureOverrides = {};
    std::array<uint32_t, TextureSlotCount> textureUvIndices = {};
    MaterialData material = {};
};