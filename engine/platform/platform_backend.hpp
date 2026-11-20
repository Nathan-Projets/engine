#pragma once

#include "platform/handle.hpp"
#include "platform/window.hpp"
#include "platform/window_config.hpp"

class IPlatformBackend
{
public:
    virtual ~IPlatformBackend() = default;

    virtual bool Init() = 0;
    virtual void Shutdown() = 0;

    virtual void PollEvents() = 0;
    virtual void SwapBuffers() = 0;
    virtual bool ShouldClose(Handle<Window> id) const = 0;

    virtual bool CreateWindow(Handle<Window> id, const WindowConfig &config) = 0;
    virtual void DestroyWindow(Handle<Window> id) = 0;
};