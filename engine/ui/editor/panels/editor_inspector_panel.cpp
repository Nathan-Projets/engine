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

    void TrackItemEdit(bool &started, bool &finished)
    {
        started = started || ImGui::IsItemActivated();
        finished = finished || ImGui::IsItemDeactivatedAfterEdit();
    }

    void BeginSceneMutation(EditorContext *context, bool &started)
    {
        if (!started)
        {
            return;
        }

        context->BeginTransaction();
        started = false;
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
    if (m_context->IsPlaying())
    {
        ImGui::TextUnformatted("Play mode is active. Stop playback to edit scene data.");
    }

    ImGui::BeginDisabled(!m_context->CanEdit());
    if (ImGui::InputText("Name", m_nameBuffer.data(), m_nameBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (std::strcmp(m_nameBuffer.data(), state.name.c_str()) != 0)
        {
            m_context->ExecuteMutation([&](Scene *targetScene)
                                       { return targetScene->SetEditorEntityName(state.entity, m_nameBuffer.data()); });
        }
    }
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        if (std::strcmp(m_nameBuffer.data(), state.name.c_str()) != 0)
        {
            m_context->ExecuteMutation([&](Scene *targetScene)
                                       { return targetScene->SetEditorEntityName(state.entity, m_nameBuffer.data()); });
        }
    }

    ImGui::SeparatorText("Components");
    const bool hasAnyCollider = state.hasBoxCollider || state.hasSphereCollider || state.hasCapsuleCollider;
    if (!state.hasTransform && ImGui::Button("Add Transform"))
    {
        m_context->ExecuteMutation([&](Scene *targetScene)
                                   { return targetScene->AddEditorComponent(state.entity, "Transform"); });
    }
    if (!state.hasLight)
    {
        if (ImGui::Button("Add Light"))
        {
            m_context->ExecuteMutation([&](Scene *targetScene)
                                       { return targetScene->AddEditorComponent(state.entity, "Light"); });
        }
    }
    if (!state.hasCamera)
    {
        if (ImGui::Button("Add Camera"))
        {
            m_context->ExecuteMutation([&](Scene *targetScene)
                                       { return targetScene->AddEditorComponent(state.entity, "Camera"); });
        }
    }
    if (!state.hasAnimation)
    {
        if (ImGui::Button("Add Animation"))
        {
            m_context->ExecuteMutation([&](Scene *targetScene)
                                       { return targetScene->AddEditorComponent(state.entity, "Animation"); });
        }
    }
    if (!state.hasRigidbody)
    {
        if (ImGui::Button("Add Rigidbody"))
        {
            m_context->ExecuteMutation([&](Scene *targetScene)
                                       { return targetScene->AddEditorComponent(state.entity, "Rigidbody"); });
        }
    }
    if (!hasAnyCollider)
    {
        if (ImGui::Button("Add Box Collider"))
        {
            m_context->ExecuteMutation([&](Scene *targetScene)
                                       { return targetScene->AddEditorComponent(state.entity, "BoxCollider"); });
        }
        if (ImGui::Button("Add Sphere Collider"))
        {
            m_context->ExecuteMutation([&](Scene *targetScene)
                                       { return targetScene->AddEditorComponent(state.entity, "SphereCollider"); });
        }
        if (ImGui::Button("Add Capsule Collider"))
        {
            m_context->ExecuteMutation([&](Scene *targetScene)
                                       { return targetScene->AddEditorComponent(state.entity, "CapsuleCollider"); });
        }
    }

    ImGui::EndDisabled();

    if (state.hasTransform)
    {
        ImGui::SeparatorText("Transform");
        ImGui::BeginDisabled(!m_context->CanEdit());
        if (ImGui::SmallButton("Remove Transform"))
        {
            if (m_context->ExecuteMutation([&](Scene *targetScene)
                                           { return targetScene->RemoveEditorComponent(state.entity, "Transform"); }))
            {
                ImGui::EndDisabled();
                ImGui::End();
                return;
            }
        }
        EditorTransformState transform = state.transform;
        bool changed = false;
        bool started = false;
        bool finished = false;
        changed |= ImGui::DragFloat3("Position", &transform.position.x, 0.05f);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat3("Rotation", &transform.rotation.x, 0.5f);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat3("Scale", &transform.scale.x, 0.02f, 0.001f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        BeginSceneMutation(m_context, started);
        if (changed)
        {
            scene->SetEditorTransform(state.entity, transform);
        }
        if (finished)
        {
            m_context->CommitTransaction(true);
        }
        ImGui::EndDisabled();
    }

    if (state.hasLight)
    {
        ImGui::SeparatorText("Light");
        ImGui::BeginDisabled(!m_context->CanEdit());
        if (ImGui::SmallButton("Remove Light"))
        {
            if (m_context->ExecuteMutation([&](Scene *targetScene)
                                           { return targetScene->RemoveEditorComponent(state.entity, "Light"); }))
            {
                ImGui::EndDisabled();
                ImGui::End();
                return;
            }
        }
        EditorLightState light = state.light;
        bool changed = false;
        bool started = false;
        bool finished = false;
        changed |= ImGui::ColorEdit3("Color", &light.color.x);
        TrackItemEdit(started, finished);
        changed |= ImGui::ColorEdit3("Ambient", &light.ambient.x);
        TrackItemEdit(started, finished);
        changed |= ImGui::ColorEdit3("Diffuse", &light.diffuse.x);
        TrackItemEdit(started, finished);
        changed |= ImGui::ColorEdit3("Specular", &light.specular.x);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat("Intensity", &light.intensity, 0.05f, 0.0f, 100.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat3("Direction", &light.direction.x, 0.01f, -1.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        changed |= ImGui::Checkbox("Cast Shadows", &light.castShadows);
        TrackItemEdit(started, finished);

        if (light.type != 1)
        {
            changed |= ImGui::DragFloat("Constant", &light.constant, 0.01f, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            TrackItemEdit(started, finished);
            changed |= ImGui::DragFloat("Linear", &light.linear, 0.001f, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            TrackItemEdit(started, finished);
            changed |= ImGui::DragFloat("Quadratic", &light.quadratic, 0.001f, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            TrackItemEdit(started, finished);
        }

        if (light.type == 2)
        {
            changed |= ImGui::DragFloat("Inner Cutoff", &light.innerCutoff, 0.1f, 0.0f, 90.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
            TrackItemEdit(started, finished);
            changed |= ImGui::DragFloat("Outer Cutoff", &light.outerCutoff, 0.1f, 0.0f, 90.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
            TrackItemEdit(started, finished);
        }

        ImGui::Text("Type: %s", LightTypeLabel(light.type));
        BeginSceneMutation(m_context, started);
        if (changed)
        {
            scene->SetEditorLight(state.entity, light);
        }
        if (finished)
        {
            m_context->CommitTransaction(true);
        }
        ImGui::EndDisabled();
    }

    if (state.hasCamera)
    {
        ImGui::SeparatorText("Camera");
        ImGui::BeginDisabled(!m_context->CanEdit());
        if (ImGui::SmallButton("Remove Camera"))
        {
            if (m_context->ExecuteMutation([&](Scene *targetScene)
                                           { return targetScene->RemoveEditorComponent(state.entity, "Camera"); }))
            {
                ImGui::EndDisabled();
                ImGui::End();
                return;
            }
        }
        EditorCameraState camera = state.camera;
        bool changed = false;
        bool started = false;
        bool finished = false;
        changed |= ImGui::Checkbox("Main Camera", &camera.main);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat3("Look At", &camera.lookAt.x, 0.05f);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat3("Up Vector", &camera.upVector.x, 0.01f, -1.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat("FOV", &camera.angle, 0.1f, 1.0f, 179.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat("Near", &camera.nearPlane, 0.01f, 0.001f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat("Far", &camera.farPlane, 1.0f, 0.1f, 5000.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        ImGui::Text("Viewport: %.0f x %.0f", camera.width, camera.height);
        BeginSceneMutation(m_context, started);
        if (changed)
        {
            scene->SetEditorCamera(state.entity, camera);
        }
        if (finished)
        {
            m_context->CommitTransaction(true);
        }
        ImGui::EndDisabled();
    }

    if (state.hasRigidbody)
    {
        ImGui::SeparatorText("Rigidbody");
        ImGui::BeginDisabled(!m_context->CanEdit());
        if (ImGui::SmallButton("Remove Rigidbody"))
        {
            if (m_context->ExecuteMutation([&](Scene *targetScene)
                                           { return targetScene->RemoveEditorComponent(state.entity, "Rigidbody"); }))
            {
                ImGui::EndDisabled();
                ImGui::End();
                return;
            }
        }

        EditorRigidbodyState rigidbody = state.rigidbody;
        bool changed = false;
        bool started = false;
        bool finished = false;
        changed |= ImGui::Combo("Body Type", &rigidbody.type, "Static\0Dynamic\0Kinematic\0");
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat("Mass", &rigidbody.mass, 0.05f, 0.001f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat("Gravity Scale", &rigidbody.gravityScale, 0.05f, -10.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat("Linear Damping", &rigidbody.linearDamping, 0.01f, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat("Angular Damping", &rigidbody.angularDamping, 0.01f, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        changed |= ImGui::Checkbox("Use Gravity", &rigidbody.useGravity);
        TrackItemEdit(started, finished);
        changed |= ImGui::Checkbox("Lock Rotation", &rigidbody.lockRotation);
        TrackItemEdit(started, finished);
        changed |= ImGui::Checkbox("Trigger Body", &rigidbody.isTrigger);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat3("Linear Velocity", &rigidbody.linearVelocity.x, 0.05f);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat3("Angular Velocity", &rigidbody.angularVelocity.x, 0.05f);
        TrackItemEdit(started, finished);
        BeginSceneMutation(m_context, started);
        if (changed)
        {
            scene->SetEditorRigidbody(state.entity, rigidbody);
        }
        if (finished)
        {
            m_context->CommitTransaction(true);
        }
        ImGui::EndDisabled();
    }

    if (state.hasBoxCollider)
    {
        ImGui::SeparatorText("Box Collider");
        ImGui::BeginDisabled(!m_context->CanEdit());
        if (ImGui::SmallButton("Remove Box Collider"))
        {
            if (m_context->ExecuteMutation([&](Scene *targetScene)
                                           { return targetScene->RemoveEditorComponent(state.entity, "BoxCollider"); }))
            {
                ImGui::EndDisabled();
                ImGui::End();
                return;
            }
        }

        EditorBoxColliderState collider = state.boxCollider;
        bool changed = false;
        bool started = false;
        bool finished = false;
        changed |= ImGui::DragFloat3("Center", &collider.center.x, 0.05f);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat3("Half Extents", &collider.halfExtents.x, 0.02f, 0.001f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        changed |= ImGui::Checkbox("Trigger", &collider.isTrigger);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat("Friction", &collider.material.friction, 0.01f, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat("Restitution", &collider.material.restitution, 0.01f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        BeginSceneMutation(m_context, started);
        if (changed)
        {
            scene->SetEditorBoxCollider(state.entity, collider);
        }
        if (finished)
        {
            m_context->CommitTransaction(true);
        }
        ImGui::EndDisabled();
    }

    if (state.hasSphereCollider)
    {
        ImGui::SeparatorText("Sphere Collider");
        ImGui::BeginDisabled(!m_context->CanEdit());
        if (ImGui::SmallButton("Remove Sphere Collider"))
        {
            if (m_context->ExecuteMutation([&](Scene *targetScene)
                                           { return targetScene->RemoveEditorComponent(state.entity, "SphereCollider"); }))
            {
                ImGui::EndDisabled();
                ImGui::End();
                return;
            }
        }

        EditorSphereColliderState collider = state.sphereCollider;
        bool changed = false;
        bool started = false;
        bool finished = false;
        changed |= ImGui::DragFloat3("Sphere Center", &collider.center.x, 0.05f);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat("Radius", &collider.radius, 0.02f, 0.001f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        changed |= ImGui::Checkbox("Sphere Trigger", &collider.isTrigger);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat("Sphere Friction", &collider.material.friction, 0.01f, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat("Sphere Restitution", &collider.material.restitution, 0.01f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        BeginSceneMutation(m_context, started);
        if (changed)
        {
            scene->SetEditorSphereCollider(state.entity, collider);
        }
        if (finished)
        {
            m_context->CommitTransaction(true);
        }
        ImGui::EndDisabled();
    }

    if (state.hasCapsuleCollider)
    {
        ImGui::SeparatorText("Capsule Collider");
        ImGui::BeginDisabled(!m_context->CanEdit());
        if (ImGui::SmallButton("Remove Capsule Collider"))
        {
            if (m_context->ExecuteMutation([&](Scene *targetScene)
                                           { return targetScene->RemoveEditorComponent(state.entity, "CapsuleCollider"); }))
            {
                ImGui::EndDisabled();
                ImGui::End();
                return;
            }
        }

        EditorCapsuleColliderState collider = state.capsuleCollider;
        bool changed = false;
        bool started = false;
        bool finished = false;
        changed |= ImGui::DragFloat3("Capsule Center", &collider.center.x, 0.05f);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat("Capsule Radius", &collider.radius, 0.02f, 0.001f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat("Capsule Height", &collider.height, 0.02f, 0.001f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        changed |= ImGui::Checkbox("Capsule Trigger", &collider.isTrigger);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat("Capsule Friction", &collider.material.friction, 0.01f, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        changed |= ImGui::DragFloat("Capsule Restitution", &collider.material.restitution, 0.01f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        TrackItemEdit(started, finished);
        BeginSceneMutation(m_context, started);
        if (changed)
        {
            scene->SetEditorCapsuleCollider(state.entity, collider);
        }
        if (finished)
        {
            m_context->CommitTransaction(true);
        }
        ImGui::EndDisabled();
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
        ImGui::BeginDisabled(!m_context->CanEdit());
        if (ImGui::SmallButton("Remove Animation"))
        {
            if (m_context->ExecuteMutation([&](Scene *targetScene)
                                           { return targetScene->RemoveEditorComponent(state.entity, "Animation"); }))
            {
                ImGui::EndDisabled();
                ImGui::End();
                return;
            }
        }
        ImGui::EndDisabled();
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