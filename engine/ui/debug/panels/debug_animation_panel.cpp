#include "debug_animation_panel.hpp"

#include "debug_panel_layout.hpp"

#include <algorithm>
#include <utility>

#include <imgui.h>

void DebugAnimationPanel::SetScene(Scene *scene)
{
    m_scene = scene;
}

void DebugAnimationPanel::SetSnapshot(AnimationDebugSnapshot snapshot)
{
    m_snapshot = std::move(snapshot);
}

void DebugAnimationPanel::Draw()
{
    ImGui::SetNextWindowPos(debug_panels::GetInitialStackedPosition(1), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(520.0f, 460.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.88f);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("Animation Debug", nullptr, windowFlags);

    if (!m_scene)
    {
        ImGui::TextUnformatted("No scene bound.");
        ImGui::End();
        return;
    }

    if (m_snapshot.entries.empty())
    {
        ImGui::TextUnformatted("No animated entities found.");
        ImGui::End();
        return;
    }

    ImGui::Text("Animated entities: %d", static_cast<int>(m_snapshot.entries.size()));
    ImGui::Separator();
    ImGui::BeginChild("animation_debug_entries", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    for (const AnimationDebugEntry &entry : m_snapshot.entries)
    {
        EntityUIState &uiState = m_uiState[entry.entity.GetID()];
        if (entry.availableClips.empty())
        {
            uiState.selectedClipIndex = -1;
        }
        else if (uiState.selectedClipIndex < 0 || uiState.selectedClipIndex >= static_cast<int>(entry.availableClips.size()))
        {
            uiState.selectedClipIndex = entry.activeClipIndex >= 0 ? entry.activeClipIndex : 0;
        }

        ImGui::PushID(static_cast<int>(entry.entity.GetID()));
        ImGui::Text("Entity %u", entry.entity.GetID());
        ImGui::TextWrapped("Model: %s", entry.modelPath.empty() ? "(unknown)" : entry.modelPath.c_str());
        ImGui::Text("Clip: %s", entry.activeClipName.empty() ? "(unresolved)" : entry.activeClipName.c_str());
        ImGui::Text("Time: %.3f / %.3f s", entry.currentTimeSeconds, entry.clipDurationSeconds);
        ImGui::Text("Bones: %d | Channels: %d", static_cast<int>(entry.boneCount), static_cast<int>(entry.channelCount));
        ImGui::Text("Ticks/sec: %.2f | Loop: %s | Paused: %s | Pose valid: %s",
                    entry.ticksPerSecond,
                    entry.loop ? "yes" : "no",
                    entry.paused ? "yes" : "no",
                    entry.poseValid ? "yes" : "no");

        if (entry.transitionActive)
        {
            ImGui::Text("Transition: %s -> %s (%.0f%%, %.2f s)",
                        entry.activeClipName.empty() ? "(unresolved)" : entry.activeClipName.c_str(),
                        entry.targetClipName.empty() ? "(unresolved)" : entry.targetClipName.c_str(),
                        entry.transitionAlpha * 100.0f,
                        entry.transitionDurationSeconds);
            ImGui::Text("Target time: %.3f / %.3f s", entry.targetTimeSeconds, entry.targetClipDurationSeconds);
            ImGui::ProgressBar(entry.transitionAlpha, ImVec2(-1.0f, 0.0f));
        }

        if (!entry.availableClips.empty())
        {
            const int safeSelectedIndex = std::clamp(uiState.selectedClipIndex, 0, static_cast<int>(entry.availableClips.size()) - 1);
            const char *selectedLabel = entry.availableClips[static_cast<size_t>(safeSelectedIndex)].c_str();
            if (ImGui::BeginCombo("Target clip", selectedLabel))
            {
                for (int clipIndex = 0; clipIndex < static_cast<int>(entry.availableClips.size()); ++clipIndex)
                {
                    const bool isSelected = uiState.selectedClipIndex == clipIndex;
                    if (ImGui::Selectable(entry.availableClips[static_cast<size_t>(clipIndex)].c_str(), isSelected))
                    {
                        uiState.selectedClipIndex = clipIndex;
                    }

                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::SliderFloat("Transition seconds", &uiState.transitionDurationSeconds, 0.0f, 2.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            if (ImGui::Button("Play now"))
            {
                m_scene->RequestAnimationTransition(entry.entity, safeSelectedIndex, 0.0f);
            }
            ImGui::SameLine();
            if (ImGui::Button("Blend to clip"))
            {
                m_scene->RequestAnimationTransition(entry.entity, safeSelectedIndex, uiState.transitionDurationSeconds);
            }
        }

        ImGui::Separator();
        ImGui::PopID();
    }

    ImGui::EndChild();
    ImGui::End();
}