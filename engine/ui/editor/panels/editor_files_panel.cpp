#include "editor_files_panel.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include <imgui.h>

void EditorFilesPanel::SetContext(EditorContext *context)
{
    m_context = context;
}

void EditorFilesPanel::Draw()
{
    ImGui::SetNextWindowSize(ImVec2(320.0f, 240.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Scene Files");

    if (!m_context || !m_context->GetScene())
    {
        ImGui::TextUnformatted("No scene bound.");
        ImGui::End();
        return;
    }

    Scene *scene = m_context->GetScene();
    const std::string activeScenePath = scene->GetEditorScenePath();
    std::vector<std::string> sceneFiles = scene->GetEditorSceneFiles();
    std::sort(sceneFiles.begin(), sceneFiles.end());

    if (ImGui::Button(m_context->IsPlaying() ? "Stop" : "Play"))
    {
        m_context->TogglePlayMode();
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!m_context->CanUndo());
    if (ImGui::Button("Undo"))
    {
        m_context->Undo();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!m_context->CanRedo());
    if (ImGui::Button("Redo"))
    {
        m_context->Redo();
    }
    ImGui::EndDisabled();

    ImGui::BeginDisabled(m_context->IsPlaying());
    if (ImGui::Button("Save Scene"))
    {
        if (scene->SaveEditorScene())
        {
            m_context->ClearDirty();
        }
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (m_context->IsPlaying())
    {
        ImGui::TextUnformatted("Play mode");
    }
    else
    {
        ImGui::TextUnformatted(m_context->IsDirty() ? "Unsaved changes" : "Saved");
    }
    ImGui::TextWrapped("Active scene: %s%s",
                       activeScenePath.empty() ? "(unknown)" : activeScenePath.c_str(),
                       m_context->IsDirty() ? " *" : "");
    ImGui::Separator();

    if (sceneFiles.empty())
    {
        ImGui::TextUnformatted("No scene files found.");
        ImGui::End();
        return;
    }

    ImGui::BeginChild("scene_file_list", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    for (const std::string &sceneFile : sceneFiles)
    {
        const bool isSelected = sceneFile == activeScenePath;
        ImGui::BeginDisabled(m_context->IsPlaying());
        if (ImGui::Selectable(sceneFile.c_str(), isSelected) && !isSelected)
        {
            if (scene->LoadEditorScene(sceneFile))
            {
                m_context->ClearSelection();
                m_context->ResetHistory();
            }
        }
        ImGui::EndDisabled();
    }
    ImGui::EndChild();

    ImGui::End();
}