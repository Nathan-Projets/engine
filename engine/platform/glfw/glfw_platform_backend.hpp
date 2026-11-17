#pragma once

#include <unordered_map>

#include <GLFW/glfw3.h>

#include "platform/platform_backend.hpp"

class GlfwPlatformBackend : public IPlatformBackend
{
public:
    bool Init() override;
    void Shutdown() override;

    void PollEvents() override;
    bool ShouldClose(Handle<Window> id) const override;

    bool CreateWindow(Handle<Window> id, const WindowConfig &config) override;
    void DestroyWindow(Handle<Window> id) override;

private:
    std::unordered_map<Handle<Window>, GLFWwindow *> m_windows;
};