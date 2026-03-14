#pragma once

#include "../system.hpp"
#include "../components/transform.hpp"

class PhysicSystem : public System
{
public:
    // TODO: call here physics update when time's right
    void Update(World &world, float deltaTime) override
    {
    }
};
