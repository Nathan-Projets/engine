#pragma once

#include "../component.hpp"
#include "../../render/camera/camera.hpp"
#include "../../render/camera/camera_perspective.hpp"

// Simple component wrapper around a PerspectiveCamera. Not sure if I will keep it like this, for now it will be good enough.
// The renderer will look for the entity whose CameraComponent::main flag is set and use its view/projection matrices when drawing.
class CameraComponent : public Component
{
public:
    CameraComponent() = default;

    explicit CameraComponent(const PerspectiveCamera &cam, bool isMain = false) : camera(cam), main(isMain)
    {
    }

    explicit CameraComponent(const PerspectiveCamera::Frustrum &frustrum,
                             const glm::vec3 &position = glm::vec3(0.0f),
                             const glm::vec3 &lookAt = glm::vec3(-1.0f),
                             const glm::vec3 &upVector = glm::vec3(0.0f, 1.0f, 0.0f),
                             bool isMain = false)
        : camera(frustrum, position, lookAt, upVector), main(isMain)
    {
    }

    PerspectiveCamera &GetCamera()
    {
        return camera;
    }

    const PerspectiveCamera &GetCamera() const
    {
        return camera;
    }

    PerspectiveCamera camera;
    bool main = false; // when true the render system uploads this camera
};