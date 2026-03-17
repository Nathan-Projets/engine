#include "perspective_projection.hpp"

PerspectiveProjection::PerspectiveProjection(const PerspectiveProjection::Frustrum &frustrum, const glm::vec3 &lookAt, const glm::vec3 &upVector)
{
    m_cameraFrustrum = frustrum;
    m_lookAt = lookAt;
    m_upVector = upVector;

    RecalculateProjection();
}

void PerspectiveProjection::RecalculateProjection()
{
    m_projectionMatrix = glm::perspective(glm::radians(m_cameraFrustrum.angle), m_cameraFrustrum.width / m_cameraFrustrum.height, m_cameraFrustrum.near, m_cameraFrustrum.far);
}
