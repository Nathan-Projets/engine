#pragma once

#include <glm/glm.hpp>

#include "../ecs/component.hpp"

enum class RigidbodyType : unsigned int
{
    Static = 0,
    Dynamic = 1,
    Kinematic = 2,
};

class Rigidbody : public Component
{
public:
    RigidbodyType type = RigidbodyType::Dynamic;
    float mass = 1.0f;
    float gravityScale = 1.0f;
    float linearDamping = 0.05f;
    float angularDamping = 0.8f;
    bool useGravity = true;
    bool lockRotation = false;
    bool isTrigger = false;
    glm::vec3 linearVelocity = glm::vec3(0.0f);
    glm::vec3 angularVelocity = glm::vec3(0.0f);
};