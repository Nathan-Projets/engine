
#pragma once

#include "platform/handle.hpp"

class IPlatformBackend;

class Window
{
public:
    Window(Handle<Window> id, IPlatformBackend &platform);

    Handle<Window> GetId() const noexcept;
    bool ShouldClose() const;

private:
    Handle<Window> m_id;
    IPlatformBackend &m_platform;
};