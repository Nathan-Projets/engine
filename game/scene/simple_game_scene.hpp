#pragma once

#include <core/scene.hpp>
#include <ecs/world.hpp>
#include <resources/resource_manager.hpp>

class RenderSystem;

class SimpleGameScene : public Scene
{
public:
    explicit SimpleGameScene(resources::ResourceManager *resourceManager = nullptr);
    ~SimpleGameScene() override = default;

    void Init() override;
    void Update(float deltaTime) override;
    void Draw(float deltaTime) override;
    void OnResize(int width, int height) override;
    resources::ResourceManager *GetResourceManager() override { return m_resourceManager; }
    void PresentToScreen() override;

private:
    void BuildScene();
    void ApplyViewportSize(int width, int height);

private:
    World m_world;
    resources::ResourceManager *m_resourceManager = nullptr;
    RenderSystem *m_renderSystem = nullptr;
    Entity m_cubeEntity;
    int m_viewportWidth = 1280;
    int m_viewportHeight = 720;
};