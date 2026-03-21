#pragma once

#include <vector>
#include <glfw/glfw3.h>

struct IDebugPanel
{
    virtual ~IDebugPanel() = default;
    virtual void Draw() = 0;
    bool visible = true;
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
    std::vector<IDebugPanel *> m_panels;
};