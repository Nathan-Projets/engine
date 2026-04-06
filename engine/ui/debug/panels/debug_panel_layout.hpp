#pragma once

#include <imgui.h>

namespace debug_panels
{
    constexpr float kInitialMarginX = 10.0f;
    constexpr float kInitialMarginY = 10.0f;
    constexpr float kCollapsedPanelHeight = 28.0f;
    constexpr float kPanelGapY = 6.0f;

    inline ImVec2 GetInitialStackedPosition(int panelIndex)
    {
        return ImVec2(
            kInitialMarginX,
            kInitialMarginY + static_cast<float>(panelIndex) * (kCollapsedPanelHeight + kPanelGapY));
    }
}