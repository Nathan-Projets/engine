#pragma once

#include <memory>
#include <unordered_map>
#include <stdexcept>

#include "platform/platform_backend.hpp"
#include "platform/window.hpp"
#include "platform/window_config.hpp"
#include "platform/handle.hpp"

class WindowManager
{
public:
    explicit WindowManager(IPlatformBackend &platform);

    Window &CreateWindow(const WindowConfig &config);
    void DestroyWindow(Handle<Window> id);
    void Shutdown();

private:
    IPlatformBackend &m_platform;

    std::unordered_map<Handle<Window>, std::unique_ptr<Window>> m_windows;

    Handle<Window> m_nextId;
};