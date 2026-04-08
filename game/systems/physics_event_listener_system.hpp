#pragma once

#include <ecs/system.hpp>
#include <ecs/world.hpp>
#include <ecs/systems/physics_system.hpp>

#include "../components/physics_listener.hpp"

class PhysicsEventListenerSystem : public System
{
public:
    void Update(World &world, float deltaTime) override
    {
        (void)deltaTime;

        for (Entity entity : world.GetEntitiesWith<PhysicsListener>())
        {
            if (PhysicsListener *listener = world.GetComponent<PhysicsListener>(entity))
            {
                listener->ClearFrameEvents();
            }
        }

        const PhysicsSystem *physicsSystem = world.GetSystem<PhysicsSystem>();
        if (!physicsSystem)
        {
            return;
        }

        for (const PhysicsEvent &event : physicsSystem->GetPhysicsWorld().GetEvents())
        {
            RouteEvent(world, event.first, event.second, event, false);
            RouteEvent(world, event.second, event.first, event, true);
        }
    }

private:
    static void RouteEvent(World &world, Entity targetEntity, Entity otherEntity, const PhysicsEvent &event, bool flipNormal)
    {
        PhysicsListener *listener = world.GetComponent<PhysicsListener>(targetEntity);
        if (!listener)
        {
            return;
        }

        listener->frameEvents.push_back(PhysicsListener::Event{
            event.type,
            otherEntity,
            event.point,
            flipNormal ? -event.normal : event.normal,
            event.hasContact});
    }
};