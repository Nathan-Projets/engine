#pragma once

#include "../ecs/component.hpp"
#include "../render/camera/camera.hpp"
#include "../render/camera/camera_perspective.hpp"

// Simple component wrapper around a PerspectiveCamera. Not sure if I will keep it like this, for now it will be good enough.
// The renderer will look for the entity whose CameraComponent::main flag is set and use its view/projection matrices when drawing.
class CameraComponent : public Component
{
public:
    CameraComponent() = default;

    explicit CameraComponent(const PerspectiveCamera &cam, bool isMain = false) : camera(&cam), main(isMain)
    {
    }

    const PerspectiveCamera *camera;
    bool main = false; // when true the render system uploads this camera
};