#include "material.hpp"

void Material::Use()
{
    if (!shader)
    {
        WARNING("No shader linked to material.");
        return;
    }

    shader->Use();
}
