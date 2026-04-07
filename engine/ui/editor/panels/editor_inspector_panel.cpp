#include "editor_inspector_panel.hpp"

#include <algorithm>
#include <cstring>

#include <imgui.h>

namespace
{
    const char *LightTypeLabel(int type)
    {
        switch (type)
        {
        case 1:
            return "Directional";
        case 2:
            return "Spot";
        default:
            return "Point";
        }
    }
}

void EditorInspectorPanel::SetContext(EditorContext *context)
{
    m_context = context;
}

void EditorInspectorPanel::SyncNameBuffer(const EditorEntityInspectorState &state)
{
    if (m_nameBufferEntityId == state.entity.GetID() && std::strcmp(m_nameBuffer.data(), state.name.c_str()) == 0)
    {
        return;
    }

    m_nameBufferEntityId = state.entity.GetID();
    m_nameBuffer.fill('\0');
    const size_t copyLength = std::min(state.name.size(), m_nameBuffer.size() - 1);
    std::memcpy(m_nameBuffer.data(), state.name.data(), copyLength);
    m_nameBuffer[copyLength] = '\0';
}

void EditorInspectorPanel::Draw()
{
    ImGui::SetNextWindowSize(ImVec2(360.0f, 720.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Inspector");

    if (!m_context || !m_context->GetScene())
    {
        ImGui::TextUnformatted("No scene bound.");
        ImGui::End();
        return;
    }

    if (!m_context->HasSelection())
    {
        ImGui::TextUnformatted("Select an entity from the outliner.");
        ImGui::End();
        return;
    }

    Scene *scene = m_context->GetScene();
    const EditorEntityInspectorState state = scene->GetEditorEntityInspectorState(m_context->GetSelectedEntity());
    if (!state.valid)
    {
        ImGui::TextUnformatted("Selected entity is no longer valid.");
        if (ImGui::Button("Clear selection"))
        {
            m_context->ClearSelection();
        }
        ImGui::End();
        return;
    }

    SyncNameBuffer(state);

    ImGui::Text("Entity %u", state.entity.GetID());
    if (ImGui::InputText("Name", m_nameBuffer.data(), m_nameBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (scene->SetEditorEntityName(state.entity, m_nameBuffer.data()))
        {
            m_context->MarkDirty();
        }
    }
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        if (scene->SetEditorEntityName(state.entity, m_nameBuffer.data()))
        {
            m_context->MarkDirty();
        }
    }

    ImGui::SeparatorText("Components");
    if (!state.hasTransform && ImGui::Button("Add Transform"))
    {
        if (scene->AddEditorComponent(state.entity, "Transform"))
        {
            m_context->MarkDirty();
        }
    }
    if (!state.hasLight)
    {
        if (ImGui::Button("Add Light"))
        {
            if (scene->AddEditorComponent(state.entity, "Light"))
            {
                m_context->MarkDirty();
            }
        }
    }
    if (!state.hasCamera)
    {
        if (ImGui::Button("Add Camera"))
        {
            if (scene->AddEditorComponent(state.entity, "Camera"))
            {
                m_context->MarkDirty();
            }
        }
    }
    if (!state.hasAnimation)
    {
        if (ImGui::Button("Add Animation"))
        {
            if (scene->AddEditorComponent(state.entity, "Animation"))
            {
                m_context->MarkDirty();
            }
        }
    }

    if (state.hasTransform)
    {
        ImGui::SeparatorText("Transform");
        if (ImGui::SmallButton("Remove Transform"))
        {
            if (scene->RemoveEditorComponent(state.entity, "Transform"))
            {
                m_context->MarkDirty();
                ImGui::End();
                return;
            }
        }
        EditorTransformState transform = state.transform;
        bool changed = false;
        changed |= ImGui::DragFloat3("Position", &transform.position.x, 0.05f);
        changed |= ImGui::DragFloat3("Rotation", &transform.rotation.x, 0.5f);
        changed |= ImGui::DragFloat3("Scale", &transform.scale.x, 0.02f, 0.001f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        if (changed)
        {
            if (scene->SetEditorTransform(state.entity, transform))
            {
                m_context->MarkDirty();
            }
        }
    }

    if (state.hasLight)
    {
        ImGui::SeparatorText("Light");
        if (ImGui::SmallButton("Remove Light"))
        {
            if (scene->RemoveEditorComponent(state.entity, "Light"))
            {
                m_context->MarkDirty();
                ImGui::End();
                return;
            }
        }
        EditorLightState light = state.light;
        bool changed = false;
        changed |= ImGui::ColorEdit3("Color", &light.color.x);
        changed |= ImGui::ColorEdit3("Ambient", &light.ambient.x);
        changed |= ImGui::ColorEdit3("Diffuse", &light.diffuse.x);
        changed |= ImGui::ColorEdit3("Specular", &light.specular.x);
        changed |= ImGui::DragFloat("Intensity", &light.intensity, 0.05f, 0.0f, 100.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
        changed |= ImGui::DragFloat3("Direction", &light.direction.x, 0.01f, -1.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        changed |= ImGui::Checkbox("Cast Shadows", &light.castShadows);

        if (light.type != 1)
        {
            changed |= ImGui::DragFloat("Constant", &light.constant, 0.01f, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            changed |= ImGui::DragFloat("Linear", &light.linear, 0.001f, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            changed |= ImGui::DragFloat("Quadratic", &light.quadratic, 0.001f, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        }

        if (light.type == 2)
        {
            changed |= ImGui::DragFloat("Inner Cutoff", &light.innerCutoff, 0.1f, 0.0f, 90.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
            changed |= ImGui::DragFloat("Outer Cutoff", &light.outerCutoff, 0.1f, 0.0f, 90.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
        }

        ImGui::Text("Type: %s", LightTypeLabel(light.type));
        if (changed)
        {
            if (scene->SetEditorLight(state.entity, light))
            {
                m_context->MarkDirty();
            }
        }
    }

    if (state.hasCamera)
    {
        ImGui::SeparatorText("Camera");
        if (ImGui::SmallButton("Remove Camera"))
        {
            if (scene->RemoveEditorComponent(state.entity, "Camera"))
            {
                m_context->MarkDirty();
                ImGui::End();
                return;
            }
        }
        EditorCameraState camera = state.camera;
        bool changed = false;
        changed |= ImGui::Checkbox("Main Camera", &camera.main);
        changed |= ImGui::DragFloat3("Look At", &camera.lookAt.x, 0.05f);
        changed |= ImGui::DragFloat3("Up Vector", &camera.upVector.x, 0.01f, -1.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        changed |= ImGui::DragFloat("FOV", &camera.angle, 0.1f, 1.0f, 179.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
        changed |= ImGui::DragFloat("Near", &camera.nearPlane, 0.01f, 0.001f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        changed |= ImGui::DragFloat("Far", &camera.farPlane, 1.0f, 0.1f, 5000.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::Text("Viewport: %.0f x %.0f", camera.width, camera.height);
        if (changed)
        {
            if (scene->SetEditorCamera(state.entity, camera))
            {
                m_context->MarkDirty();
            }
        }
    }

    if (state.hasMeshRenderer)
    {
        ImGui::SeparatorText("Render");
        ImGui::TextWrapped("Model: %s", state.meshRenderer.modelPath.empty() ? "(unresolved)" : state.meshRenderer.modelPath.c_str());
        ImGui::TextWrapped("Material: %s", state.meshRenderer.materialPath.empty() ? "(none)" : state.meshRenderer.materialPath.c_str());
        ImGui::TextWrapped("Shader: %s", state.meshRenderer.shaderPath.empty() ? "(none)" : state.meshRenderer.shaderPath.c_str());
        ImGui::Text("Queue: %d | Textures: %s | Resource path: %s",
                    state.meshRenderer.queue,
                    state.meshRenderer.loadTextures ? "on" : "off",
                    state.meshRenderer.usesResourcePipeline ? "yes" : "no");
    }

    if (state.hasAnimation)
    {
        ImGui::SeparatorText("Animation");
        if (ImGui::SmallButton("Remove Animation"))
        {
            if (scene->RemoveEditorComponent(state.entity, "Animation"))
            {
                m_context->MarkDirty();
                ImGui::End();
                return;
            }
        }
        ImGui::TextWrapped("Clip: %s", state.animation.clipName.empty() ? "(none)" : state.animation.clipName.c_str());
        ImGui::Text("Clip index: %d", state.animation.clipIndex);
        ImGui::Text("Speed: %.2f | Loop: %s | Paused: %s",
                    state.animation.speed,
                    state.animation.loop ? "yes" : "no",
                    state.animation.paused ? "yes" : "no");
        ImGui::Text("Transition active: %s", state.animation.transitionActive ? "yes" : "no");
    }

    ImGui::End();
}