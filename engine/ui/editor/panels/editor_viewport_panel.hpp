#pragma once

#include <glm/glm.hpp>

#include "../../debug/debug_ui.hpp"
#include "../editor_context.hpp"

class EditorViewportPanel : public IDebugPanel
{
public:
    enum class GizmoAxis
    {
        None,
        X,
        Y,
        Z
    };

    void SetContext(EditorContext *context);
    void Draw() override;

private:
    struct GizmoDragState
    {
        bool active = false;
        GizmoAxis axis = GizmoAxis::None;
        Entity entity;
        EditorTransformState startTransform;
        ImVec2 startMouse = ImVec2(0.0f, 0.0f);
        glm::vec3 worldAxis = glm::vec3(0.0f);
        glm::vec2 screenAxis = glm::vec2(0.0f);
        float pixelsPerWorldUnit = 1.0f;
    };

    void CancelGizmoDrag();
    void HandlePicking(Scene *scene, const ImVec2 &imageMin, const ImVec2 &imageSize, const EditorViewportCameraState &cameraState);
    void DrawAndHandleTranslateGizmo(Scene *scene, const ImVec2 &imageMin, const ImVec2 &imageSize, const EditorViewportCameraState &cameraState);

    EditorContext *m_context = nullptr;
    GizmoDragState m_gizmoDrag;
};