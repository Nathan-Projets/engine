#pragma once

#include <core/scene.hpp>
#include <ecs/world.hpp>
#include <resources/handle.hpp>
#include <resources/resource_manager.hpp>

namespace resources
{
    class AudioClip;
}

class RenderSystem;

class SimpleGameScene : public Scene
{
public:
    explicit SimpleGameScene(resources::ResourceManager *resourceManager = nullptr);
    ~SimpleGameScene() override;

    void Init() override;
    void Update(float deltaTime) override;
    void Draw(float deltaTime) override;
    void OnResize(int width, int height) override;
    resources::ResourceManager *GetResourceManager() override { return m_resourceManager; }
    RuntimeUIState GetRuntimeUIState() const override;
    bool SetGamePaused(bool paused) override;
    bool IsGamePaused() const override { return m_gamePaused; }
    bool ResetRuntimeState() override;
    void PresentToScreen() override;

private:
    void BuildScene();
    void ApplyViewportSize(int width, int height);
    void TryStartBackgroundMusic();

private:
    World m_world;
    resources::ResourceManager *m_resourceManager = nullptr;
    RenderSystem *m_renderSystem = nullptr;
    Entity m_cubeEntity;
    resources::Handle<resources::AudioClip> m_musicClip;
    resources::Handle<resources::AudioClip> m_goalOpenClip;
    resources::Handle<resources::AudioClip> m_goalCloseClip;
    int m_viewportWidth = 1280;
    int m_viewportHeight = 720;
    bool m_gamePaused = false;
    bool m_backgroundMusicStarted = false;
};