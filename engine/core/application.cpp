#include "application.hpp"

#include "helpers/log.hpp"
#include "platform/glfw/glfw_platform_backend.hpp"

Application::Application()
{
    m_platform = std::make_unique<GlfwPlatformBackend>();
    m_windowManager = std::make_unique<WindowManager>(*m_platform);
}

Application::~Application()
{
}

static Window *testWindow = nullptr;

void Application::Run()
{
    init();

    Window &testWindow = m_windowManager->CreateWindow({.title = "Test", .width = 400, .height = 400});

    while (true)
    {
        if (testWindow.ShouldClose())
        {
            break;
        }
    }
}

void Application::Shutdown()
{
    m_windowManager.reset();
    m_platform->Shutdown();
}

void Application::init()
{
    if (!m_platform->Init())
    {
        throw std::runtime_error("Platform initialization failed");
    }

    DEBUG("Application initialized");
}
