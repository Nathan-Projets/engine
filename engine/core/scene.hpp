#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Scene
{
public:
    virtual ~Scene() = default;

    virtual void Init() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Draw(float deltaTime) = 0;

    void Render(float deltaTime)
    {
        Update(deltaTime);
        Draw(deltaTime);
    }
};