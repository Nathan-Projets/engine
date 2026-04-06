#pragma once

#include <glm/glm.hpp>

#include "../component.hpp"

enum class LightType : unsigned int
{
    Point = 0,
    Directional = 1,
    Spot = 2,
};

class Light : public Component
{
public:
    Light() : ambient(0.2f, 0.2f, 0.2f),
              diffuse(0.5f, 0.5f, 0.5f),
              specular(0.7f, 0.7f, 0.7f),
              color(1.0f, 1.0f, 1.0f),
              intensity(1.0f),
              constant(1.0f),
              linear(0.09f),
              quadratic(0.032f),
              direction(0.0f, -1.0f, 0.0f),
              innerCutoff(12.5f),
              outerCutoff(17.5f),
              type(LightType::Point),
              castShadows(true)
    {
    }

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    glm::vec3 direction;
    float innerCutoff;
    float outerCutoff;
    LightType type;
    bool castShadows;
};
