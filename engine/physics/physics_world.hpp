#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

#include <glm/glm.hpp>

#include "../ecs/components/transform.hpp"
#include "../ecs/world.hpp"
#include "collider.hpp"
#include "physics_backend.hpp"
#include "rigidbody.hpp"

namespace physics
{
    std::unique_ptr<IPhysicsBackend> CreateDefaultPhysicsBackend();

    inline bool IntersectRaySphere(const PhysicsRay &ray, const glm::vec3 &center, float radius, float &distance, glm::vec3 &normal)
    {
        const glm::vec3 offset = ray.origin - center;
        const float a = glm::dot(ray.direction, ray.direction);
        const float b = 2.0f * glm::dot(offset, ray.direction);
        const float c = glm::dot(offset, offset) - radius * radius;
        const float discriminant = b * b - 4.0f * a * c;
        if (discriminant < 0.0f)
        {
            return false;
        }

        const float sqrtDiscriminant = std::sqrt(discriminant);
        const float nearDistance = (-b - sqrtDiscriminant) / (2.0f * a);
        const float farDistance = (-b + sqrtDiscriminant) / (2.0f * a);
        const float hitDistance = nearDistance >= 0.0f ? nearDistance : farDistance;
        if (hitDistance < 0.0f)
        {
            return false;
        }

        distance = hitDistance;
        normal = glm::normalize((ray.origin + ray.direction * hitDistance) - center);
        return true;
    }

    inline bool IntersectRayAabb(const PhysicsRay &ray, const glm::vec3 &boundsMin, const glm::vec3 &boundsMax, float &distance, glm::vec3 &normal)
    {
        float tMin = 0.0f;
        float tMax = std::numeric_limits<float>::max();
        int hitAxis = -1;
        for (int axis = 0; axis < 3; ++axis)
        {
            const float direction = ray.direction[axis];
            if (std::abs(direction) < 0.00001f)
            {
                if (ray.origin[axis] < boundsMin[axis] || ray.origin[axis] > boundsMax[axis])
                {
                    return false;
                }
                continue;
            }

            const float inverseDirection = 1.0f / direction;
            float t0 = (boundsMin[axis] - ray.origin[axis]) * inverseDirection;
            float t1 = (boundsMax[axis] - ray.origin[axis]) * inverseDirection;
            if (t0 > t1)
            {
                std::swap(t0, t1);
            }

            if (t0 > tMin)
            {
                tMin = t0;
                hitAxis = axis;
            }

            tMax = std::min(tMax, t1);
            if (tMax < tMin)
            {
                return false;
            }
        }

        distance = tMin;
        normal = glm::vec3(0.0f);
        if (hitAxis >= 0)
        {
            normal[hitAxis] = ray.direction[hitAxis] > 0.0f ? -1.0f : 1.0f;
        }
        return true;
    }

    class SimplePhysicsBackend : public IPhysicsBackend
    {
    public:
        void Step(World &world, const glm::vec3 &gravity, float deltaTime) override
        {
            if (deltaTime <= 0.0f)
            {
                return;
            }

            for (Entity entity : world.GetEntitiesWith<Transform, Rigidbody>())
            {
                Transform *transform = world.GetComponent<Transform>(entity);
                Rigidbody *rigidbody = world.GetComponent<Rigidbody>(entity);
                if (!transform || !rigidbody)
                {
                    continue;
                }

                if (rigidbody->type != RigidbodyType::Dynamic)
                {
                    continue;
                }

                if (rigidbody->useGravity)
                {
                    rigidbody->linearVelocity += gravity * rigidbody->gravityScale * deltaTime;
                }

                const float dampingFactor = std::clamp(1.0f - rigidbody->linearDamping * deltaTime, 0.0f, 1.0f);
                rigidbody->linearVelocity *= dampingFactor;
                transform->position += rigidbody->linearVelocity * deltaTime;
                if (!rigidbody->lockRotation)
                {
                    transform->rotation += rigidbody->angularVelocity * deltaTime;
                }
            }
        }

        RaycastResult Raycast(const World &world, const PhysicsRay &ray, float maxDistance) const override
        {
            RaycastResult closestHit;
            float closestDistance = maxDistance;

            for (Entity entity : world.GetEntitiesWith<Transform>())
            {
                const Transform *transform = world.GetComponent<Transform>(entity);
                if (!transform)
                {
                    continue;
                }

                float hitDistance = 0.0f;
                glm::vec3 hitNormal(0.0f, 1.0f, 0.0f);
                bool hit = false;

                if (const SphereCollider *sphere = world.GetComponent<SphereCollider>(entity))
                {
                    const float radius = sphere->radius * std::max({std::abs(transform->scale.x), std::abs(transform->scale.y), std::abs(transform->scale.z), 1.0f});
                    hit = IntersectRaySphere(ray, transform->position + sphere->center, radius, hitDistance, hitNormal);
                }
                else if (const BoxCollider *box = world.GetComponent<BoxCollider>(entity))
                {
                    const glm::vec3 scaledHalfExtents = box->halfExtents * glm::abs(transform->scale);
                    const glm::vec3 center = transform->position + box->center;
                    hit = IntersectRayAabb(ray, center - scaledHalfExtents, center + scaledHalfExtents, hitDistance, hitNormal);
                }

                if (!hit || hitDistance > closestDistance)
                {
                    continue;
                }

                closestDistance = hitDistance;
                closestHit = RaycastHit{entity, ray.origin + ray.direction * hitDistance, hitNormal, hitDistance};
            }

            return closestHit;
        }

        const std::vector<PhysicsEvent> &GetEvents() const override
        {
            return m_events;
        }

    private:
        std::vector<PhysicsEvent> m_events;
    };
}

class PhysicsWorld
{
public:
    PhysicsWorld() : m_backend(physics::CreateDefaultPhysicsBackend()) {}

    void SetGravity(const glm::vec3 &gravity)
    {
        m_gravity = gravity;
    }

    const glm::vec3 &GetGravity() const
    {
        return m_gravity;
    }

    void SetBackend(std::unique_ptr<IPhysicsBackend> backend)
    {
        if (backend)
        {
            m_backend = std::move(backend);
        }
    }

    IPhysicsBackend *GetBackend() const
    {
        return m_backend.get();
    }

    void Step(World &world, float deltaTime)
    {
        if (m_backend)
        {
            m_backend->Step(world, m_gravity, deltaTime);
        }
    }

    RaycastResult Raycast(const World &world, const PhysicsRay &ray, float maxDistance = 1000.0f) const
    {
        if (!m_backend)
        {
            return std::nullopt;
        }

        return m_backend->Raycast(world, ray, maxDistance);
    }

    const std::vector<PhysicsEvent> &GetEvents() const
    {
        static const std::vector<PhysicsEvent> emptyEvents;
        return m_backend ? m_backend->GetEvents() : emptyEvents;
    }

private:
    glm::vec3 m_gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    std::unique_ptr<IPhysicsBackend> m_backend;
};