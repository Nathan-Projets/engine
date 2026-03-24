#pragma once

#include "../system.hpp"
#include "../components/transform.hpp"
#include "../components/mesh_renderer.hpp"
#include "../components/light.hpp"
#include "../components/camera.hpp"
#include "../components/skybox.hpp"
#include "../../render/renderer.hpp"
#include "../../resources/resource_manager.hpp"
#include "../../input/input_manager.hpp"
#include "../../helpers/log.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <optional>
#include <variant>

class RenderSystem : public System
{
public:
    explicit RenderSystem(resources::ResourceManager *resourceManager = nullptr) : m_resourceManager(resourceManager) {}

    void Update(World &world, float deltaTime) override {}

    void Render(World &world, float deltaTime) override
    {
        bool debugModeChanged = false;

        if (InputManager::Get().IsKeyJustPressed(GLFW_KEY_F3))
        {
            m_debugViewMode = (m_debugViewMode + 1u) % 6u;
            debugModeChanged = true;
        }

        if (InputManager::Get().IsKeyJustPressed(GLFW_KEY_1))
        {
            m_debugViewMode = 0u;
            debugModeChanged = true;
        }
        if (InputManager::Get().IsKeyJustPressed(GLFW_KEY_2))
        {
            m_debugViewMode = 1u;
            debugModeChanged = true;
        }
        if (InputManager::Get().IsKeyJustPressed(GLFW_KEY_3))
        {
            m_debugViewMode = 2u;
            debugModeChanged = true;
        }
        if (InputManager::Get().IsKeyJustPressed(GLFW_KEY_4))
        {
            m_debugViewMode = 3u;
            debugModeChanged = true;
        }
        if (InputManager::Get().IsKeyJustPressed(GLFW_KEY_5))
        {
            m_debugViewMode = 4u;
            debugModeChanged = true;
        }
        if (InputManager::Get().IsKeyJustPressed(GLFW_KEY_6))
        {
            m_debugViewMode = 5u;
            debugModeChanged = true;
        }

        if (debugModeChanged)
        {
            INFO("Debug view mode [" << m_debugViewMode << "]: " << GetDebugViewModeName(m_debugViewMode));
        }

        if (!m_rendererInitialized)
        {
            m_renderer.SetResourceManager(m_resourceManager);
            m_renderer.Initialize(0, 0);
            m_rendererInitialized = true;
        }

        auto lights = world.GetEntitiesWith<Light>();
        auto entities = world.GetEntitiesWith<Transform, MeshRenderer>();
        auto skyboxes = world.GetEntitiesWith<Skybox>();

        const PerspectiveProjection *activeCameraProjection = nullptr;
        const Transform *activeCameraTransform = nullptr;
        for (Entity cameraEntity : world.GetEntitiesWith<Camera>())
        {
            Camera *cam = world.GetComponent<Camera>(cameraEntity);
            Transform *t = world.GetComponent<Transform>(cameraEntity);
            if (cam && t && cam->main)
            {
                activeCameraProjection = &cam->GetProjection();
                activeCameraTransform = t;
                break;
            }
        }

        if (!activeCameraProjection || !activeCameraTransform)
            return;

        const glm::mat4 view = activeCameraProjection->BuildViewMatrix(activeCameraTransform->position);
        const glm::mat4 projection = activeCameraProjection->GetProjectionMatrix();

        CameraRenderData cameraData;
        cameraData.view = view;
        cameraData.projection = projection;
        cameraData.viewProjection = projection * view;
        cameraData.invViewProjection = glm::inverse(cameraData.viewProjection);
        cameraData.position = activeCameraTransform->position;

        m_renderer.BeginFrame(cameraData);

        for (Entity lightEntity : lights)
        {
            Light *light = world.GetComponent<Light>(lightEntity);
            if (!light)
                continue;
            Transform *lightTransform = world.GetComponent<Transform>(lightEntity);

            LightRenderData lightData;
            if (lightTransform)
                lightData.position = lightTransform->position;
            lightData.ambient = light->ambient;
            lightData.diffuse = light->diffuse;
            lightData.specular = light->specular;
            lightData.color = light->color;
            lightData.intensity = light->intensity;
            lightData.constant = light->constant;
            lightData.linear = light->linear;
            lightData.quadratic = light->quadratic;
            lightData.direction = light->direction;
            lightData.innerCutoff = light->innerCutoff;
            lightData.outerCutoff = light->outerCutoff;
            lightData.type = static_cast<unsigned int>(light->type);
            m_renderer.SubmitLight(lightData);
        }

        const std::array<resources::MaterialTextureSlot, 6> slots = {
            resources::MaterialTextureSlot::BaseColor,
            resources::MaterialTextureSlot::Normal,
            resources::MaterialTextureSlot::MetallicRoughness,
            resources::MaterialTextureSlot::Occlusion,
            resources::MaterialTextureSlot::Emissive,
            resources::MaterialTextureSlot::Displacement};

        for (Entity entity : entities)
        {
            Transform *transform = world.GetComponent<Transform>(entity);
            MeshRenderer *renderer = world.GetComponent<MeshRenderer>(entity);
            if (!transform || !renderer)
                continue;

            const glm::mat4 modelMatrix = transform->GetMatrix();

            if (m_resourceManager && renderer->UsesResourcePipeline())
            {
                resources::Model *model = m_resourceManager->Get(renderer->GetModelHandle());
                resources::Material *material = nullptr;

                if (renderer->GetMaterialHandle().IsValid())
                {
                    material = m_resourceManager->Get(renderer->GetMaterialHandle());
                    if (material)
                    {
                        if (!material->GetShaderHandle().IsValid() && !material->GetShaderPath().empty())
                            material->SetShaderHandle(m_resourceManager->Load<resources::Shader>(material->GetShaderPath()));

                        if (renderer->ShouldLoadTextures())
                        {
                            for (const auto &[slot, texturePath] : material->GetTexturePaths())
                            {
                                const auto existing = material->GetTextureHandle(slot);
                                if (!existing.has_value() || !existing.value().IsValid())
                                    material->SetTextureHandle(slot, m_resourceManager->Load<resources::Texture>(texturePath));
                            }
                        }

                        if (!renderer->GetShaderHandle().IsValid() && material->GetShaderHandle().IsValid())
                            renderer->SetShaderHandle(material->GetShaderHandle());
                    }
                }

                resources::Shader *shader = m_resourceManager->Get(renderer->GetShaderHandle());
                if (model && shader)
                {
                    RenderUnit baseUnit;
                    baseUnit.resourceModel = model;
                    baseUnit.resourceShader = shader;
                    baseUnit.resourceMaterial = material;
                    baseUnit.model = modelMatrix;
                    baseUnit.material = renderer->GetMaterialData();
                    baseUnit.material.features = m_debugViewMode;

                    if (material)
                    {
                        const auto &props = material->GetProperties();
                        if (auto it = props.find("roughness"); it != props.end())
                            if (const float *v = std::get_if<float>(&it->second))
                                baseUnit.material.roughness = *v;
                        if (auto it = props.find("metallic"); it != props.end())
                            if (const float *v = std::get_if<float>(&it->second))
                                baseUnit.material.metallic = *v;
                        if (auto it = props.find("baseColor"); it != props.end())
                            if (const glm::vec3 *v = std::get_if<glm::vec3>(&it->second))
                                baseUnit.material.baseColor = *v;
                        if (auto it = props.find("emissive"); it != props.end())
                            if (const glm::vec3 *v = std::get_if<glm::vec3>(&it->second))
                                baseUnit.material.emissive = *v;
                        if (auto it = props.find("displacementStrength"); it != props.end())
                            if (const float *v = std::get_if<float>(&it->second))
                                baseUnit.material.displacementStrength = *v;
                    }

                    const auto &primitiveInstances = model->GetPrimitiveInstances();
                    if (!primitiveInstances.empty())
                    {
                        for (const resources::MeshPrimitiveInstance &pi : primitiveInstances)
                        {
                            RenderUnit unit = baseUnit;
                            unit.primitiveIndex = pi.primitiveIndex;
                            unit.materialIndex = pi.materialIndex;
                            unit.localTransform = pi.localTransform;

                            for (resources::MaterialTextureSlot slot : slots)
                            {
                                if (!renderer->ShouldLoadTextures())
                                    continue;
                                if (material && material->HasTexturePath(slot))
                                    continue;
                                const auto importedPath = model->GetImportedMaterialTexturePath(pi.materialIndex, slot);
                                if (importedPath.has_value())
                                {
                                    unit.textureOverrides[static_cast<size_t>(slot)] = m_resourceManager->Load<resources::Texture>(importedPath.value());
                                    unit.textureUvIndices[static_cast<size_t>(slot)] = model->GetImportedMaterialTextureUvIndex(pi.materialIndex, slot);
                                }
                            }
                            m_renderer.Submit(unit, renderer->GetQueue());
                        }
                    }
                    else
                    {
                        for (resources::MaterialTextureSlot slot : slots)
                        {
                            if (!renderer->ShouldLoadTextures())
                                continue;
                            if (material && material->HasTexturePath(slot))
                                continue;
                            const auto importedPath = model->GetImportedMaterialTexturePath(0u, slot);
                            if (importedPath.has_value())
                            {
                                baseUnit.textureOverrides[static_cast<size_t>(slot)] = m_resourceManager->Load<resources::Texture>(importedPath.value());
                                baseUnit.textureUvIndices[static_cast<size_t>(slot)] = model->GetImportedMaterialTextureUvIndex(0u, slot);
                            }
                        }
                        m_renderer.Submit(baseUnit, renderer->GetQueue());
                    }
                    continue;
                }
            }
        }

        if (m_resourceManager)
        {
            for (Entity entity : skyboxes)
            {
                Skybox *skybox = world.GetComponent<Skybox>(entity);
                Transform *transform = world.GetComponent<Transform>(entity);
                if (!skybox || !skybox->modelHandle.IsValid() || !skybox->materialHandle.IsValid())
                    continue;

                resources::Model *model = m_resourceManager->Get(skybox->modelHandle);
                resources::Material *material = m_resourceManager->Get(skybox->materialHandle);
                if (!model || !material)
                    continue;

                if (!material->GetShaderHandle().IsValid() && !material->GetShaderPath().empty())
                    material->SetShaderHandle(m_resourceManager->Load<resources::Shader>(material->GetShaderPath()));

                for (const auto &[slot, texturePath] : material->GetTexturePaths())
                {
                    const auto existing = material->GetTextureHandle(slot);
                    if (!existing.has_value() || !existing.value().IsValid())
                        material->SetTextureHandle(slot, m_resourceManager->Load<resources::Texture>(texturePath));
                }

                resources::Shader *shader = m_resourceManager->Get(material->GetShaderHandle());
                if (!shader)
                    continue;

                RenderUnit unit;
                unit.resourceModel = model;
                unit.resourceShader = shader;
                unit.resourceMaterial = material;

                glm::mat4 rotation = glm::mat4(1.0f);
                if (transform)
                {
                    rotation = glm::rotate(rotation, glm::radians(transform->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                    rotation = glm::rotate(rotation, glm::radians(transform->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                    rotation = glm::rotate(rotation, glm::radians(transform->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                }

                unit.model = glm::translate(glm::mat4(1.0f), activeCameraTransform->position) * rotation * glm::scale(glm::mat4(1.0f), glm::vec3(skybox->scale));
                unit.material.baseColor = glm::vec3(1.0f);
                unit.material.emissive = glm::vec3(skybox->intensity);
                unit.material.features = m_debugViewMode;

                m_renderer.Submit(unit, RenderQueue::Unlit);
            }
        }

        m_renderer.EndFrame();
    }

private:
    static const char *GetDebugViewModeName(uint32_t mode)
    {
        switch (mode)
        {
        case 0:
            return "Lit";
        case 1:
            return "Albedo";
        case 2:
            return "Normals";
        case 3:
            return "Roughness";
        case 4:
            return "Metallic";
        case 5:
            return "AO";
        default:
            return "Unknown";
        }
    }

    Renderer m_renderer = {};
    bool m_rendererInitialized = false;
    resources::ResourceManager *m_resourceManager = nullptr;
    uint32_t m_debugViewMode = 0;
};
