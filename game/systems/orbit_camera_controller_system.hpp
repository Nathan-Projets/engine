#pragma once

#include <cmath>

#include <glm/common.hpp>

#include <ecs/system.hpp>
#include <ecs/components/camera.hpp>
#include <ecs/components/transform.hpp>
#include <input/input_manager.hpp>

#include "../components/orbit_camera_controller.hpp"

class OrbitCameraControllerSystem : public System
{
public:
    void Update(World &world, float deltaTime) override
    {
        if (deltaTime <= 0.0f)
            return;

        InputManager &input = InputManager::Get();
        auto entities = world.GetEntitiesWith<Transform, Camera, OrbitCameraController>();
        for (Entity entity : entities)
        {
            Transform *transform = world.GetComponent<Transform>(entity);
            Camera *camera = world.GetComponent<Camera>(entity);
            OrbitCameraController *controller = world.GetComponent<OrbitCameraController>(entity);
            if (!transform || !camera || !controller)
                continue;

            PerspectiveProjection &projection = camera->GetProjection();
            if (!controller->orbitInitialized)
            {
                controller->orbitInitialized = true;

                const glm::vec3 offset = transform->position - controller->target;
                const float distance = glm::length(offset);

                if (distance > 0.0001f)
                {
                    controller->radius = distance;
                    controller->yaw = std::atan2(offset.z, offset.x);
                    controller->pitch = std::asin(glm::clamp(offset.y / distance, -1.0f, 1.0f));
                }
            }

            bool changed = false;

            if (input.IsActionActive("move_left"))
            {
                controller->yaw += controller->yawSpeed * deltaTime;
                changed = true;
            }

            if (input.IsActionActive("move_right"))
            {
                controller->yaw -= controller->yawSpeed * deltaTime;
                changed = true;
            }

            if (input.IsActionActive("move_forward"))
            {
                controller->pitch += controller->pitchSpeed * deltaTime;
                changed = true;
            }

            if (input.IsActionActive("move_backward"))
            {
                controller->pitch -= controller->pitchSpeed * deltaTime;
                changed = true;
            }

            // don't recompute the position for nothing
            if (!changed)
                continue;

            controller->pitch = glm::clamp(controller->pitch, -controller->maxPitch, controller->maxPitch);

            const float horizontalRadius = controller->radius * std::cos(controller->pitch);
            const glm::vec3 position = controller->target + glm::vec3(
                                                                horizontalRadius * std::cos(controller->yaw),
                                                                controller->radius * std::sin(controller->pitch),
                                                                horizontalRadius * std::sin(controller->yaw));

            transform->position = position;
            projection.SetLookAt(controller->target);
            projection.SetUpVector(controller->upVector);
        }
    }
};