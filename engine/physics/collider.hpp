#pragma once

#include <glm/glm.hpp>

#include "../ecs/component.hpp"

struct PhysicsMaterial
{
    float friction = 0.5f;
    float restitution = 0.0f;
};

class BoxCollider : public Component
{
public:
    glm::vec3 center = glm::vec3(0.0f);
    glm::vec3 halfExtents = glm::vec3(0.5f);
    bool isTrigger = false;
    PhysicsMaterial material;
};

class SphereCollider : public Component
{
public:
    glm::vec3 center = glm::vec3(0.0f);
    float radius = 0.5f;
    bool isTrigger = false;
    PhysicsMaterial material;
};

class CapsuleCollider : public Component
{
public:
    glm::vec3 center = glm::vec3(0.0f);
    float radius = 0.5f;
    float height = 1.0f;
    bool isTrigger = false;
    PhysicsMaterial material;
};