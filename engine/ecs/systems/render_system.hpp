#pragma once

#include "../system.hpp"
#include "../components/transform.hpp"
#include "../components/mesh_renderer.hpp"
#include "../components/light.hpp"
#include "../components/camera.hpp"
#include "../../render/shader.hpp"
#include "../../render/mesh.hpp"

class RenderSystem : public System
{
public:
    void Update(World &world, float deltaTime) override
    {
    }

    void Render(World &world, float deltaTime) override
    {
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

        for (Entity entity : entities)
        {
            Transform *transform = world.GetComponent<Transform>(entity);
            MeshRenderer *renderer = world.GetComponent<MeshRenderer>(entity);

            if (!transform || !renderer || !renderer->GetShader() || !renderer->GetMeshes() || renderer->GetMeshes()->empty())
                continue;

            Shader &shader = *renderer->GetShader();
            std::shared_ptr<Meshes> meshes = renderer->GetMeshes();

            shader.Use();
            if (activeCameraProjection && activeCameraTransform)
            {
                shader.Upload("view", activeCameraProjection->BuildViewMatrix(activeCameraTransform->position));
                shader.Upload("projection", activeCameraProjection->GetProjectionMatrix());
                shader.Upload("viewPos", activeCameraTransform->position);
            }
            shader.Upload("model", transform->GetMatrix());

            if (!lights.empty())
            {
                Light *light = world.GetComponent<Light>(lights[0]);
                if (light)
                {
                    shader.Upload("light.ambient", light->ambient);
                    shader.Upload("light.diffuse", light->diffuse);
                    shader.Upload("light.specular", light->specular);
                    shader.Upload("color", light->color);
                    shader.Upload("material.shininess", renderer->GetShininess());
                    
                    glm::vec3 position {0.0f};
                    Transform *transform = world.GetComponent<Transform>(lights[0]);
                    if (transform)
                        position += transform->position;
                    shader.Upload("light.position", position);
                }
            }

            for (Mesh &mesh : *meshes)
            {
                mesh.Draw(shader);
            }
        }
    }
};
