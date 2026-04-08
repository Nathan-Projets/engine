#pragma once

#include <print>
#include <chrono>
#include <string>
#include <vector>
#include <memory>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "scene.hpp"
#include "helpers/log.hpp"
#include "input/input_manager.hpp"
#include "ui/debug/debug_ui.hpp"
#include "ui/debug/panels/debug_animation_panel.hpp"
#include "ui/debug/panels/debug_loading_panel.hpp"
#include "ui/debug/panels/debug_stats_panel.hpp"
#include "ui/editor/editor_context.hpp"
#include "ui/editor/panels/editor_files_panel.hpp"
#include "ui/editor/panels/editor_inspector_panel.hpp"
#include "ui/editor/panels/editor_outliner_panel.hpp"
#include "ui/editor/panels/editor_viewport_panel.hpp"
#include "ui/game/game_hud_panel.hpp"

class Application
{
public:
    struct Options
    {
        bool enableEditorUI = true;
        bool enableDebugUI = true;
        bool enableGameUI = false;
        std::string windowTitle = "Engine";
    };

    Application(int width = 800, int height = 800, Options options = {});
    ~Application();

    bool Init();
    bool Run();
    void Stop();

    void SetScene(Scene *scene);

    static void ResizeCallback(GLFWwindow *window, int width, int height);

private:
    GLFWwindow *m_window;
    int m_width, m_height;
    bool m_bShouldExit;
    bool m_initialized;
    bool m_debugUIInitialized = false;
    Options m_options;

    Scene* m_scene = nullptr;

    DebugUI m_debugUI;
    EditorContext m_editorContext;
    DebugAnimationPanel m_animationPanel;
    DebugLoadingPanel m_loadingPanel;
    DebugStatsPanel m_statsPanel;
    EditorFilesPanel m_filesPanel;
    EditorOutlinerPanel m_outlinerPanel;
    EditorInspectorPanel m_inspectorPanel;
    EditorViewportPanel m_viewportPanel;
    GameHUDPanel m_gameHUDPanel;
    bool m_debugPanelsVisible = false;

    std::vector<std::unique_ptr<Scene>> m_scenes;
};