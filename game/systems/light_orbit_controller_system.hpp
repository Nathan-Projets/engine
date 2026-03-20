#pragma once

#include <glm/glm.hpp>

#include <ecs/system.hpp>
#include <ecs/components/light.hpp>
#include <ecs/components/transform.hpp>

#include "../components/light_orbit_controller.hpp"

class LightOrbitControllerSystem : public System
{
public:
    void Update(World &world, float deltaTime) override
    {
        if (deltaTime <= 0.0f)
            return;

        auto entities = world.GetEntitiesWith<Transform, Light, LightOrbitController>();
        for (Entity entity : entities)
        {
            Transform *transform = world.GetComponent<Transform>(entity);
            Light *light = world.GetComponent<Light>(entity);
            LightOrbitController *controller = world.GetComponent<LightOrbitController>(entity);
            if (!transform || !light || !controller)
                continue;

            if (!controller->initialized)
            {
                controller->initialized = true;
                controller->center = transform->position;
            }

            controller->angles += controller->angularSpeeds * deltaTime;

            transform->position = controller->center + glm::vec3(
                                                           controller->axisRadii.x * std::cos(controller->angles.x),
                                                           controller->axisRadii.y * std::sin(controller->angles.y),
                                                           controller->axisRadii.z * std::sin(controller->angles.z));

            const glm::vec3 offsetFromCenter = transform->position - controller->center;
            if (glm::length(offsetFromCenter) > 0.0001f)
            {
                light->direction = glm::normalize(controller->center - transform->position);
            }
        }
    }
};