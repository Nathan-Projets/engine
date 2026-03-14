#pragma once

#include <glm/glm.hpp>

#include "../component.hpp"

class Light : public Component
{
public:
    Light() : ambient(0.2f, 0.2f, 0.2f),
              diffuse(0.5f, 0.5f, 0.5f),
              specular(0.7f, 0.7f, 0.7f),
              position(0.0f, 0.0f, 0.0f)
    {
    }

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 position;
};
