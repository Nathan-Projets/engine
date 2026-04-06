#pragma once

#include <unordered_map>

#include "../debug_ui.hpp"
#include "../../../core/scene.hpp"

class DebugAnimationPanel : public IDebugPanel
{
public:
    void SetScene(Scene *scene);
    void SetSnapshot(AnimationDebugSnapshot snapshot);
    void Draw() override;

private:
    struct EntityUIState
    {
        int selectedClipIndex = -1;
        float transitionDurationSeconds = 0.25f;
    };

    Scene *m_scene = nullptr;
    AnimationDebugSnapshot m_snapshot;
    std::unordered_map<Entity::ID, EntityUIState> m_uiState;
};