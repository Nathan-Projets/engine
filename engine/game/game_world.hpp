#pragma once

#include <memory>

#include "../ecs/world.hpp"
#include "../resources/manager.hpp"
#include "../render/camera/camera_perspective.hpp"
#include "../render/mesh.hpp"

class GameWorld
{
public:
    GameWorld(int width, int height, ResourceManager *resourceManager);
    ~GameWorld();

    void Init();
    void Update(float deltaTime);
    void Render(float deltaTime);

    World &GetWorld() { return m_world; }
    PerspectiveCamera &GetCamera() { return m_camera; }
    ResourceManager *GetResourceManager() const { return m_resourceManager; }

private:
    World m_world;
    PerspectiveCamera m_camera;
    ResourceManager *m_resourceManager;

    int m_width;
    int m_height;
};
