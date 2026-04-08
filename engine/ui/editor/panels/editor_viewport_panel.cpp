#include "editor_viewport_panel.hpp"

#include <algorithm>
#include <cmath>

#include <imgui.h>

#include <glm/gtc/matrix_transform.hpp>

namespace
{
    bool ProjectWorldPoint(const EditorViewportCameraState &cameraState, const ImVec2 &imageMin, const ImVec2 &imageSize, const glm::vec3 &worldPoint, ImVec2 &screenPoint)
    {
        const glm::vec4 clip = cameraState.viewProjection * glm::vec4(worldPoint, 1.0f);
        if (std::abs(clip.w) < 0.00001f || clip.w <= 0.0f)
        {
            return false;
        }

        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        if (ndc.z < -1.0f || ndc.z > 1.0f)
        {
            return false;
        }

        screenPoint.x = imageMin.x + (ndc.x * 0.5f + 0.5f) * imageSize.x;
        screenPoint.y = imageMin.y + (-ndc.y * 0.5f + 0.5f) * imageSize.y;
        return true;
    }

    float DistanceToSegment(const ImVec2 &point, const ImVec2 &segmentStart, const ImVec2 &segmentEnd)
    {
        const float dx = segmentEnd.x - segmentStart.x;
        const float dy = segmentEnd.y - segmentStart.y;
        const float lengthSquared = dx * dx + dy * dy;
        if (lengthSquared <= 0.00001f)
        {
            const float px = point.x - segmentStart.x;
            const float py = point.y - segmentStart.y;
            return std::sqrt(px * px + py * py);
        }

        const float projection = ((point.x - segmentStart.x) * dx + (point.y - segmentStart.y) * dy) / lengthSquared;
        const float t = std::clamp(projection, 0.0f, 1.0f);
        const float closestX = segmentStart.x + t * dx;
        const float closestY = segmentStart.y + t * dy;
        const float deltaX = point.x - closestX;
        const float deltaY = point.y - closestY;
        return std::sqrt(deltaX * deltaX + deltaY * deltaY);
    }

    ImU32 GetAxisColor(EditorViewportPanel::GizmoAxis axis, bool active)
    {
        if (active)
        {
            return IM_COL32(255, 235, 120, 255);
        }

        switch (axis)
        {
        case EditorViewportPanel::GizmoAxis::X:
            return IM_COL32(230, 70, 70, 255);
        case EditorViewportPanel::GizmoAxis::Y:
            return IM_COL32(90, 210, 90, 255);
        case EditorViewportPanel::GizmoAxis::Z:
            return IM_COL32(90, 150, 255, 255);
        default:
            return IM_COL32(220, 220, 220, 255);
        }
    }

    glm::vec3 GetAxisVector(EditorViewportPanel::GizmoAxis axis)
    {
        switch (axis)
        {
        case EditorViewportPanel::GizmoAxis::X:
            return glm::vec3(1.0f, 0.0f, 0.0f);
        case EditorViewportPanel::GizmoAxis::Y:
            return glm::vec3(0.0f, 1.0f, 0.0f);
        case EditorViewportPanel::GizmoAxis::Z:
            return glm::vec3(0.0f, 0.0f, 1.0f);
        default:
            return glm::vec3(0.0f);
        }
    }

    const char *GetModeLabel(EditorViewportPanel::GizmoMode mode)
    {
        switch (mode)
        {
        case EditorViewportPanel::GizmoMode::Rotate:
            return "Rotate";
        case EditorViewportPanel::GizmoMode::Scale:
            return "Scale";
        default:
            return "Translate";
        }
    }

