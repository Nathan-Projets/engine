#pragma once

#include <print>
#include <chrono>
#include <vector>
#include <memory>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "scene.hpp"
#include "helpers/log.hpp"
#include "input/input_manager.hpp"
#include "ui/debug/debug_ui.hpp"
#include "ui/debug/panels/debug_stats_panel.hpp"

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

    DebugUI m_debugUI;
    DebugStatsPanel m_statsPanel;

    std::vector<std::unique_ptr<Scene>> m_scenes;
};