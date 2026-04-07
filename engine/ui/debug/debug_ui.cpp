#include "debug_ui.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

namespace
{
    constexpr const char *kDockedWindowNames[] = {
        "Outliner",
        "Scene Files",
        "Inspector",
        "Viewport",
        "Debug Stats",
        "Animation Debug",
        "Resource Loading"};
}

void DebugUI::Init(GLFWwindow *window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void DebugUI::Shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void DebugUI::BeginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    DrawDockspace();
}

void DebugUI::EndFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DebugUI::AddPanel(IDebugPanel *panel)
{
    m_panels.push_back(panel);
}

void DebugUI::DrawPanels()
{
    for (IDebugPanel *panel : m_panels)
    {
        if (panel && panel->visible)
            panel->Draw();
    }
}

void DebugUI::DrawDockspace()
{
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    constexpr ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("EditorDockspaceHost", nullptr, hostFlags);
    ImGui::PopStyleVar(3);

    const ImGuiID dockspaceId = ImGui::GetID("EditorDockspace");
    EnsureDefaultDockLayout(dockspaceId);
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();
}

void DebugUI::EnsureDefaultDockLayout(ImGuiID dockspaceId)
{
    if (m_defaultDockLayoutBuilt)
    {
        return;
    }

    m_defaultDockLayoutBuilt = true;
    ResetDockedWindowSettings();
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

    ImGuiID rightDockId = 0;
    ImGuiID leftDockId = 0;
    ImGuiID centerDockId = dockspaceId;
    rightDockId = ImGui::DockBuilderSplitNode(centerDockId, ImGuiDir_Right, 0.30f, nullptr, &centerDockId);
    leftDockId = ImGui::DockBuilderSplitNode(centerDockId, ImGuiDir_Left, 0.50f, nullptr, &centerDockId);
    ImGuiID leftBottomDockId = ImGui::DockBuilderSplitNode(leftDockId, ImGuiDir_Down, 0.34f, nullptr, &leftDockId);
    ImGuiID bottomCenterDockId = ImGui::DockBuilderSplitNode(centerDockId, ImGuiDir_Down, 0.28f, nullptr, &centerDockId);

    ImGui::DockBuilderDockWindow("Outliner", leftDockId);
    ImGui::DockBuilderDockWindow("Scene Files", leftBottomDockId);
    ImGui::DockBuilderDockWindow("Inspector", rightDockId);
    ImGui::DockBuilderDockWindow("Viewport", centerDockId);
    ImGui::DockBuilderDockWindow("Debug Stats", bottomCenterDockId);
    ImGui::DockBuilderDockWindow("Animation Debug", bottomCenterDockId);
    ImGui::DockBuilderDockWindow("Resource Loading", bottomCenterDockId);
    ImGui::DockBuilderFinish(dockspaceId);
}

void DebugUI::ResetDockedWindowSettings()
{
    for (const char *windowName : kDockedWindowNames)
    {
        ImGui::ClearWindowSettings(windowName);
    }
}
