#pragma once

#include <string>
#include <print>

#include "camera_projection.hpp"

#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class PerspectiveProjection : public CameraProjection
{
public:
    struct Frustrum
    {
        float angle;
        float width;
        float height;
        float near;
        float far;
    };

public:
    PerspectiveProjection(const PerspectiveProjection::Frustrum &frustrum = {45.0f, -1.0f, 1.0f, 1.0f, -1.0f},
                      const glm::vec3 &lookAt = glm::vec3(-1.0f),
                      const glm::vec3 &upVector = glm::vec3(0.0f, 1.0f, 0.0f));

    PerspectiveProjection(const PerspectiveProjection &camera) : CameraProjection(camera)
    {
        m_lookAt = camera.m_lookAt;
        m_upVector = camera.m_upVector;
        m_cameraFrustrum = camera.m_cameraFrustrum;
    }

    ~PerspectiveProjection() = default;

    void SetFrustrum(const Frustrum &frustrum)
    {
        m_cameraFrustrum = frustrum;
        RecalculateProjection();
    }

    void SetLookAt(const glm::vec3 &lookAt)
    {
        m_lookAt = lookAt;
    }

    void SetUpVector(const glm::vec3 &upVector)
    {
        m_upVector = upVector;
    }

    const glm::vec3 &GetLookAt() const
    {
        return m_lookAt;
    }

    const glm::vec3 &GetUpVector() const
    {
        return m_upVector;
    }

    // TODO: check if I can only build it when camera change or something, right now with the new changes it recalculates everything each frame
    glm::mat4 BuildViewMatrix(const glm::vec3 &position) const
    {
        return glm::lookAt(position, m_lookAt, m_upVector);
    }

    glm::mat4 BuildViewProjectionMatrix(const glm::vec3 &position) const
    {
        return m_projectionMatrix * BuildViewMatrix(position);
    }

protected:
    void RecalculateProjection() override;

protected:
    glm::vec3 m_lookAt;
    glm::vec3 m_upVector;
    Frustrum m_cameraFrustrum;
};