    glm::mat4 BuildRotationMatrix(const EditorTransformState &transform)
    {
        glm::mat4 rotation(1.0f);
        rotation = glm::rotate(rotation, glm::radians(transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        rotation = glm::rotate(rotation, glm::radians(transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        rotation = glm::rotate(rotation, glm::radians(transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        return rotation;
    }

    glm::mat4 BuildTransformMatrix(const EditorTransformState &transform)
    {
        glm::mat4 matrix(1.0f);
        matrix = glm::translate(matrix, transform.position);
        matrix *= BuildRotationMatrix(transform);
        matrix = glm::scale(matrix, transform.scale);
        return matrix;
    }

    glm::vec3 TransformDirection(const EditorTransformState &transform, const glm::vec3 &direction)
    {
        return glm::normalize(glm::vec3(BuildRotationMatrix(transform) * glm::vec4(direction, 0.0f)));
    }

    glm::vec3 TransformColliderPoint(const EditorTransformState &transform, const glm::vec3 &localPoint)
    {
        const glm::mat4 matrix = BuildTransformMatrix(transform);
        return glm::vec3(matrix * glm::vec4(localPoint, 1.0f));
    }

    glm::vec3 TransformColliderOffset(const EditorTransformState &transform, const glm::vec3 &offset)
    {
        return transform.position + glm::vec3(BuildRotationMatrix(transform) * glm::vec4(offset, 0.0f));
    }

    int GetAxisIndex(EditorViewportPanel::GizmoAxis axis)
    {
        switch (axis)
        {
        case EditorViewportPanel::GizmoAxis::X:
            return 0;
        case EditorViewportPanel::GizmoAxis::Y:
            return 1;
        case EditorViewportPanel::GizmoAxis::Z:
            return 2;
        default:
            return -1;
        }
    }

    float GetColliderHandleDistance(const EditorEntityInspectorState &state, EditorViewportPanel::GizmoMode mode, EditorViewportPanel::GizmoAxis axis, float fallbackLength)
    {
        const int axisIndex = GetAxisIndex(axis);
        if (axisIndex < 0)
        {
            return fallbackLength;
        }

        if (mode != EditorViewportPanel::GizmoMode::Scale)
        {
            return fallbackLength;
        }

        if (state.hasBoxCollider)
        {
            const glm::vec3 scaled = state.boxCollider.halfExtents * glm::abs(state.transform.scale);
            return std::max(scaled[axisIndex], fallbackLength * 0.35f);
        }

        if (state.hasSphereCollider)
        {
            const float radius = state.sphereCollider.radius * std::max({std::abs(state.transform.scale.x), std::abs(state.transform.scale.y), std::abs(state.transform.scale.z), 1.0f});
            return std::max(radius, fallbackLength * 0.35f);
        }

        if (state.hasCapsuleCollider)
        {
            if (axis == EditorViewportPanel::GizmoAxis::Y)
            {
                return std::max(state.capsuleCollider.height * std::abs(state.transform.scale.y) * 0.5f, fallbackLength * 0.35f);
            }

            const float radius = state.capsuleCollider.radius * std::max({std::abs(state.transform.scale.x), std::abs(state.transform.scale.z), 1.0f});
            return std::max(radius, fallbackLength * 0.35f);
        }

        return fallbackLength;
    }

    void DrawWorldSegment(const EditorViewportCameraState &cameraState, const ImVec2 &imageMin, const ImVec2 &imageSize, ImDrawList *drawList, const glm::vec3 &start, const glm::vec3 &end, ImU32 color, float thickness = 1.5f)
    {
        ImVec2 screenStart;
        ImVec2 screenEnd;
        if (!ProjectWorldPoint(cameraState, imageMin, imageSize, start, screenStart) ||
            !ProjectWorldPoint(cameraState, imageMin, imageSize, end, screenEnd))
        {
            return;
        }

        drawList->AddLine(screenStart, screenEnd, color, thickness);
    }

    void DrawWireBox(const EditorViewportCameraState &cameraState, const ImVec2 &imageMin, const ImVec2 &imageSize, ImDrawList *drawList, const EditorTransformState &transform, const EditorBoxColliderState &collider, ImU32 color)
    {
        const glm::vec3 center = collider.center;
        const glm::vec3 halfExtents = collider.halfExtents * glm::abs(transform.scale);
        const glm::vec3 corners[8] = {
            glm::vec3(-halfExtents.x, -halfExtents.y, -halfExtents.z),
            glm::vec3(halfExtents.x, -halfExtents.y, -halfExtents.z),
            glm::vec3(halfExtents.x, halfExtents.y, -halfExtents.z),
            glm::vec3(-halfExtents.x, halfExtents.y, -halfExtents.z),
            glm::vec3(-halfExtents.x, -halfExtents.y, halfExtents.z),
            glm::vec3(halfExtents.x, -halfExtents.y, halfExtents.z),
            glm::vec3(halfExtents.x, halfExtents.y, halfExtents.z),
            glm::vec3(-halfExtents.x, halfExtents.y, halfExtents.z)};

        glm::vec3 transformed[8];
        const glm::vec3 worldCenter = TransformColliderOffset(transform, center);
        glm::mat4 rotation(1.0f);
        rotation = glm::rotate(rotation, glm::radians(transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        rotation = glm::rotate(rotation, glm::radians(transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        rotation = glm::rotate(rotation, glm::radians(transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        for (int index = 0; index < 8; ++index)
        {
            transformed[index] = worldCenter + glm::vec3(rotation * glm::vec4(corners[index], 0.0f));
        }

        const int edges[12][2] = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7}};
        for (const auto &edge : edges)
        {
            DrawWorldSegment(cameraState, imageMin, imageSize, drawList, transformed[edge[0]], transformed[edge[1]], color);
        }
    }

    void DrawCircleRing(const EditorViewportCameraState &cameraState, const ImVec2 &imageMin, const ImVec2 &imageSize, ImDrawList *drawList, const glm::vec3 &center, const glm::vec3 &axisA, const glm::vec3 &axisB, float radius, ImU32 color)
    {
        constexpr int segmentCount = 24;
        glm::vec3 previousPoint = center + axisA * radius;
        for (int index = 1; index <= segmentCount; ++index)
        {
            const float angle = (static_cast<float>(index) / static_cast<float>(segmentCount)) * 2.0f * 3.14159265f;
            const glm::vec3 point = center + (axisA * std::cos(angle) + axisB * std::sin(angle)) * radius;
            DrawWorldSegment(cameraState, imageMin, imageSize, drawList, previousPoint, point, color);
            previousPoint = point;
        }
    }
}

void EditorViewportPanel::SetContext(EditorContext *context)
{
    m_context = context;
}

void EditorViewportPanel::CancelGizmoDrag()
{
    if (m_context && m_context->HasPendingTransaction())
    {
        m_context->CancelTransaction();
    }
    m_gizmoDrag = {};
    m_colliderGizmoDrag = {};
}

bool EditorViewportPanel::HasActiveGizmo() const
{
    return m_gizmoDrag.active || m_colliderGizmoDrag.active;
}

void EditorViewportPanel::HandlePicking(Scene *scene, const ImVec2 &imageMin, const ImVec2 &imageSize, const EditorViewportCameraState &cameraState)
{
    if (!scene || !cameraState.valid || !ImGui::IsItemHovered() || !ImGui::IsMouseClicked(ImGuiMouseButton_Left) || HasActiveGizmo())
    {
        return;
    }

    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const float localX = std::clamp(mouse.x - imageMin.x, 0.0f, imageSize.x);
    const float localY = std::clamp(mouse.y - imageMin.y, 0.0f, imageSize.y);

    const Entity pickedEntity = scene->PickEditorEntityInViewport(localX, localY);
    if (pickedEntity.IsValid())
    {
        m_context->SelectEntity(pickedEntity);
    }
    else
    {
        m_context->ClearSelection();
    }
}

void EditorViewportPanel::DrawAndHandleTransformGizmo(Scene *scene, const ImVec2 &imageMin, const ImVec2 &imageSize, const EditorViewportCameraState &cameraState)
{
    if (!scene || !cameraState.valid || !m_context || !m_context->HasSelection() || !m_context->CanEdit())
    {
        CancelGizmoDrag();
        return;
    }

    const EditorEntityInspectorState state = scene->GetEditorEntityInspectorState(m_context->GetSelectedEntity());
    if (!state.valid || !state.hasTransform)
    {
        CancelGizmoDrag();
        return;
    }

    const glm::vec3 origin = state.transform.position;
    const float cameraDistance = std::max(glm::length(cameraState.position - origin), 0.25f);
    const float handleLength = std::max(cameraDistance * 0.15f, 0.75f);

    ImVec2 originScreen;
    if (!ProjectWorldPoint(cameraState, imageMin, imageSize, origin, originScreen))
    {
        CancelGizmoDrag();
        return;
    }

    ImDrawList *drawList = ImGui::GetWindowDrawList();
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const bool imageHovered = ImGui::IsItemHovered();
    GizmoAxis hoveredAxis = GizmoAxis::None;
    float hoveredDistance = 10.0f;

    for (GizmoAxis axis : {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z})
    {
        const glm::vec3 axisVector = GetAxisVector(axis);
        ImVec2 endpointScreen;
        if (!ProjectWorldPoint(cameraState, imageMin, imageSize, origin + axisVector * handleLength, endpointScreen))
        {
            continue;
        }

        const bool axisActive = m_gizmoDrag.active && m_gizmoDrag.axis == axis;
        const float thickness = m_gizmoMode == GizmoMode::Rotate ? 2.0f : (axisActive ? 4.0f : 3.0f);
        drawList->AddLine(originScreen, endpointScreen, GetAxisColor(axis, axisActive), thickness);
        drawList->AddCircleFilled(endpointScreen, axisActive ? 7.0f : 6.0f, GetAxisColor(axis, axisActive));

        if (!m_gizmoDrag.active && imageHovered)
        {
            const float distance = DistanceToSegment(mouse, originScreen, endpointScreen);
            if (distance < hoveredDistance)
            {
                hoveredDistance = distance;
                hoveredAxis = axis;
            }
        }
    }

    drawList->AddCircleFilled(originScreen, 5.0f, IM_COL32(240, 240, 240, 255));

    if (!m_gizmoDrag.active && imageHovered && hoveredAxis != GizmoAxis::None && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        const glm::vec3 worldAxis = GetAxisVector(hoveredAxis);
        ImVec2 endpointScreen;
        if (ProjectWorldPoint(cameraState, imageMin, imageSize, origin + worldAxis * handleLength, endpointScreen))
        {
            const glm::vec2 screenAxis(endpointScreen.x - originScreen.x, endpointScreen.y - originScreen.y);
            const float screenAxisLength = std::sqrt(screenAxis.x * screenAxis.x + screenAxis.y * screenAxis.y);
            if (screenAxisLength > 0.00001f)
            {
                ImVec2 oneUnitScreen;
                float pixelsPerWorldUnit = screenAxisLength / handleLength;
                if (ProjectWorldPoint(cameraState, imageMin, imageSize, origin + worldAxis, oneUnitScreen))
                {
                    const float dx = oneUnitScreen.x - originScreen.x;
                    const float dy = oneUnitScreen.y - originScreen.y;
                    pixelsPerWorldUnit = std::max(std::sqrt(dx * dx + dy * dy), 1.0f);
                }

                m_context->BeginTransaction();
                m_gizmoDrag.active = true;
                m_gizmoDrag.modified = false;
                m_gizmoDrag.mode = m_gizmoMode;
                m_gizmoDrag.axis = hoveredAxis;
                m_gizmoDrag.entity = state.entity;
                m_gizmoDrag.startTransform = state.transform;
                m_gizmoDrag.startMouse = mouse;
                m_gizmoDrag.worldAxis = worldAxis;
                m_gizmoDrag.screenAxis = screenAxis / screenAxisLength;
                m_gizmoDrag.pixelsPerWorldUnit = std::max(pixelsPerWorldUnit, 1.0f);
            }
        }
    }

    if (!m_gizmoDrag.active)
    {
        return;
    }

    if (m_gizmoDrag.entity != state.entity)
    {
        CancelGizmoDrag();
        return;
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        if (m_context)
        {
            m_context->CommitTransaction(m_gizmoDrag.modified);
        }
        m_gizmoDrag = {};
        return;
    }

    const ImVec2 delta(mouse.x - m_gizmoDrag.startMouse.x, mouse.y - m_gizmoDrag.startMouse.y);
    const float projectedDeltaPixels = delta.x * m_gizmoDrag.screenAxis.x + delta.y * m_gizmoDrag.screenAxis.y;
    const float worldDelta = projectedDeltaPixels / std::max(m_gizmoDrag.pixelsPerWorldUnit, 1.0f);

    EditorTransformState updatedTransform = m_gizmoDrag.startTransform;
    switch (m_gizmoDrag.mode)
    {
    case GizmoMode::Rotate:
        updatedTransform.rotation += m_gizmoDrag.worldAxis * (projectedDeltaPixels * 0.3f);
        break;
    case GizmoMode::Scale:
        updatedTransform.scale += m_gizmoDrag.worldAxis * (projectedDeltaPixels * 0.01f);
        updatedTransform.scale.x = std::max(updatedTransform.scale.x, 0.001f);
        updatedTransform.scale.y = std::max(updatedTransform.scale.y, 0.001f);
        updatedTransform.scale.z = std::max(updatedTransform.scale.z, 0.001f);
        break;
    default:
        updatedTransform.position += m_gizmoDrag.worldAxis * worldDelta;
        break;
    }

    if (scene->SetEditorTransform(state.entity, updatedTransform))
    {
        m_gizmoDrag.modified = true;
    }
}

void EditorViewportPanel::DrawAndHandleColliderGizmo(Scene *scene, const ImVec2 &imageMin, const ImVec2 &imageSize, const EditorViewportCameraState &cameraState)
{
    if (!scene || !cameraState.valid || !m_context || !m_context->HasSelection() || !m_context->CanEdit() || !m_editColliders)
    {
        if (m_colliderGizmoDrag.active)
        {
            CancelGizmoDrag();
        }
        return;
    }

    const EditorEntityInspectorState state = scene->GetEditorEntityInspectorState(m_context->GetSelectedEntity());
    if (!state.valid || !state.hasTransform || (!state.hasBoxCollider && !state.hasSphereCollider && !state.hasCapsuleCollider))
    {
        if (m_colliderGizmoDrag.active)
        {
            CancelGizmoDrag();
        }
        return;
    }

    if (m_gizmoMode == GizmoMode::Rotate)
    {
        return;
    }

    const glm::vec3 origin = state.hasBoxCollider ? TransformColliderOffset(state.transform, state.boxCollider.center)
                                                  : (state.hasSphereCollider ? TransformColliderOffset(state.transform, state.sphereCollider.center)
                                                                            : TransformColliderOffset(state.transform, state.capsuleCollider.center));
    const float cameraDistance = std::max(glm::length(cameraState.position - origin), 0.25f);
    const float baseHandleLength = std::max(cameraDistance * 0.15f, 0.75f);

    ImVec2 originScreen;
    if (!ProjectWorldPoint(cameraState, imageMin, imageSize, origin, originScreen))
    {
        if (m_colliderGizmoDrag.active)
        {
            CancelGizmoDrag();
        }
        return;
    }

    ImDrawList *drawList = ImGui::GetWindowDrawList();
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const bool imageHovered = ImGui::IsItemHovered();
    GizmoAxis hoveredAxis = GizmoAxis::None;
    float hoveredDistance = 10.0f;

    for (GizmoAxis axis : {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z})
    {
        const glm::vec3 localAxis = GetAxisVector(axis);
        const glm::vec3 worldAxis = TransformDirection(state.transform, localAxis);
        const float axisLength = GetColliderHandleDistance(state, m_gizmoMode, axis, baseHandleLength);
        ImVec2 endpointScreen;
        if (!ProjectWorldPoint(cameraState, imageMin, imageSize, origin + worldAxis * axisLength, endpointScreen))
        {
            continue;
        }

        const bool axisActive = m_colliderGizmoDrag.active && m_colliderGizmoDrag.axis == axis;
        drawList->AddLine(originScreen, endpointScreen, GetAxisColor(axis, axisActive), axisActive ? 4.0f : 3.0f);
        drawList->AddCircleFilled(endpointScreen, axisActive ? 7.0f : 6.0f, GetAxisColor(axis, axisActive));

        if (!m_colliderGizmoDrag.active && imageHovered)
        {
            const float distance = DistanceToSegment(mouse, originScreen, endpointScreen);
            if (distance < hoveredDistance)
            {
                hoveredDistance = distance;
                hoveredAxis = axis;
            }
        }
    }

    drawList->AddCircleFilled(originScreen, 5.0f, IM_COL32(255, 245, 180, 255));

    if (!m_colliderGizmoDrag.active && imageHovered && hoveredAxis != GizmoAxis::None && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        const glm::vec3 worldAxis = TransformDirection(state.transform, GetAxisVector(hoveredAxis));
        const float axisLength = GetColliderHandleDistance(state, m_gizmoMode, hoveredAxis, baseHandleLength);
        ImVec2 endpointScreen;
        if (ProjectWorldPoint(cameraState, imageMin, imageSize, origin + worldAxis * axisLength, endpointScreen))
        {
            const glm::vec2 screenAxis(endpointScreen.x - originScreen.x, endpointScreen.y - originScreen.y);
            const float screenAxisLength = std::sqrt(screenAxis.x * screenAxis.x + screenAxis.y * screenAxis.y);
            if (screenAxisLength > 0.00001f)
            {
                if (m_context->BeginTransaction())
                {
                    ImVec2 oneUnitScreen;
                    float pixelsPerWorldUnit = screenAxisLength / std::max(axisLength, 0.001f);
                    if (ProjectWorldPoint(cameraState, imageMin, imageSize, origin + worldAxis, oneUnitScreen))
                    {
                        const float dx = oneUnitScreen.x - originScreen.x;
                        const float dy = oneUnitScreen.y - originScreen.y;
                        pixelsPerWorldUnit = std::max(std::sqrt(dx * dx + dy * dy), 1.0f);
                    }

                    m_colliderGizmoDrag.active = true;
                    m_colliderGizmoDrag.modified = false;
                    m_colliderGizmoDrag.mode = m_gizmoMode;
                    m_colliderGizmoDrag.axis = hoveredAxis;
                    m_colliderGizmoDrag.entity = state.entity;
                    m_colliderGizmoDrag.transform = state.transform;
                    m_colliderGizmoDrag.boxCollider = state.boxCollider;
                    m_colliderGizmoDrag.sphereCollider = state.sphereCollider;
                    m_colliderGizmoDrag.capsuleCollider = state.capsuleCollider;
                    m_colliderGizmoDrag.hasBoxCollider = state.hasBoxCollider;
                    m_colliderGizmoDrag.hasSphereCollider = state.hasSphereCollider;
                    m_colliderGizmoDrag.hasCapsuleCollider = state.hasCapsuleCollider;
                    m_colliderGizmoDrag.startMouse = mouse;
                    m_colliderGizmoDrag.worldAxis = worldAxis;
                    m_colliderGizmoDrag.screenAxis = screenAxis / screenAxisLength;
                    m_colliderGizmoDrag.pixelsPerWorldUnit = std::max(pixelsPerWorldUnit, 1.0f);
                }
            }
        }
    }

    if (!m_colliderGizmoDrag.active)
    {
        return;
    }

    if (m_colliderGizmoDrag.entity != state.entity)
    {
        CancelGizmoDrag();
        return;
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        m_context->CommitTransaction(m_colliderGizmoDrag.modified);
        m_colliderGizmoDrag = {};
        return;
    }

    const ImVec2 delta(mouse.x - m_colliderGizmoDrag.startMouse.x, mouse.y - m_colliderGizmoDrag.startMouse.y);
    const float projectedDeltaPixels = delta.x * m_colliderGizmoDrag.screenAxis.x + delta.y * m_colliderGizmoDrag.screenAxis.y;
    const float worldDelta = projectedDeltaPixels / std::max(m_colliderGizmoDrag.pixelsPerWorldUnit, 1.0f);
    const int axisIndex = GetAxisIndex(m_colliderGizmoDrag.axis);
    bool updated = false;

    if (m_colliderGizmoDrag.mode == GizmoMode::Translate && axisIndex >= 0)
    {
        if (m_colliderGizmoDrag.hasBoxCollider)
        {
            EditorBoxColliderState collider = m_colliderGizmoDrag.boxCollider;
            collider.center[axisIndex] += worldDelta;
            updated = scene->SetEditorBoxCollider(state.entity, collider);
        }
        else if (m_colliderGizmoDrag.hasSphereCollider)
        {
            EditorSphereColliderState collider = m_colliderGizmoDrag.sphereCollider;
            collider.center[axisIndex] += worldDelta;
            updated = scene->SetEditorSphereCollider(state.entity, collider);
        }
        else if (m_colliderGizmoDrag.hasCapsuleCollider)
        {
            EditorCapsuleColliderState collider = m_colliderGizmoDrag.capsuleCollider;
            collider.center[axisIndex] += worldDelta;
            updated = scene->SetEditorCapsuleCollider(state.entity, collider);
        }
    }
    else if (m_colliderGizmoDrag.mode == GizmoMode::Scale)
    {
        if (m_colliderGizmoDrag.hasBoxCollider && axisIndex >= 0)
        {
            EditorBoxColliderState collider = m_colliderGizmoDrag.boxCollider;
            const float scaleMagnitude = std::max(std::abs(m_colliderGizmoDrag.transform.scale[axisIndex]), 0.001f);
            collider.halfExtents[axisIndex] = std::max(collider.halfExtents[axisIndex] + worldDelta / scaleMagnitude, 0.001f);
            updated = scene->SetEditorBoxCollider(state.entity, collider);
        }
        else if (m_colliderGizmoDrag.hasSphereCollider)
        {
            EditorSphereColliderState collider = m_colliderGizmoDrag.sphereCollider;
            const float scaleMagnitude = std::max({std::abs(m_colliderGizmoDrag.transform.scale.x), std::abs(m_colliderGizmoDrag.transform.scale.y), std::abs(m_colliderGizmoDrag.transform.scale.z), 0.001f});
            collider.radius = std::max(collider.radius + worldDelta / scaleMagnitude, 0.001f);
            updated = scene->SetEditorSphereCollider(state.entity, collider);
        }
        else if (m_colliderGizmoDrag.hasCapsuleCollider)
        {
            EditorCapsuleColliderState collider = m_colliderGizmoDrag.capsuleCollider;
            if (m_colliderGizmoDrag.axis == GizmoAxis::Y)
            {
                const float scaleMagnitude = std::max(std::abs(m_colliderGizmoDrag.transform.scale.y), 0.001f);
                collider.height = std::max(collider.height + (worldDelta / scaleMagnitude) * 2.0f, collider.radius * 2.0f);
            }
            else
            {
                const float scaleMagnitude = std::max({std::abs(m_colliderGizmoDrag.transform.scale.x), std::abs(m_colliderGizmoDrag.transform.scale.z), 0.001f});
                collider.radius = std::max(collider.radius + worldDelta / scaleMagnitude, 0.001f);
                collider.height = std::max(collider.height, collider.radius * 2.0f);
            }
            updated = scene->SetEditorCapsuleCollider(state.entity, collider);
        }
    }

    if (updated)
    {
        m_colliderGizmoDrag.modified = true;
    }
}

void EditorViewportPanel::DrawColliderOverlay(Scene *scene, const ImVec2 &imageMin, const ImVec2 &imageSize, const EditorViewportCameraState &cameraState)
{
    if (!scene || !cameraState.valid || !m_showColliders)
    {
        return;
    }

    ImDrawList *drawList = ImGui::GetWindowDrawList();
    const Entity selectedEntity = m_context ? m_context->GetSelectedEntity() : Entity();
    const std::vector<EditorEntitySummary> entities = scene->GetEditorEntitySummaries();
    for (const EditorEntitySummary &summary : entities)
    {
        const EditorEntityInspectorState state = scene->GetEditorEntityInspectorState(summary.entity);
        if (!state.valid || !state.hasTransform)
        {
            continue;
        }

        const bool selected = selectedEntity.IsValid() && summary.entity == selectedEntity;
        const ImU32 color = selected ? IM_COL32(255, 220, 80, 255) : IM_COL32(80, 220, 255, 230);

        if (state.hasBoxCollider)
        {
            DrawWireBox(cameraState, imageMin, imageSize, drawList, state.transform, state.boxCollider, color);
        }

        if (state.hasSphereCollider)
        {
            const glm::vec3 sphereCenter = TransformColliderOffset(state.transform, state.sphereCollider.center);
            const float radius = state.sphereCollider.radius * std::max({std::abs(state.transform.scale.x), std::abs(state.transform.scale.y), std::abs(state.transform.scale.z), 1.0f});
            DrawCircleRing(cameraState, imageMin, imageSize, drawList, sphereCenter, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), radius, color);
            DrawCircleRing(cameraState, imageMin, imageSize, drawList, sphereCenter, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), radius, color);
            DrawCircleRing(cameraState, imageMin, imageSize, drawList, sphereCenter, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), radius, color);
        }

        if (state.hasCapsuleCollider)
        {
            glm::mat4 rotation(1.0f);
            rotation = glm::rotate(rotation, glm::radians(state.transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            rotation = glm::rotate(rotation, glm::radians(state.transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            rotation = glm::rotate(rotation, glm::radians(state.transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            const glm::vec3 up = glm::normalize(glm::vec3(rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
            const glm::vec3 right = glm::normalize(glm::vec3(rotation * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
            const glm::vec3 forward = glm::normalize(glm::vec3(rotation * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)));
            const glm::vec3 center = TransformColliderOffset(state.transform, state.capsuleCollider.center);
            const float radius = state.capsuleCollider.radius * std::max({std::abs(state.transform.scale.x), std::abs(state.transform.scale.z), 1.0f});
            const float height = std::max(state.capsuleCollider.height * std::abs(state.transform.scale.y), radius * 2.0f);
            const float cylinderHalfHeight = std::max((height * 0.5f) - radius, 0.0f);
            const glm::vec3 topCenter = center + up * cylinderHalfHeight;
            const glm::vec3 bottomCenter = center - up * cylinderHalfHeight;

            DrawWorldSegment(cameraState, imageMin, imageSize, drawList, topCenter + right * radius, bottomCenter + right * radius, color);
            DrawWorldSegment(cameraState, imageMin, imageSize, drawList, topCenter - right * radius, bottomCenter - right * radius, color);
            DrawWorldSegment(cameraState, imageMin, imageSize, drawList, topCenter + forward * radius, bottomCenter + forward * radius, color);
            DrawWorldSegment(cameraState, imageMin, imageSize, drawList, topCenter - forward * radius, bottomCenter - forward * radius, color);
            DrawCircleRing(cameraState, imageMin, imageSize, drawList, topCenter, right, forward, radius, color);
            DrawCircleRing(cameraState, imageMin, imageSize, drawList, bottomCenter, right, forward, radius, color);
            DrawCircleRing(cameraState, imageMin, imageSize, drawList, topCenter, up, right, radius, color);
            DrawCircleRing(cameraState, imageMin, imageSize, drawList, bottomCenter, up, right, radius, color);
        }
    }
}

void EditorViewportPanel::Draw()
{
    ImGui::SetNextWindowSize(ImVec2(800.0f, 600.0f), ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport");

    if (!m_context)
    {
        ImGui::TextUnformatted("Editor context unavailable.");
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }

    m_context->SetViewportHovered(ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows));
    m_context->SetViewportFocused(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));

    Scene *scene = m_context->GetScene();
    const ImVec2 availableSize = ImGui::GetContentRegionAvail();
    const int viewportWidth = std::max(1, static_cast<int>(availableSize.x));
    const int viewportHeight = std::max(1, static_cast<int>(availableSize.y));

    if (scene)
    {
        scene->SetEditorViewportSize(viewportWidth, viewportHeight);
    }

    ImDrawList *drawList = ImGui::GetWindowDrawList();
    const ImVec2 min = ImGui::GetWindowPos();
    const ImVec2 max(min.x + ImGui::GetWindowSize().x, min.y + ImGui::GetWindowSize().y);
    drawList->AddRectFilled(min, max, IM_COL32(18, 20, 24, 80));

    const uint32_t textureId = scene ? scene->GetEditorViewportTextureId() : 0;
    const EditorViewportCameraState cameraState = scene ? scene->GetEditorViewportCameraState() : EditorViewportCameraState{};
    if (textureId != 0 && availableSize.x > 0.0f && availableSize.y > 0.0f)
    {
        ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(textureId)), availableSize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
        const ImVec2 imageMin = ImGui::GetItemRectMin();
        const ImVec2 imageSize = ImGui::GetItemRectSize();
        ImGuiIO &io = ImGui::GetIO();

        if (scene && cameraState.valid && ImGui::IsItemHovered() && !HasActiveGizmo())
        {
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
            {
                if (io.KeyShift)
                {
                    scene->DollyEditorViewportCamera(io.MouseDelta.y);
                }
                else
                {
                    scene->OrbitEditorViewportCamera(io.MouseDelta.x, io.MouseDelta.y);
                }
            }

            if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
            {
                scene->PanEditorViewportCamera(io.MouseDelta.x, -io.MouseDelta.y);
            }

            if (m_context->HasSelection() && ImGui::IsKeyPressed(ImGuiKey_F, false))
            {
                scene->FrameEditorEntity(m_context->GetSelectedEntity());
            }
        }

        DrawColliderOverlay(scene, imageMin, imageSize, cameraState);
        if (m_editColliders)
        {
            DrawAndHandleColliderGizmo(scene, imageMin, imageSize, cameraState);
        }
        else
        {
            DrawAndHandleTransformGizmo(scene, imageMin, imageSize, cameraState);
        }
        HandlePicking(scene, imageMin, imageSize, cameraState);

        ImGui::SetCursorScreenPos(ImVec2(imageMin.x + 12.0f, imageMin.y + 12.0f));
        ImGui::BeginChild("viewport_overlay", ImVec2(340.0f, 136.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::TextUnformatted("Viewport");

        ImGui::BeginDisabled(!m_context->CanEdit());
        if (ImGui::Selectable("Translate", m_gizmoMode == GizmoMode::Translate, 0, ImVec2(80.0f, 0.0f)))
        {
            m_gizmoMode = GizmoMode::Translate;
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(m_editColliders);
        if (ImGui::Selectable("Rotate", m_gizmoMode == GizmoMode::Rotate, 0, ImVec2(80.0f, 0.0f)))
        {
            m_gizmoMode = GizmoMode::Rotate;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Selectable("Scale", m_gizmoMode == GizmoMode::Scale, 0, ImVec2(80.0f, 0.0f)))
        {
            m_gizmoMode = GizmoMode::Scale;
        }

        if (ImGui::Button("Frame Selected") && m_context->HasSelection())
        {
            scene->FrameEditorEntity(m_context->GetSelectedEntity());
        }
        ImGui::Checkbox("Show Colliders", &m_showColliders);
        if (ImGui::Checkbox("Edit Selected Collider", &m_editColliders) && m_editColliders && m_gizmoMode == GizmoMode::Rotate)
        {
            m_gizmoMode = GizmoMode::Translate;
        }
        ImGui::EndDisabled();

        ImGui::Text("Tool: %s%s", GetModeLabel(m_gizmoMode), m_editColliders ? " Collider" : " Transform");
        ImGui::TextUnformatted(m_editColliders ? "Left click: pick | Drag axis: move/resize collider" : "Left click: pick | Drag axis: edit transform");
        ImGui::TextUnformatted("Right drag: orbit | Middle drag: pan | Shift+Right: dolly");
        ImGui::TextUnformatted("F: frame selected");
        ImGui::EndChild();
    }
    else
    {
        CancelGizmoDrag();
        ImGui::SetCursorPos(ImVec2(12.0f, 12.0f));
        ImGui::BeginChild("viewport_empty_state", ImVec2(320.0f, 84.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::TextUnformatted("Viewport");
        ImGui::TextUnformatted(scene ? "Waiting for renderer output..." : "No scene bound.");
        ImGui::TextUnformatted(m_context->IsViewportFocused() ? "Input: focused" : "Input: click to focus");
        ImGui::TextUnformatted(m_context->IsViewportHovered() ? "Mouse: hovering viewport" : "Mouse: outside viewport");
        ImGui::EndChild();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}