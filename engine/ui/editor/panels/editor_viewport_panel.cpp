#include "editor_viewport_panel.hpp"

#include <algorithm>
#include <cmath>

#include <imgui.h>

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
}

void EditorViewportPanel::SetContext(EditorContext *context)
{
    m_context = context;
}

void EditorViewportPanel::CancelGizmoDrag()
{
    m_gizmoDrag = {};
}

void EditorViewportPanel::HandlePicking(Scene *scene, const ImVec2 &imageMin, const ImVec2 &imageSize, const EditorViewportCameraState &cameraState)
{
    if (!scene || !cameraState.valid || !ImGui::IsItemHovered() || !ImGui::IsMouseClicked(ImGuiMouseButton_Left) || m_gizmoDrag.active)
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

void EditorViewportPanel::DrawAndHandleTranslateGizmo(Scene *scene, const ImVec2 &imageMin, const ImVec2 &imageSize, const EditorViewportCameraState &cameraState)
{
    if (!scene || !cameraState.valid || !m_context || !m_context->HasSelection())
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
        drawList->AddLine(originScreen, endpointScreen, GetAxisColor(axis, axisActive), axisActive ? 4.0f : 3.0f);
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

                m_gizmoDrag.active = true;
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
        CancelGizmoDrag();
        return;
    }

    const ImVec2 delta(mouse.x - m_gizmoDrag.startMouse.x, mouse.y - m_gizmoDrag.startMouse.y);
    const float projectedDeltaPixels = delta.x * m_gizmoDrag.screenAxis.x + delta.y * m_gizmoDrag.screenAxis.y;
    const float worldDelta = projectedDeltaPixels / std::max(m_gizmoDrag.pixelsPerWorldUnit, 1.0f);

    EditorTransformState updatedTransform = m_gizmoDrag.startTransform;
    updatedTransform.position += m_gizmoDrag.worldAxis * worldDelta;
    if (scene->SetEditorTransform(state.entity, updatedTransform))
    {
        m_context->MarkDirty();
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

        if (scene && cameraState.valid && ImGui::IsItemHovered() && !m_gizmoDrag.active)
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
        }

        DrawAndHandleTranslateGizmo(scene, imageMin, imageSize, cameraState);
        HandlePicking(scene, imageMin, imageSize, cameraState);

        ImGui::SetCursorScreenPos(ImVec2(imageMin.x + 12.0f, imageMin.y + 12.0f));
        ImGui::BeginChild("viewport_overlay", ImVec2(260.0f, 72.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::TextUnformatted("Viewport");
        ImGui::TextUnformatted("Left click: pick | Drag axis: move");
        ImGui::TextUnformatted("Right drag: orbit | Middle drag: pan");
        ImGui::TextUnformatted("Shift + Right drag: dolly");
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