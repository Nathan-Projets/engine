#pragma once

#include "../system.hpp"

#include "../../physics/physics_world.hpp"

class PhysicsSystem : public System
{
public:
    PhysicsWorld &GetPhysicsWorld()
    {
        return m_physicsWorld;
    }

    const PhysicsWorld &GetPhysicsWorld() const
    {
        return m_physicsWorld;
    }

    void Update(World &world, float deltaTime) override
    {
        m_physicsWorld.Step(world, deltaTime);
    }

private:
    PhysicsWorld m_physicsWorld;
};
