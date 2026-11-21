#include "core/engine.hpp"

#include <chrono>
#include <stdexcept>

#include "core/application.hpp"
#include "helpers/log.hpp"
#include "platform/glfw/glfw_platform_backend.hpp"
#include "renderer/gl/gl_renderer_backend.hpp"

Engine::Engine() : m_platform(std::make_unique<GlfwPlatformBackend>()),
                   m_renderer(std::make_unique<Renderer>(std::make_unique<GlRendererBackend>(*m_platform))),
                   m_windowManager(std::make_unique<WindowManager>(*m_platform)),
                   m_initialized(false),
                   m_running(false)
{
}

Engine::~Engine()
{
    Shutdown();
}

void Engine::Run(Application &application)
{
    try
    {
        Init(application);

        auto previousTime = std::chrono::steady_clock::now();
        m_running = application.IsRunning();

        while (m_running)
        {
            auto currentTime = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - previousTime).count();
            previousTime = currentTime;

            PollEvents();

            Update(application, deltaTime);

            m_renderer->Clear();
            m_platform->SwapBuffers();

            m_running = application.IsRunning();
        }

        application.OnShutdown();
    }
    catch (...)
    {
        application.OnShutdown();
        Shutdown();
    }

    Shutdown();
}

WindowManager &Engine::GetWindowManager()
{
    return *m_windowManager;
}

void Engine::Init(Application &application)
{
    if (!m_platform->Init())
    {
        throw std::runtime_error("Platform initialization failed");
    }

    application.OnInit();

    if (!m_renderer->Init())
    {
        throw std::runtime_error("Renderer initialization failed");
    }

    m_initialized = true;
}

void Engine::Update(Application &application, float deltaTime)
{
    application.OnUpdate(deltaTime);
}

void Engine::Shutdown()
{
    if (!m_initialized)
        return;

    m_windowManager->Shutdown();
    m_platform->Shutdown();
    m_initialized = false;
    m_running = false;
}

void Engine::PollEvents()
{
    m_platform->PollEvents();
}
