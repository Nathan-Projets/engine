#include "debug_stats_panel.hpp"

#include <imgui.h>

void DebugStatsPanel::SetStats(const FrameStats &stats)
{
    m_stats = stats;
}

void DebugStatsPanel::Draw()
{
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("Debug Stats", nullptr, overlayFlags);
    ImGui::Text("FPS: %d", m_stats.fps);
    ImGui::Text("Frame time: %.2f ms", m_stats.frameTimeMs);
    ImGui::Separator();
    ImGui::Text("Uptime: %.1f s", m_stats.uptimeSeconds);
    ImGui::Text("Frame: %llu", static_cast<unsigned long long>(m_stats.totalFrames));
    ImGui::Text("Viewport: %d x %d", m_stats.viewportWidth, m_stats.viewportHeight);
    ImGui::Text("Mouse: (%.1f, %.1f)", m_stats.mousePosition.x, m_stats.mousePosition.y);
    ImGui::Text("Scene bound: %s", m_stats.sceneLoaded ? "yes" : "no");
    ImGui::Text("Toggle panel: F1");
    ImGui::End();
}
