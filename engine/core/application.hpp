#pragma once

#include <print>
#include <chrono>
#include <vector>
#include <memory>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "scene.hpp"
#include "scene_backpack.hpp"
#include "scene_load_testing.hpp"
#include "render/shader.hpp"
#include "render/camera/camera_perspective.hpp"
#include "render/model.hpp"
#include "resources/manager.hpp"
#include "helpers/log.hpp"

class Application
{
public:
    Application(int width = 800, int height = 800);
    ~Application();

    bool Init();
    bool Run();
    void Stop();

    static void ResizeCallback(GLFWwindow *window, int width, int height);

private:
    GLFWwindow *m_window;
    int m_width, m_height;
    bool m_bShouldExit;

    std::vector<std::unique_ptr<Scene>> m_scenes;
};