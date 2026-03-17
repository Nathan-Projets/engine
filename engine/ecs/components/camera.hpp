#pragma once

#include "../component.hpp"
#include "../../render/camera/camera_projection.hpp"
#include "../../render/camera/perspective_projection.hpp"

// TODO: extend this component for orthogonal projection, or refactor into two components PerspectiveCamera/OrthogonalCamera
class Camera : public Component
{
public:
    Camera() = default;

    explicit Camera(const PerspectiveProjection &projection, bool isMain = false) : projection(projection), main(isMain)
    {
    }

    explicit Camera(const PerspectiveProjection::Frustrum &frustrum,
                    const glm::vec3 &position = glm::vec3(0.0f),
                    const glm::vec3 &lookAt = glm::vec3(-1.0f),
                    const glm::vec3 &upVector = glm::vec3(0.0f, 1.0f, 0.0f),
                    bool isMain = false) : projection(frustrum, position, lookAt, upVector), main(isMain)
    {
    }

    PerspectiveProjection &GetProjection()
    {
        return projection;
    }

    const PerspectiveProjection &GetProjection() const
    {
        return projection;
    }

    PerspectiveProjection projection;
    bool main = false; // when true the render system uploads this camera
};