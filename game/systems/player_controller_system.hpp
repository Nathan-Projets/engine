#pragma once

#include <cmath>

#include <ecs/components/camera.hpp>
#include <ecs/components/transform.hpp>
#include <ecs/system.hpp>
#include <input/input_manager.hpp>
#include <physics/rigidbody.hpp>

#include "../components/player_controller.hpp"

class PlayerControllerSystem : public System
{
public:
    void Update(World &world, float deltaTime) override
    {
        if (deltaTime <= 0.0f)
        {
            return;
        }

        Entity mainCameraEntity;
        for (Entity entity : world.GetEntitiesWith<Camera>())
        {
            Camera *camera = world.GetComponent<Camera>(entity);
            if (camera && camera->main)
            {
                mainCameraEntity = entity;
                break;
            }
        }

        Transform *mainCameraTransform = world.GetComponent<Transform>(mainCameraEntity);
        Camera *mainCamera = world.GetComponent<Camera>(mainCameraEntity);

        InputManager &input = InputManager::Get();
        glm::vec3 inputDirection(0.0f);
        if (input.IsActionActive("move_forward"))
        {
            inputDirection.z -= 1.0f;
        }
        if (input.IsActionActive("move_backward"))
        {
            inputDirection.z += 1.0f;
        }
        if (input.IsActionActive("move_left"))
        {
            inputDirection.x -= 1.0f;
        }
        if (input.IsActionActive("move_right"))
        {
            inputDirection.x += 1.0f;
        }

        if (glm::dot(inputDirection, inputDirection) > 0.0f)
        {
            inputDirection = glm::normalize(inputDirection);
        }

        for (Entity entity : world.GetEntitiesWith<Transform, PlayerController>())
        {
            Transform *transform = world.GetComponent<Transform>(entity);
            PlayerController *controller = world.GetComponent<PlayerController>(entity);
            Rigidbody *rigidbody = world.GetComponent<Rigidbody>(entity);
            if (!transform || !controller)
            {
                continue;
            }

            if (glm::dot(inputDirection, inputDirection) > 0.0f)
            {
                if (rigidbody)
                {
                    rigidbody->linearVelocity.x = inputDirection.x * controller->moveSpeed;
                    rigidbody->linearVelocity.z = inputDirection.z * controller->moveSpeed;
                }
                else
                {
                    transform->position += inputDirection * controller->moveSpeed * deltaTime;
                }

                const float targetYaw = glm::degrees(std::atan2(-inputDirection.x, -inputDirection.z));
                float deltaYaw = std::fmod(targetYaw - transform->rotation.y + 540.0f, 360.0f) - 180.0f;
                const float maxStep = controller->turnSpeedDegrees * deltaTime;
                deltaYaw = glm::clamp(deltaYaw, -maxStep, maxStep);
                transform->rotation.y += deltaYaw;
            }
            else if (rigidbody)
            {
                rigidbody->linearVelocity.x = 0.0f;
                rigidbody->linearVelocity.z = 0.0f;
            }

            if (mainCameraTransform && mainCamera)
            {
                const glm::vec3 focusPoint = transform->position + glm::vec3(0.0f, controller->lookAtHeight, 0.0f);
                const glm::vec3 forward = glm::normalize(controller->cameraForward);
                mainCameraTransform->position = focusPoint - forward * controller->cameraDistance + glm::vec3(0.0f, controller->cameraHeight, 0.0f);
                mainCamera->GetProjection().SetLookAt(focusPoint);
                mainCamera->GetProjection().SetUpVector(glm::vec3(0.0f, 1.0f, 0.0f));
            }
        }
    }
};