#pragma once

#include <vector>

#include <imgui.h>

#include <glad/glad.h>
#include <glfw/glfw3.h>

struct IDebugPanel
{
    virtual ~IDebugPanel() = default;
    virtual void Draw() = 0;
    bool visible = false;
};

class DebugUI
{
public:
    void Init(GLFWwindow *window);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void AddPanel(IDebugPanel *panel);
    void DrawPanels();

private:
    void DrawDockspace();
    void EnsureDefaultDockLayout(ImGuiID dockspaceId);
    void ResetDockedWindowSettings();

private:
    std::vector<IDebugPanel *> m_panels;
    bool m_defaultDockLayoutBuilt = false;
};