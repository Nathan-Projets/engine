#pragma once

#include "../system.hpp"
#include "../components/transform.hpp"
#include "../components/mesh_renderer.hpp"
#include "../components/light.hpp"
#include "../components/camera.hpp"
#include "../../render/renderer.hpp"
#include "../../resources/resource_manager.hpp"

#include <algorithm>
#include <optional>
#include <variant>

/**
 * @class RenderSystem
 * @brief ECS system that drives rendering of all entities with render components
 * @details
 * 
 * The RenderSystem manages the material-driven rendering pipeline:
 * 
 * ## Key Responsibilities
 * - **Material Resolution**: For each entity with a material handle, resolves:
 *   - Shader handle (from material's shader path if not yet loaded)
 *   - Texture handles (from material's texture paths for each slot)
 *   - Material properties (roughness, metallic, baseColor, emissive) extracted from variant map
 * 
 * - **Render Unit Submission**: Builds RenderUnit structures containing:
 *   - Mesh pointer
 *   - Shader pointer
 *   - Material pointer
 *   - Synchronized material properties
 *   - World transform (model matrix)
 * 
 * - **Rendering Execution**: Renders all opaque and unlit passes via Renderer
 * 
 * ## Material Property Resolution
 * Material properties are stored as variant<bool, int32, uint32, float, vec2, vec3, vec4>.
 * The render system extracts typed properties using std::get_if:
 * - "roughness" → float
 * - "metallic" → float
 * - "baseColor" → vec3
 * - "emissive" → vec3
 * 
 * Missing properties fall back to component-level defaults.
 * 
 * ## Texture Slot Binding
 * Textures are resolved lazily per frame for each material slot:
 * - BaseColor, Normal, MetallicRoughness, Occlusion, Emissive
 * Handles are cached on the material to avoid re-loading same textures.
 * 
 * @see Material for texture slot definitions
 * @see MeshRenderer for per-entity render state
 */
class RenderSystem : public System
{
public:
    /// @brief Construct a render system with the given resource manager
    /// @param resourceManager Pointer to the main resource manager (nullptr is unsafe but allowed)
    explicit RenderSystem(resources::ResourceManager *resourceManager = nullptr) : m_resourceManager(resourceManager) {}

    void Update(World &world, float deltaTime) override
    {
    }

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
        auto cameraEntities = world.GetEntitiesWith<Camera>();
        for (Entity cameraEntity : cameraEntities)
        {
            Camera *cameraComponent = world.GetComponent<Camera>(cameraEntity);
            Transform *cameraTransform = world.GetComponent<Transform>(cameraEntity);
            if (cameraComponent && cameraTransform && cameraComponent->main)
            {
                activeCameraProjection = &cameraComponent->GetProjection();
                activeCameraTransform = cameraTransform;
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

        for (Entity entity : entities)
        {
            Transform *transform = world.GetComponent<Transform>(entity);
            MeshRenderer *renderer = world.GetComponent<MeshRenderer>(entity);

            if (!transform || !renderer)
                continue;

            const glm::mat4 model = transform->GetMatrix();

            if (m_resourceManager && renderer->UsesResourcePipeline())
            {
                resources::Mesh *mesh = m_resourceManager->Get(renderer->GetMeshHandle());
                resources::Material *material = nullptr;
                if (renderer->GetMaterialHandle().IsValid())
                {
                    material = m_resourceManager->Get(renderer->GetMaterialHandle());
                    if (material)
                    {
                        if (!material->GetShaderHandle().IsValid() && !material->GetShaderPath().empty())
                        {
                            material->SetShaderHandle(m_resourceManager->Load<resources::Shader>(material->GetShaderPath()));
                        }

                        for (const auto &[slot, texturePath] : material->GetTexturePaths())
                        {
                            const std::optional<resources::Handle<resources::Texture>> existing = material->GetTextureHandle(slot);
                            if (!existing.has_value() || !existing.value().IsValid())
                            {
                                material->SetTextureHandle(slot, m_resourceManager->Load<resources::Texture>(texturePath));
                            }
                        }

                        if (!renderer->GetShaderHandle().IsValid() && material->GetShaderHandle().IsValid())
                        {
                            renderer->SetShaderHandle(material->GetShaderHandle());
                        }
                    }
                }

                resources::Shader *shader = m_resourceManager->Get(renderer->GetShaderHandle());
                if (mesh && shader)
                {
                    RenderUnit unit;
                    unit.resourceMesh = mesh;
                    unit.resourceShader = shader;
                    unit.resourceMaterial = material;
                    unit.model = model;
                    unit.material = renderer->GetMaterialData();

                    if (material)
                    {
                        const auto &properties = material->GetProperties();
                        auto roughnessIt = properties.find("roughness");
                        if (roughnessIt != properties.end())
                        {
                            if (const float *roughness = std::get_if<float>(&roughnessIt->second))
                            {
                                unit.material.roughness = *roughness;
                            }
                        }

                        auto metallicIt = properties.find("metallic");
                        if (metallicIt != properties.end())
                        {
                            if (const float *metallic = std::get_if<float>(&metallicIt->second))
                            {
                                unit.material.metallic = *metallic;
                            }
                        }

                        auto baseColorIt = properties.find("baseColor");
                        if (baseColorIt != properties.end())
                        {
                            if (const glm::vec3 *baseColor = std::get_if<glm::vec3>(&baseColorIt->second))
                            {
                                unit.material.baseColor = *baseColor;
                            }
                        }

                        auto emissiveIt = properties.find("emissive");
                        if (emissiveIt != properties.end())
                        {
                            if (const glm::vec3 *emissive = std::get_if<glm::vec3>(&emissiveIt->second))
                            {
                                unit.material.emissive = *emissive;
                            }
                        }
                    }

                    m_renderer.Submit(unit, renderer->GetQueue());
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
