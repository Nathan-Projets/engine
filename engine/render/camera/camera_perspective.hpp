#pragma once

#include <string>
#include <print>

#include "camera.hpp"

#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class PerspectiveCamera : public Camera
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
    PerspectiveCamera(const PerspectiveCamera::Frustrum &frustrum = {45.0f, -1.0f, 1.0f, 1.0f, -1.0f},
                      const glm::vec3 &position = glm::vec3(0.0f),
                      const glm::vec3 &lookAt = glm::vec3(-1.0f),
                      const glm::vec3 &upVector = glm::vec3(0.0f, 1.0f, 0.0f));

    PerspectiveCamera(const PerspectiveCamera &camera) : Camera(camera)
    {
        m_lookAt = camera.m_lookAt;
        m_upVector = camera.m_upVector;
        m_cameraFrustrum = camera.m_cameraFrustrum;
    }

    ~PerspectiveCamera() = default;

    void SetFrustrum(const Frustrum &frustrum)
    {
        m_cameraFrustrum = frustrum;
        RecalculateMatrix();
    }

    void SetLookAt(const glm::vec3 &lookAt)
    {
        m_lookAt = lookAt;
        RecalculateMatrix();
    }

    void SetUpVector(const glm::vec3 &upVector)
    {
        m_upVector = upVector;
        RecalculateMatrix();
    }

    void MoveForward(float velocity)
    {
        m_position += glm::normalize(m_lookAt - m_position) * velocity;
        RecalculateMatrix();
    }

    void MoveBackward(float velocity)
    {
        m_position -= glm::normalize(m_lookAt - m_position) * velocity;
        RecalculateMatrix();
    }

    void MoveLeft(float velocity)
    {
        glm::vec3 right = glm::normalize(glm::cross(m_lookAt - m_position, m_upVector));
        m_position -= right * velocity;
        RecalculateMatrix();
    }

    void MoveRight(float velocity)
    {
        glm::vec3 right = glm::normalize(glm::cross(m_lookAt - m_position, m_upVector));
        m_position += right * velocity;
        RecalculateMatrix();
    }

protected:
    void RecalculateMatrix();

protected:
    glm::vec3 m_lookAt;
    glm::vec3 m_upVector;
    Frustrum m_cameraFrustrum;
};