#include "glfw_platform_backend.hpp"

bool GlfwPlatformBackend::Init()
{
    return glfwInit() == GLFW_TRUE;
}

void GlfwPlatformBackend::Shutdown()
{
    for (auto &[id, window] : m_windows)
    {
        glfwDestroyWindow(window);
    }

    m_windows.clear();

    glfwTerminate();
}

void GlfwPlatformBackend::PollEvents()
{
    glfwPollEvents();
}

bool GlfwPlatformBackend::ShouldClose(Handle<Window> id) const
{
    auto windowIt = m_windows.find(id);

    if (windowIt == m_windows.end())
        return true;

    return glfwWindowShouldClose(windowIt->second);
}

bool GlfwPlatformBackend::CreateWindow(Handle<Window> id, const WindowConfig &config)
{
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);

    GLFWwindow *window = glfwCreateWindow(
        static_cast<int>(config.width),
        static_cast<int>(config.height),
        config.title.c_str(),
        nullptr,
        nullptr);

    if (!window)
    {
        return false;
    }

    m_windows.emplace(id, window);

    return true;
}

void GlfwPlatformBackend::DestroyWindow(Handle<Window> id)
{
    auto windowIt = m_windows.find(id);

    if (windowIt == m_windows.end())
        return;

    glfwDestroyWindow(windowIt->second);
    m_windows.erase(windowIt);
}
