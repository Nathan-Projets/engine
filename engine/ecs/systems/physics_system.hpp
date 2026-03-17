#pragma once

#include "../system.hpp"
#include "../components/transform.hpp"

class PhysicsSystem : public System
{
public:
    // TODO: call here physics update when time's right
    void Update(World &world, float deltaTime) override
    {
    }
};
