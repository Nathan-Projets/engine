#pragma once

#include <glm/glm.hpp>

#include <ecs/component.hpp>

class PlayerController : public Component
{
public:
    float moveSpeed = 4.5f;
    float turnSpeedDegrees = 540.0f;
    float cameraDistance = 6.5f;
    float cameraHeight = 3.0f;
    float lookAtHeight = 1.0f;
    glm::vec3 cameraForward = glm::normalize(glm::vec3(-0.15f, -0.3f, -1.0f));
};