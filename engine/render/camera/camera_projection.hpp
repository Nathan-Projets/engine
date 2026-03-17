#pragma once

#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class CameraProjection
{
public:
    CameraProjection() = default;
    ~CameraProjection() = default;

    const glm::mat4 &GetProjectionMatrix() const
    {
        return m_projectionMatrix;
    }

protected:
    virtual void RecalculateProjection() = 0;

protected:
    CameraProjection(const CameraProjection &camera)
    {
        m_projectionMatrix = camera.m_projectionMatrix;
    }

protected:
    glm::mat4 m_projectionMatrix = glm::mat4(1.0f);
};