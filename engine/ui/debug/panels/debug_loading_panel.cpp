#include "debug_loading_panel.hpp"

#include <algorithm>

#include <imgui.h>

void DebugLoadingPanel::SetResourceManager(resources::ResourceManager *resourceManager)
{
    m_resourceManager = resourceManager;
}

void DebugLoadingPanel::Draw()
{
    ImGui::SetNextWindowPos(ImVec2(10.0f, 190.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("Resource Loading", nullptr, windowFlags);

    if (!m_resourceManager)
    {
        ImGui::TextUnformatted("No resource manager bound.");
        ImGui::End();
        return;
    }

    const resources::ResourceManager::DebugSnapshot snapshot = m_resourceManager->GetDebugSnapshot();

    ImGui::Text("Threads: %d", static_cast<int>(snapshot.threads.size()));
    ImGui::Text("Active: %d", static_cast<int>(snapshot.activeThreadCount));
    ImGui::Text("Queued: %d", static_cast<int>(snapshot.queuedItems.size()));
    ImGui::Text("Pending GPU uploads: %d", static_cast<int>(snapshot.pendingGpuUploadCount));
    ImGui::Text("Pending deletes: %d", static_cast<int>(snapshot.pendingDeleteCount));
    ImGui::Text("Completed jobs: %llu", static_cast<unsigned long long>(snapshot.completedJobCount));
    ImGui::Text("Failed jobs: %llu", static_cast<unsigned long long>(snapshot.failedJobCount));
    ImGui::Separator();

    for (const resources::ResourceManager::DebugThreadState &thread : snapshot.threads)
    {
        ImGui::PushID(static_cast<int>(thread.threadIndex));
        if (thread.active)
        {
            ImGui::Text("Thread %d: %s", static_cast<int>(thread.threadIndex), thread.current.resourceType.c_str());
            ImGui::TextWrapped("%s", thread.current.path.c_str());
            ImGui::Text("Stage: %s", thread.current.stage.c_str());
            ImGui::ProgressBar(thread.current.progress, ImVec2(260.0f, 0.0f));
            ImGui::Text("Elapsed: %.1f ms", thread.current.elapsedMs);
        }
        else
        {
            ImGui::Text("Thread %d: Idle", static_cast<int>(thread.threadIndex));
        }

        ImGui::Separator();
        ImGui::PopID();
    }

    ImGui::Text("Queue items (%d)", static_cast<int>(snapshot.queuedItems.size()));
    ImGui::BeginChild("load_queue", ImVec2(420.0f, 180.0f), true);
    const size_t maxVisibleItems = std::min<size_t>(snapshot.queuedItems.size(), 64);
    for (size_t index = 0; index < maxVisibleItems; ++index)
    {
        const resources::ResourceManager::DebugLoadEntry &entry = snapshot.queuedItems[index];
        ImGui::BulletText("[%s] %s", entry.resourceType.c_str(), entry.path.c_str());
    }

    if (snapshot.queuedItems.size() > maxVisibleItems)
    {
        ImGui::Text("... %d more", static_cast<int>(snapshot.queuedItems.size() - maxVisibleItems));
    }

    ImGui::EndChild();
    ImGui::End();
}