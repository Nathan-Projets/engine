#pragma once

#include <memory>

#include <ecs/world.hpp>
#include <resources/manager.hpp>
#include <render/camera/perspective_projection.hpp>
#include <render/mesh.hpp>
#include <core/scene.hpp>

class GameWorld : public Scene
{
public:
    GameWorld(int width, int height, ResourceManager *resourceManager);
    ~GameWorld();

    void Init() override;
    void Update(float deltaTime) override;
    void Draw(float deltaTime) override;

    World &GetWorld() { return m_world; }
    PerspectiveProjection &GetActiveProjection();
    ResourceManager *GetResourceManager() const { return m_resourceManager; }

private:
    World m_world;
    ResourceManager *m_resourceManager;

    int m_width;
    int m_height;
};
