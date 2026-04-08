#pragma once

#include <optional>

#include <glm/glm.hpp>

#include "../ecs/entity.hpp"

struct PhysicsRay
{
    glm::vec3 origin = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f, 0.0f, -1.0f);
};

struct RaycastHit
{
    Entity entity;
    glm::vec3 point = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
    float distance = 0.0f;
};

using RaycastResult = std::optional<RaycastHit>;