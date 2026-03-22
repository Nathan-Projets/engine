#pragma once

#include <string>

#include <core/scene.hpp>
#include <ecs/world.hpp>
#include <resources/resource_manager.hpp>

class GameScene : public Scene
{
public:
    GameScene(std::string scenePath, resources::ResourceManager *resourceManager = nullptr);
    ~GameScene() override = default;

    void Init() override;
    void Update(float deltaTime) override;
    void Draw(float deltaTime) override;
    void OnResize(int width, int height) override;
    resources::ResourceManager *GetResourceManager() override { return m_resourceManager; }

private:
    void ReloadScene(bool clearResourceCache = false);
    void ConfigureSystems();

private:
    World m_world;
    resources::ResourceManager *m_resourceManager;
    std::string m_scenePath;
    int m_viewportWidth = 0;
    int m_viewportHeight = 0;
};