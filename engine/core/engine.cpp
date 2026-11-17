#include "core/engine.hpp"

#include <chrono>

#include "core/application.hpp"
#include "helpers/log.hpp"
#include "platform/glfw/glfw_platform_backend.hpp"

Engine::Engine()
{
    m_platform = std::make_unique<GlfwPlatformBackend>();
    m_windowManager = std::make_unique<WindowManager>(*m_platform);
    m_initialized = false;
}

Engine::~Engine()
{
    Shutdown();
}

void Engine::Run(Application &application)
{
    Init();

    application.OnInit();

    auto previousTime = std::chrono::steady_clock::now();

    m_running = application.IsRunning();

    while (m_running)
    {
        auto currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - previousTime).count();
        previousTime = currentTime;

        Update(application, deltaTime);

        m_running = application.IsRunning();
    }

    application.OnShutdown();

    Shutdown();
}

WindowManager &Engine::GetWindowManager()
{
    return *m_windowManager;
}

void Engine::Init()
{
    if (!m_platform->Init())
    {
        throw std::runtime_error("Platform initialization failed");
    }
}

void Engine::Update(Application &application, float deltaTime)
{
    application.OnUpdate(deltaTime);

    m_platform->PollEvents();
}

void Engine::Shutdown()
{
    m_windowManager.reset();
    m_platform->Shutdown();
}

void Engine::PollEvents()
{
    m_platform->PollEvents();
}
