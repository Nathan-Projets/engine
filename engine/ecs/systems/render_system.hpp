#pragma once

#include "../ecs/system.hpp"
#include "../game/components/transform.hpp"
#include "../game/components/mesh_renderer.hpp"
#include "../game/components/light.hpp"
#include "../game/components/camera.hpp" // new camera component
#include "../render/shader.hpp"
#include "../render/mesh.hpp"

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

        const PerspectiveCamera *activeCamera = nullptr;
        auto cameraEntities = world.GetEntitiesWith<CameraComponent>();
        for (Entity cameraEntity : cameraEntities)
        {
            CameraComponent *cameraComponent = world.GetComponent<CameraComponent>(cameraEntity);
            if (cameraComponent && cameraComponent->main)
            {
                activeCamera = cameraComponent->camera;
                break;
            }
        }

        for (Entity entity : entities)
        {
            Transform *transform = world.GetComponent<Transform>(entity);
            MeshRenderer *renderer = world.GetComponent<MeshRenderer>(entity);

            if (!transform || !renderer || !renderer->GetShader() || renderer->GetMeshes().empty())
                continue;

            Shader &shader = *renderer->GetShader();
            const std::vector<Mesh *> &meshes = renderer->GetMeshes();

            shader.Use();
            if (activeCamera)
            {
                shader.Upload("view", activeCamera->GetViewMatrix());
                shader.Upload("projection", activeCamera->GetProjectionMatrix());
                shader.Upload("viewPos", activeCamera->GetPosition());
            }
            // TODO: the name is hard coded I need to check how to pass the name (maybe in the Shader class directly ?)
            shader.Upload("model", transform->GetMatrix());

            if (!lights.empty())
            {
                Light *light = world.GetComponent<Light>(lights[0]);
                if (light)
                {
                    shader.Upload("light.position", light->position);
                    shader.Upload("light.ambient", light->ambient);
                    shader.Upload("light.diffuse", light->diffuse);
                    shader.Upload("light.specular", light->specular);
                }
            }

            for (Mesh *mesh : meshes)
            {
                if (mesh)
                    mesh->Draw(shader);
            }
        }
    }
};
