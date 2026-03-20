#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <ecs/component.hpp>

class LightOrbitController : public Component
{
public:
    glm::vec3 center = glm::vec3(0.0f);
    glm::vec3 axisRadii = glm::vec3(2.5f, 1.25f, 2.5f);
    glm::vec3 angularSpeeds = glm::vec3(glm::radians(35.0f), glm::radians(55.0f), glm::radians(25.0f));
    glm::vec3 angles = glm::vec3(0.0f, glm::quarter_pi<float>(), glm::half_pi<float>());
    bool initialized = false;
};