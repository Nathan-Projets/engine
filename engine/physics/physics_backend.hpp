#pragma once

#include <vector>

#include "raycast.hpp"

class World;

enum class PhysicsEventType : unsigned int
{
    CollisionEnter = 0,
    CollisionStay = 1,
    CollisionExit = 2,
    TriggerEnter = 3,
    TriggerStay = 4,
    TriggerExit = 5,
};

struct PhysicsEvent
{
    PhysicsEventType type = PhysicsEventType::CollisionEnter;
    Entity first;
    Entity second;
    glm::vec3 point = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
    bool hasContact = false;
};

class IPhysicsBackend
{
public:
    virtual ~IPhysicsBackend() = default;

    virtual void Step(World &world, const glm::vec3 &gravity, float deltaTime) = 0;
    virtual RaycastResult Raycast(const World &world, const PhysicsRay &ray, float maxDistance) const = 0;
    virtual const std::vector<PhysicsEvent> &GetEvents() const = 0;
};