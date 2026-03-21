#pragma once

#include "../debug_ui.hpp"

#include <cstdint>
#include <glm/glm.hpp>

struct FrameStats
{
    int fps = 0;
    float frameTimeMs = 0.0f;
    uint64_t totalFrames = 0;
    double uptimeSeconds = 0.0;
    int viewportWidth = 0;
    int viewportHeight = 0;
    glm::vec2 mousePosition = {};
    bool sceneLoaded = false;
};

class DebugStatsPanel : public IDebugPanel
{
public:
    void SetStats(const FrameStats &stats);
    void Draw() override;

private:
    FrameStats m_stats;
};
