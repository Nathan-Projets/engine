#pragma once

#include "../ecs/system.hpp"
#include "../game/components/transform.hpp"

class PhysicSystem : public System
{
public:
    // TODO: call here physics update when time's right
    void Update(World &world, float deltaTime) override
    {
    }
};
