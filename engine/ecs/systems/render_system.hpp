#pragma once

#include "../system.hpp"
#include "../components/transform.hpp"
#include "../components/mesh_renderer.hpp"
#include "../components/light.hpp"
#include "../components/camera.hpp"
#include "../../render/renderer.hpp"
#include "../../resources/resource_manager.hpp"

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
        if (!m_rendererInitialized)
        {
            m_renderer.SetResourceManager(m_resourceManager);
            m_renderer.Initialize(0, 0);
            m_rendererInitialized = true;
        }

        auto lights = world.GetEntitiesWith<Light>();
        auto entities = world.GetEntitiesWith<Transform, MeshRenderer>();

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
            m_renderer.SubmitLight(lightData);
        }

        const std::array<resources::MaterialTextureSlot, 5> slots = {
            resources::MaterialTextureSlot::BaseColor,
            resources::MaterialTextureSlot::Normal,
            resources::MaterialTextureSlot::MetallicRoughness,
            resources::MaterialTextureSlot::Occlusion,
            resources::MaterialTextureSlot::Emissive};

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

                        for (const auto &[slot, texturePath] : material->GetTexturePaths())
                        {
                            const auto existing = material->GetTextureHandle(slot);
                            if (!existing.has_value() || !existing.value().IsValid())
                                material->SetTextureHandle(slot, m_resourceManager->Load<resources::Texture>(texturePath));
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

        m_renderer.EndFrame();
    }

private:
    Renderer m_renderer = {};
    bool m_rendererInitialized = false;
    resources::ResourceManager *m_resourceManager = nullptr;
};
