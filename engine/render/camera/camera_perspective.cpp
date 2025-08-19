#include "camera_perspective.hpp"

PerspectiveCamera::PerspectiveCamera(const PerspectiveCamera::Frustrum &frustrum, const glm::vec3 &position, const glm::vec3 &lookAt, const glm::vec3 &upVector)
{
    m_cameraFrustrum = frustrum;
    m_position = position;
    m_lookAt = lookAt;
    m_upVector = upVector;

    RecalculateMatrix();
}

void PerspectiveCamera::RecalculateMatrix()
{
    m_projectionMatrix = glm::perspective(glm::radians(m_cameraFrustrum.angle), m_cameraFrustrum.width / m_cameraFrustrum.height, m_cameraFrustrum.near, m_cameraFrustrum.far);
    m_viewMatrix = glm::lookAt(m_position, m_lookAt, m_upVector);
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
}
