#include "window.hpp"

#include "platform/platform_backend.hpp"

Window::Window(Handle<Window> id, IPlatformBackend &platform) : m_id(id), m_platform(platform)
{
}

Handle<Window> Window::GetId() const noexcept
{
    return m_id;
}

bool Window::ShouldClose() const
{
    return m_platform.ShouldClose(m_id);
}