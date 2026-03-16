#pragma once

#include <string>

#include <core/scene.hpp>
#include <ecs/world.hpp>
#include <resources/manager.hpp>

class GameScene : public Scene
{
public:
    GameScene(std::string scenePath, ResourceManager *resourceManager);
    ~GameScene() override = default;

    void Init() override;
    void Update(float deltaTime) override;
    void Draw(float deltaTime) override;

private:
    void ReloadScene();
    void ConfigureSystems();

private:
    World m_world;
    ResourceManager *m_resourceManager;
    std::string m_scenePath;
};