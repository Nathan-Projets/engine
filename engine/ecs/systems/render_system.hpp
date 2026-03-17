#pragma once

#include "../system.hpp"
#include "../components/transform.hpp"
#include "../components/mesh_renderer.hpp"
#include "../components/light.hpp"
#include "../components/camera.hpp"
#include "../../render/mesh.hpp"
#include "../../render/renderer.hpp"

#include <algorithm>

class RenderSystem : public System
{
public:
    void Update(World &world, float deltaTime) override
    {
    }

    void Render(World &world, float deltaTime) override
    {
        if (!m_rendererInitialized)
        {
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

            if (!transform || !renderer || !renderer->GetShader() || !renderer->GetMeshes() || renderer->GetMeshes()->empty())
                continue;

            std::shared_ptr<Meshes> meshes = renderer->GetMeshes();

            const glm::mat4 model = transform->GetMatrix();

            for (Mesh &mesh : *meshes)
            {
                RenderUnit unit;
                unit.mesh = &mesh;
                unit.shader = renderer->GetShader();
                unit.model = model;
                unit.material = renderer->GetMaterialData();
                m_renderer.Submit(unit, renderer->GetQueue());
            }
        }

        m_renderer.EndFrame();
    }

private:
    Renderer m_renderer = {};
    bool m_rendererInitialized = false;
};
