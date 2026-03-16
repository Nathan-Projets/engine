#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <ecs/component.hpp>

class OrbitCameraController : public Component
{
public:
    glm::vec3 target = glm::vec3(0.0f);
    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
    float radius = 9.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float yawSpeed = glm::quarter_pi<float>();
    float pitchSpeed = glm::quarter_pi<float>();
    float maxPitch = glm::radians(75.0f);

    bool orbitInitialized = false;
};