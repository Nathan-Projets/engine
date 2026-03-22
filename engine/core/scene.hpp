#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace resources
{
    class ResourceManager;
}

class Scene
{
public:
    virtual ~Scene() = default;

    virtual void Init() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Draw(float deltaTime) = 0;
    virtual void OnResize(int width, int height) {}
    virtual resources::ResourceManager *GetResourceManager() { return nullptr; }

    void Render(float deltaTime)
    {
        Update(deltaTime);
        Draw(deltaTime);
    }
};