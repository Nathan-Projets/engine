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

    enum class GizmoMode
    {
        Translate,
        Rotate,
        Scale
    };

    void SetContext(EditorContext *context);
    void Draw() override;

private:
    struct GizmoDragState
    {
        bool active = false;
        bool modified = false;
        GizmoMode mode = GizmoMode::Translate;
        GizmoAxis axis = GizmoAxis::None;
        Entity entity;
        EditorTransformState startTransform;
        ImVec2 startMouse = ImVec2(0.0f, 0.0f);
        glm::vec3 worldAxis = glm::vec3(0.0f);
        glm::vec2 screenAxis = glm::vec2(0.0f);
        float pixelsPerWorldUnit = 1.0f;
    };

    struct ColliderGizmoDragState
    {
        bool active = false;
        bool modified = false;
        GizmoMode mode = GizmoMode::Translate;
        GizmoAxis axis = GizmoAxis::None;
        Entity entity;
        EditorTransformState transform;
        EditorBoxColliderState boxCollider;
        EditorSphereColliderState sphereCollider;
        EditorCapsuleColliderState capsuleCollider;
        bool hasBoxCollider = false;
        bool hasSphereCollider = false;
        bool hasCapsuleCollider = false;
        ImVec2 startMouse = ImVec2(0.0f, 0.0f);
        glm::vec3 worldAxis = glm::vec3(0.0f);
        glm::vec2 screenAxis = glm::vec2(0.0f);
        float pixelsPerWorldUnit = 1.0f;
    };

    void CancelGizmoDrag();
    bool HasActiveGizmo() const;
    void HandlePicking(Scene *scene, const ImVec2 &imageMin, const ImVec2 &imageSize, const EditorViewportCameraState &cameraState);
    void DrawAndHandleTransformGizmo(Scene *scene, const ImVec2 &imageMin, const ImVec2 &imageSize, const EditorViewportCameraState &cameraState);
    void DrawAndHandleColliderGizmo(Scene *scene, const ImVec2 &imageMin, const ImVec2 &imageSize, const EditorViewportCameraState &cameraState);
    void DrawColliderOverlay(Scene *scene, const ImVec2 &imageMin, const ImVec2 &imageSize, const EditorViewportCameraState &cameraState);

    EditorContext *m_context = nullptr;
    GizmoDragState m_gizmoDrag;
    ColliderGizmoDragState m_colliderGizmoDrag;
    GizmoMode m_gizmoMode = GizmoMode::Translate;
    bool m_showColliders = false;
    bool m_editColliders = false;
};