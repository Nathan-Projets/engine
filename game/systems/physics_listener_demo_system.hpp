#pragma once

#include <string>

#include <audio/audio_engine.hpp>
#include <ecs/components/light.hpp>
#include <ecs/components/name.hpp>
#include <ecs/components/transform.hpp>
#include <ecs/world.hpp>
#include <helpers/log.hpp>

#include <glm/common.hpp>

#include "../components/physics_listener.hpp"
#include "../components/player_controller.hpp"

class PhysicsListenerDemoSystem : public System
{
public:
    void Update(World &world, float deltaTime) override
    {
        (void)deltaTime;

        Entity playerEntity;
        PhysicsListener *playerListener = nullptr;
        for (Entity entity : world.GetEntitiesWith<PlayerController, PhysicsListener>())
        {
            playerEntity = entity;
            playerListener = world.GetComponent<PhysicsListener>(entity);
            if (playerListener)
            {
                break;
            }
        }

        if (!playerListener)
        {
            SetFillLightActive(world, false);
            UpdateGoalDoor(world, false, deltaTime);
            return;
        }

        bool triggerActive = m_triggerActive;
        for (const PhysicsListener::Event &event : playerListener->frameEvents)
        {
            if (event.type == PhysicsEventType::TriggerEnter)
            {
                triggerActive = true;
            }
            else if (event.type == PhysicsEventType::TriggerExit)
            {
                triggerActive = false;
            }
            else if (event.type == PhysicsEventType::CollisionEnter)
            {
                // INFO("Player collision enter with: " + GetEntityLabel(world, event.other));
            }
        }

        if (triggerActive != m_triggerActive)
        {
            SetFillLightActive(world, triggerActive);
            AudioEngine::Get().PlayOneShot(triggerActive ? "assets/audio/goal_open.wav" : "assets/audio/goal_close.wav", 0.85f);
            m_triggerActive = triggerActive;
        }

        UpdateGoalDoor(world, m_triggerActive, deltaTime);
    }

private:
    static std::string GetEntityLabel(World &world, Entity entity)
    {
        if (const Name *name = world.GetComponent<Name>(entity))
        {
            return name->value;
        }

        return "Entity " + std::to_string(entity.GetID());
    }

    static void SetFillLightActive(World &world, bool active)
    {
        for (Entity entity : world.GetEntitiesWith<Light, Name>())
        {
            const Name *name = world.GetComponent<Name>(entity);
            Light *light = world.GetComponent<Light>(entity);
            if (!name || !light || name->value != "FillLight")
            {
                continue;
            }

            if (active)
            {
                light->color = glm::vec3(0.45f, 1.0f, 0.55f);
                light->diffuse = glm::vec3(0.4f, 0.95f, 0.5f);
                light->specular = glm::vec3(0.55f, 1.0f, 0.7f);
                light->intensity = 4.0f;
            }
            else
            {
                light->color = glm::vec3(0.45f, 0.55f, 1.0f);
                light->diffuse = glm::vec3(0.35f, 0.4f, 0.75f);
                light->specular = glm::vec3(0.4f, 0.45f, 0.8f);
                light->intensity = 2.5f;
            }
            return;
        }
    }

    static void UpdateGoalDoor(World &world, bool open, float deltaTime)
    {
        constexpr float closedY = 0.65f;
        constexpr float openY = 3.2f;
        constexpr float moveSpeed = 2.5f;

        for (Entity entity : world.GetEntitiesWith<Transform, Name>())
        {
            const Name *name = world.GetComponent<Name>(entity);
            Transform *transform = world.GetComponent<Transform>(entity);
            if (!name || !transform || name->value != "GoalDoor")
            {
                continue;
            }

            const float targetY = open ? openY : closedY;
            const float step = std::min(moveSpeed * deltaTime, 1.0f);
            transform->position.y = glm::mix(transform->position.y, targetY, step);
            return;
        }
    }

    bool m_triggerActive = false;
};