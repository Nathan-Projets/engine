#include "debug_loading_panel.hpp"

#include <algorithm>

#include <imgui.h>

void DebugLoadingPanel::SetResourceManager(resources::ResourceManager *resourceManager)
{
    m_resourceManager = resourceManager;
}

void DebugLoadingPanel::Draw()
{
    ImGui::SetNextWindowPos(ImVec2(10.0f, 190.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(760.0f, 560.0f), ImGuiCond_FirstUseEver);

    ImGuiIO &io = ImGui::GetIO();
    const float maxWidth = std::max(520.0f, io.DisplaySize.x - 20.0f);
    const float maxHeight = std::max(280.0f, io.DisplaySize.y - 20.0f);
    ImGui::SetNextWindowSizeConstraints(ImVec2(420.0f, 260.0f), ImVec2(maxWidth, maxHeight));

    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("Resource Loading", nullptr, windowFlags);

    if (!m_resourceManager)
    {
        ImGui::TextUnformatted("No resource manager bound.");
        ImGui::End();
        return;
    }

    const resources::ResourceManager::DebugSnapshot snapshot = m_resourceManager->GetDebugSnapshot();

    ImGui::BeginChild("load_panel_content", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    if (ImGui::Button("Clear Load History"))
    {
        m_resourceManager->ClearDebugLoadHistory();
    }

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
    ImGui::BeginChild("load_queue", ImVec2(0.0f, 180.0f), true);
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

    ImGui::Separator();
    if (ImGui::CollapsingHeader(("Recent load history (" + std::to_string(snapshot.recentHistory.size()) + ")").c_str()))
    {
        ImGui::BeginChild("load_history", ImVec2(0.0f, 170.0f), true);
        const size_t maxHistoryItems = std::min<size_t>(snapshot.recentHistory.size(), 64);
        for (size_t index = 0; index < maxHistoryItems; ++index)
        {
            const resources::ResourceManager::DebugLoadEntry &entry = snapshot.recentHistory[index];
            ImGui::Text("[%s] %.1f ms", entry.resourceType.c_str(), entry.elapsedMs);
            ImGui::TextWrapped("%s", entry.path.c_str());
            ImGui::Separator();
        }

        if (snapshot.recentHistory.size() > maxHistoryItems)
        {
            ImGui::Text("... %d more", static_cast<int>(snapshot.recentHistory.size() - maxHistoryItems));
        }
        ImGui::EndChild();
    }

    ImGui::Separator();
    if (ImGui::CollapsingHeader(("Aggregate timings (" + std::to_string(snapshot.aggregateStats.size()) + ")").c_str()))
    {
        std::vector<resources::ResourceManager::DebugLoadAggregate> aggregateStats = snapshot.aggregateStats;
        std::sort(
            aggregateStats.begin(),
            aggregateStats.end(),
            [](const resources::ResourceManager::DebugLoadAggregate &a, const resources::ResourceManager::DebugLoadAggregate &b)
            {
                return a.avgMs > b.avgMs;
            });

        ImGui::BeginChild("load_aggregates", ImVec2(0.0f, 220.0f), true);
        const size_t maxAggregateItems = std::min<size_t>(aggregateStats.size(), 40);
        for (size_t index = 0; index < maxAggregateItems; ++index)
        {
            const resources::ResourceManager::DebugLoadAggregate &entry = aggregateStats[index];
            ImGui::Text(
                "[%s] avg %.1f ms | min %.1f | max %.1f | last %.1f | samples %llu (ok %llu / fail %llu)",
                entry.resourceType.c_str(),
                entry.avgMs,
                entry.minMs,
                entry.maxMs,
                entry.lastMs,
                static_cast<unsigned long long>(entry.sampleCount),
                static_cast<unsigned long long>(entry.successCount),
                static_cast<unsigned long long>(entry.failureCount));
            ImGui::TextWrapped("%s", entry.path.c_str());
            ImGui::Separator();
        }

        if (aggregateStats.size() > maxAggregateItems)
        {
            ImGui::Text("... %d more", static_cast<int>(aggregateStats.size() - maxAggregateItems));
        }

        ImGui::EndChild();
    }
    ImGui::EndChild();
    ImGui::End();
}