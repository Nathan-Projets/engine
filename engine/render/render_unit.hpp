#pragma once

#include <glm/glm.hpp>

#include "shader_domain.hpp"
#include "../resources/units/mesh.hpp"
#include "../resources/units/shader.hpp"
#include "../resources/units/material.hpp"

class RenderUnit
{
public:
    resources::Mesh *resourceMesh = nullptr;
    resources::Shader *resourceShader = nullptr;
    resources::Material *resourceMaterial = nullptr;
    glm::mat4 model = glm::mat4(1.0f);
    MaterialData material = {};
};