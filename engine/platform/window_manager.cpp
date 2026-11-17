#include "platform/window_manager.hpp"

WindowManager::WindowManager(IPlatformBackend &platform) : m_platform(platform), m_windows(), m_nextId(1)
{
}

Window &WindowManager::CreateWindow(const WindowConfig &config)
{
    const Handle<Window> id = m_nextId;
    m_nextId.id++;

    if (!m_platform.CreateWindow(id, config))
    {
        throw std::runtime_error("Failed to create window");
    }

    auto window = std::make_unique<Window>(id, m_platform);

    Window &result = *window;

    m_windows.emplace(id, std::move(window));

    return result;
}

void WindowManager::DestroyWindow(Handle<Window> id)
{
    auto windowIt = m_windows.find(id);

    if (windowIt == m_windows.end())
        return;

    m_platform.DestroyWindow(id);
    m_windows.erase(windowIt);
}
