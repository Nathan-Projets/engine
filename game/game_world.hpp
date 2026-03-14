#pragma once

#include <memory>

#include "../engine/ecs/world.hpp"
#include "../engine/resources/manager.hpp"
#include "../engine/render/camera/camera_perspective.hpp"
#include "../engine/render/mesh.hpp"
#include "../engine/core/scene.hpp"

class GameWorld : public Scene
{
public:
    GameWorld(int width, int height, ResourceManager *resourceManager);
    ~GameWorld();

    void Init() override;
    void Update(float deltaTime) override;
    void Draw(float deltaTime) override;

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
