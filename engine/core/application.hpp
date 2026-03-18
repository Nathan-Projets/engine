#pragma once

#include <print>
#include <chrono>
#include <vector>
#include <memory>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "scene.hpp"
#include "render/camera/perspective_projection.hpp"
#include "helpers/log.hpp"
#include "input/input_manager.hpp"

class Application
{
public:
    Application(int width = 800, int height = 800);
    ~Application();

    bool Init();
    bool Run();
    void Stop();

    void SetScene(Scene* scene) { m_scene = scene; }

    static void ResizeCallback(GLFWwindow *window, int width, int height);

private:
    GLFWwindow *m_window;
    int m_width, m_height;
    bool m_bShouldExit;
    bool m_initialized;

    Scene* m_scene = nullptr;

    std::vector<std::unique_ptr<Scene>> m_scenes;
};