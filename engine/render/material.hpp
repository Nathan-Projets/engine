#pragma once

#include "shader.hpp"

/**
 * Minimal material wrapper used by the current render path.
 */
class Material
{
public:
    Material() = default;
    
    void Use();
    Shader *shader = nullptr;
};