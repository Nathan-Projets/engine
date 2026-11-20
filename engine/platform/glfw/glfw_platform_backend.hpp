#pragma once

#include <unordered_map>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include <GLFW/glfw3.h>

#include "platform/platform_backend.hpp"

class GlfwPlatformBackend : public IPlatformBackend
{
public:
    bool Init() override;
    void Shutdown() override;

    void PollEvents() override;
    void SwapBuffers() override;
    bool ShouldClose(Handle<Window> id) const override;

    bool CreateWindow(Handle<Window> id, const WindowConfig &config) override;
    void DestroyWindow(Handle<Window> id) override;

private:
    std::unordered_map<Handle<Window>, GLFWwindow *> m_windows;
};