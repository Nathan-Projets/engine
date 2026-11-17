#pragma once

#include <string>
#include <memory>

#include "platform/platform_backend.hpp"
#include "platform/window_manager.hpp"

class Application
{
public:
    Application();
    virtual ~Application();

    virtual void Run();
    virtual void Shutdown();

protected:
    virtual void init();

    std::unique_ptr<IPlatformBackend> m_platform;
    std::unique_ptr<WindowManager> m_windowManager;

    // TODO: window
    // TODO: renderer
    // TODO: asset_manager
    // TODO: input_manager
    // TODO: scene_manager
};