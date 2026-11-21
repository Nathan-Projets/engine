#pragma once

#include <memory>

#include "platform/platform_backend.hpp"
#include "platform/window_manager.hpp"
#include "renderer/renderer.hpp"

class Application;

class Engine
{
public:
    explicit Engine();
    ~Engine();

    void Run(Application &application);

    WindowManager &GetWindowManager();

private:
    void Init(Application &application);
    void Update(Application &application, float deltaTime);
    void Shutdown();
    void PollEvents();

private:
    std::unique_ptr<IPlatformBackend> m_platform;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<WindowManager> m_windowManager;

    bool m_initialized;
    bool m_running;
};