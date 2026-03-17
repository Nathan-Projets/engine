#pragma once

#include "../system.hpp"
#include "../components/transform.hpp"
#include "../components/mesh_renderer.hpp"
#include "../components/light.hpp"
#include "../components/camera.hpp"
#include "../../render/shader.hpp"
#include "../../render/mesh.hpp"
#include "../../render/uniform_buffer.hpp"

#include <algorithm>

class RenderSystem : public System
{
public:
    void Update(World &world, float deltaTime) override
    {
    }

    void Render(World &world, float deltaTime) override
    {
        static UniformBufferManager uniformBuffers;

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

        FrameData frameData;
        frameData.deltaTime = deltaTime;
        uniformBuffers.UpdateFrameData(frameData);

        if (activeCameraProjection && activeCameraTransform)
        {
            const glm::mat4 view = activeCameraProjection->BuildViewMatrix(activeCameraTransform->position);
            const glm::mat4 projection = activeCameraProjection->GetProjectionMatrix();

            CameraData cameraData;
            cameraData.view = view;
            cameraData.projection = projection;
            cameraData.viewProjection = projection * view;
            cameraData.invViewProjection = glm::inverse(cameraData.viewProjection);
            cameraData.cameraPosition = activeCameraTransform->position;
            uniformBuffers.UpdateCameraData(cameraData);
        }

        if (!lights.empty())
        {
            constexpr size_t kMaxLights = 128;
            std::vector<LightData> lightData;
            lightData.reserve(std::min(lights.size(), kMaxLights));

            for (Entity lightEntity : lights)
            {
                if (lightData.size() >= kMaxLights)
                    break;

                Light *light = world.GetComponent<Light>(lightEntity);
                if (!light)
                    continue;

                Transform *lightTransform = world.GetComponent<Transform>(lightEntity);

                LightData gpuLight;
                if (lightTransform)
                    gpuLight.position = lightTransform->position;
                gpuLight.color = light->color;
                gpuLight.ambient = light->ambient;
                gpuLight.diffuse = light->diffuse;
                gpuLight.specular = light->specular;
                gpuLight.intensity = 1.0f;
                lightData.push_back(gpuLight);
            }

            uniformBuffers.UpdateLightData(lightData.data(), static_cast<unsigned int>(lightData.size()));
        }

        uniformBuffers.BindAllBuffers();

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

            const glm::mat4 model = transform->GetMatrix();
            shader.Upload("model", model);

            ObjectData objectData;
            objectData.model = model;
            objectData.normalMatrix = glm::mat4(glm::mat3(glm::transpose(glm::inverse(glm::mat3(model)))));
            uniformBuffers.UpdateObjectData(objectData);

            MaterialData materialData;
            materialData.roughness = 0.7f;
            materialData.metallic = 0.0f;
            uniformBuffers.UpdateMaterialData(materialData);

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

                    glm::vec3 position{0.0f};
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
