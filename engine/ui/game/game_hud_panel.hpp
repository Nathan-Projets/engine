#pragma once

#include <functional>

#include <imgui.h>

#include "../debug/debug_ui.hpp"
#include "../../core/scene.hpp"

class GameHUDPanel : public IDebugPanel
{
public:
    void SetScene(Scene *scene)
    {
        m_scene = scene;
    }

    void SetQuitCallback(std::function<void()> callback)
    {
        m_quitCallback = std::move(callback);
    }

    void Draw() override
    {
        if (!m_scene)
        {
            return;
        }

        const RuntimeUIState state = m_scene->GetRuntimeUIState();
        if (!state.valid)
        {
            return;
        }

        DrawHUD(state);
        if (state.paused)
        {
            DrawPauseMenu();
        }
    }

private:
    void DrawHUD(const RuntimeUIState &state)
    {
        constexpr ImGuiWindowFlags hudFlags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav;

        ImGui::SetNextWindowBgAlpha(0.35f);
        ImGui::SetNextWindowPos(ImVec2(18.0f, 18.0f), ImGuiCond_Always);
        ImGui::Begin("GameHUD", nullptr, hudFlags);
        if (!state.title.empty())
        {
            ImGui::TextUnformatted(state.title.c_str());
            ImGui::Separator();
        }

        if (!state.objective.empty())
        {
            ImGui::TextWrapped("Objective: %s", state.objective.c_str());
        }
        if (!state.status.empty())
        {
            ImVec4 color = state.objectiveCompleted ? ImVec4(0.45f, 1.0f, 0.55f, 1.0f) : ImVec4(1.0f, 0.85f, 0.45f, 1.0f);
            ImGui::TextColored(color, "%s", state.status.c_str());
        }
        if (!state.hint.empty())
        {
            ImGui::Separator();
            ImGui::TextWrapped("%s", state.hint.c_str());
        }
        ImGui::End();
    }

    void DrawPauseMenu()
    {
        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        const ImVec2 menuSize(320.0f, 180.0f);
        const ImVec2 menuPos(
            viewport->Pos.x + (viewport->Size.x - menuSize.x) * 0.5f,
            viewport->Pos.y + (viewport->Size.y - menuSize.y) * 0.5f);

        constexpr ImGuiWindowFlags menuFlags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoCollapse;

        ImGui::SetNextWindowPos(menuPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(menuSize, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.9f);
        ImGui::Begin("PauseMenu", nullptr, menuFlags);
        ImGui::TextUnformatted("Paused");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Resume", ImVec2(-1.0f, 0.0f)))
        {
            m_scene->SetGamePaused(false);
        }
        if (ImGui::Button("Restart", ImVec2(-1.0f, 0.0f)))
        {
            m_scene->ResetRuntimeState();
        }
        if (ImGui::Button("Quit", ImVec2(-1.0f, 0.0f)))
        {
            if (m_quitCallback)
            {
                m_quitCallback();
            }
        }

        ImGui::Spacing();
        ImGui::TextWrapped("ESC toggles the pause menu.");
        ImGui::End();
    }

    Scene *m_scene = nullptr;
    std::function<void()> m_quitCallback;
};
