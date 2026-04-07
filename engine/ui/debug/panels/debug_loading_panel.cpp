#include "debug_loading_panel.hpp"

#include "debug_panel_layout.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include <imgui.h>

void DebugLoadingPanel::SetResourceManager(resources::ResourceManager *resourceManager)
{
    m_resourceManager = resourceManager;
}

void DebugLoadingPanel::Draw()
{
    ImGui::SetNextWindowPos(debug_panels::GetInitialStackedPosition(2), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(760.0f, 300.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);

    ImGuiIO &io = ImGui::GetIO();
    const float maxWidth = std::max(520.0f, io.DisplaySize.x - 20.0f);
    const float maxHeight = std::max(220.0f, io.DisplaySize.y * 0.45f);
    ImGui::SetNextWindowSizeConstraints(ImVec2(420.0f, 180.0f), ImVec2(maxWidth, maxHeight));

    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;

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
        size_t renderedRows = 0;
        const size_t maxHistoryRows = 64;
        for (size_t index = 0; index < snapshot.recentHistory.size() && renderedRows < maxHistoryRows;)
        {
            const resources::ResourceManager::DebugLoadEntry &entry = snapshot.recentHistory[index];

            if (entry.jobId != 0 && entry.stage != "Completed" && entry.stage != "Failed")
            {
                ++index;
                continue;
            }

            if (entry.jobId != 0 && (entry.stage == "Completed" || entry.stage == "Failed"))
            {
                ImGui::Text("[%s] %.1f ms", entry.resourceType.c_str(), entry.elapsedMs);
                ImGui::SameLine();
                ImGui::TextDisabled("%s", entry.stage.c_str());
                ImGui::TextWrapped("%s", entry.path.c_str());
                ++renderedRows;

                size_t stageIndex = index + 1;
                while (stageIndex < snapshot.recentHistory.size() && renderedRows < maxHistoryRows)
                {
                    const resources::ResourceManager::DebugLoadEntry &stageEntry = snapshot.recentHistory[stageIndex];
                    if (stageEntry.jobId != entry.jobId || stageEntry.stage == "Completed" || stageEntry.stage == "Failed")
                    {
                        break;
                    }

                    ImGui::Indent();
                    ImGui::BulletText("%s: %.1f ms", stageEntry.stage.c_str(), stageEntry.elapsedMs);
                    ImGui::Unindent();
                    ++renderedRows;
                    ++stageIndex;
                }

                ImGui::Separator();
                index = stageIndex;
                continue;
            }

            std::string label = entry.resourceType;
            if (!entry.stage.empty())
            {
                label += " - ";
                label += entry.stage;
            }

            ImGui::Text("[%s] %.1f ms", label.c_str(), entry.elapsedMs);
            ImGui::TextWrapped("%s", entry.path.c_str());
            ImGui::Separator();
            ++renderedRows;
            ++index;
        }

        if (snapshot.recentHistory.size() > renderedRows)
        {
            ImGui::Text("... %d more", static_cast<int>(snapshot.recentHistory.size() - renderedRows));
        }
        ImGui::EndChild();
    }

    ImGui::Separator();
    if (ImGui::CollapsingHeader(("Aggregate timings (" + std::to_string(snapshot.aggregateStats.size()) + ")").c_str()))
    {
        static bool showAggregateGraph = false;

        std::vector<resources::ResourceManager::DebugLoadAggregate> aggregateStats = snapshot.aggregateStats;
        std::sort(
            aggregateStats.begin(),
            aggregateStats.end(),
            [](const resources::ResourceManager::DebugLoadAggregate &a, const resources::ResourceManager::DebugLoadAggregate &b)
            {
                return a.avgMs > b.avgMs;
            });

        if (ImGui::SmallButton(showAggregateGraph ? "Hide graph" : "Show graph"))
        {
            showAggregateGraph = !showAggregateGraph;
        }

        if (showAggregateGraph)
        {
            std::vector<resources::ResourceManager::DebugLoadAggregate> outlierStats = aggregateStats;
            std::sort(
                outlierStats.begin(),
                outlierStats.end(),
                [](const resources::ResourceManager::DebugLoadAggregate &a, const resources::ResourceManager::DebugLoadAggregate &b)
                {
                    return a.maxMs > b.maxMs;
                });

            const size_t graphCount = std::min<size_t>(outlierStats.size(), 24);
            std::vector<float> graphValues(graphCount, 0.0f);
            float graphMax = 1.0f;
            for (size_t i = 0; i < graphCount; ++i)
            {
                graphValues[i] = static_cast<float>(outlierStats[i].maxMs);
                graphMax = std::max(graphMax, graphValues[i]);
            }

            if (!graphValues.empty())
            {
                ImGui::PlotHistogram(
                    "##aggregate_outlier_graph",
                    graphValues.data(),
                    static_cast<int>(graphValues.size()),
                    0,
                    "Outlier view (max load time ms)",
                    0.0f,
                    graphMax * 1.1f,
                    ImVec2(0.0f, 110.0f));

                if (ImGui::IsItemHovered())
                {
                    const ImVec2 minRect = ImGui::GetItemRectMin();
                    const ImVec2 maxRect = ImGui::GetItemRectMax();
                    const float width = std::max(1.0f, maxRect.x - minRect.x);
                    const float localX = ImGui::GetIO().MousePos.x - minRect.x;
                    int barIndex = static_cast<int>((localX / width) * static_cast<float>(graphCount));
                    barIndex = std::clamp(barIndex, 0, static_cast<int>(graphCount) - 1);

                    const auto &entry = outlierStats[static_cast<size_t>(barIndex)];
                    ImGui::BeginTooltip();
                    ImGui::Text("[%s] max %.1f ms (avg %.1f)", entry.resourceType.c_str(), entry.maxMs, entry.avgMs);
                    ImGui::TextWrapped("%s", entry.path.c_str());
                    ImGui::EndTooltip();
                }
            }
        }

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