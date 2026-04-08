#pragma once

#include <vector>

#include <ecs/component.hpp>
#include <physics/physics_backend.hpp>

class PhysicsListener : public Component
{
public:
    struct Event
    {
        PhysicsEventType type = PhysicsEventType::CollisionEnter;
        Entity other;
        glm::vec3 point = glm::vec3(0.0f);
        glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
        bool hasContact = false;
    };

    void ClearFrameEvents()
    {
        frameEvents.clear();
    }

    bool HasEvent(PhysicsEventType type, Entity other = Entity()) const
    {
        for (const Event &event : frameEvents)
        {
            if (event.type != type)
            {
                continue;
            }

            if (!other.IsValid() || event.other == other)
            {
                return true;
            }
        }

        return false;
    }

    const Event *FindFirstEvent(PhysicsEventType type, Entity other = Entity()) const
    {
        for (const Event &event : frameEvents)
        {
            if (event.type != type)
            {
                continue;
            }

            if (!other.IsValid() || event.other == other)
            {
                return &event;
            }
        }

        return nullptr;
    }

    std::vector<Event> frameEvents;
};