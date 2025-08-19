#pragma once

#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    Camera() = default;
    ~Camera() = default;

    const glm::mat4 &GetProjectionMatrix() const
    {
        return m_projectionMatrix;
    }
    const glm::mat4 &GetViewMatrix() const
    {
        return m_viewMatrix;
    }
    const glm::mat4 &GetViewProjectionMatrix() const
    {
        return m_viewProjectionMatrix;
    }

    // Set/Get position
    const glm::vec3 &GetPosition() const
    {
        return m_position;
    }
    void SetPosition(const glm::vec3 &pos)
    {
        m_position = pos;
        RecalculateMatrix();
    }

protected:
    virtual void RecalculateMatrix() = 0;

protected:
    Camera(const Camera &camera)
    {
        m_projectionMatrix = camera.m_projectionMatrix;
        m_viewMatrix = camera.m_viewMatrix;
        m_position = camera.m_position;
        m_viewProjectionMatrix = camera.m_viewProjectionMatrix;
    }

protected:
    glm::mat4 m_projectionMatrix = glm::mat4(1.0f);
    glm::mat4 m_viewMatrix = glm::mat4(1.0f);
    glm::mat4 m_viewProjectionMatrix = glm::mat4(1.0f);
    glm::vec3 m_position = glm::vec3(0.0f);
};