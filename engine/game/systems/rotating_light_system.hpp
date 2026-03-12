#pragma once

#include <GLFW/glfw3.h>

#include "../../ecs/system.hpp"
#include "../../game/components/transform.hpp"
#include "../../game/components/light.hpp"

class RotatingLightSystem : public System
{
public:
    RotatingLightSystem() : m_angle(0.0f), m_speed(glm::radians(45.0f))
    {
    }

    void Update(World &world, float deltaTime) override
    {
        if (deltaTime <= 0.0f)
            return;

        m_angle += m_speed * deltaTime;

        auto lights = world.GetEntitiesWith<Transform, Light>();
        for (Entity entity : lights)
        {
            Transform *transform = world.GetComponent<Transform>(entity);
            Light *light = world.GetComponent<Light>(entity);

            if (!transform || !light)
                continue;

            transform->rotation.y = glm::degrees(m_angle);

            transform->position.x = 2.0f * cos(m_angle);
            transform->position.z = 2.0f * sin(m_angle);

            light->position = transform->position;
        }
    }

private:
    float m_angle;
    float m_speed;
};
