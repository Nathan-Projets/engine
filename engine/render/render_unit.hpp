#pragma once

#include <glm/glm.hpp>

#include "mesh.hpp"
#include "material.hpp"

class RenderUnit
{
public:
    Mesh *mesh;
    Material *material;
    glm::mat4 model;
};