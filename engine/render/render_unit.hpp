#pragma once

#include <glm/glm.hpp>

#include "mesh.hpp"
#include "shader.hpp"
#include "shader_domain.hpp"

class RenderUnit
{
public:
    Mesh *mesh = nullptr;
    Shader *shader = nullptr;
    glm::mat4 model = glm::mat4(1.0f);
    MaterialData material = {};
};