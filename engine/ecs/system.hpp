#pragma once

class World;

class System
{
public:
    virtual ~System() = default;

    virtual void Update(World &world, float deltaTime) = 0;
    virtual void Render(World &world, float deltaTime) {}
};
