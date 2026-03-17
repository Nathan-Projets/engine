#include "uniform_buffer.hpp"

UniformBufferObject::UniformBufferObject(UniformBufferBinding binding, size_t dataSize) : m_binding(binding), m_size(dataSize)
{
    glGenBuffers(1, &m_handle);
    glBindBuffer(GL_UNIFORM_BUFFER, m_handle);
    glBufferData(GL_UNIFORM_BUFFER, dataSize, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    DEBUG("Created UBO for binding " << static_cast<int>(binding) << ", size: " << dataSize);
}

UniformBufferObject::~UniformBufferObject()
{
    if (m_handle != 0)
    {
        glDeleteBuffers(1, &m_handle);
    }
}

void UniformBufferObject::Update(const void *data)
{
    if (!data)
    {
        WARNING("Attempted to update UBO with null data.");
        return;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, m_handle);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, m_size, data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UniformBufferObject::BindToPoint()
{
    BindToPoint(static_cast<unsigned int>(m_binding));
}

void UniformBufferObject::BindToPoint(unsigned int bindPoint)
{
    glBindBufferBase(GL_UNIFORM_BUFFER, bindPoint, m_handle);
}

UniformBufferManager::UniformBufferManager()
{
    // Create one UBO for each standard binding point.
    // Sizes must match the GLSL layout declarations.
    m_frameBuffer = std::make_unique<UniformBufferObject>(UBO_FRAME_DATA, sizeof(FrameData));
    m_cameraBuffer = std::make_unique<UniformBufferObject>(UBO_CAMERA_DATA, sizeof(CameraData));
    m_objectBuffer = std::make_unique<UniformBufferObject>(UBO_OBJECT_DATA, sizeof(ObjectData));
    m_lightBuffer = std::make_unique<UniformBufferObject>(UBO_LIGHT_DATA, sizeof(LightData) * MAX_LIGHTS);
    m_materialBuffer = std::make_unique<UniformBufferObject>(UBO_MATERIAL_DATA, sizeof(MaterialData));

    DEBUG("UniformBufferManager initialized necessary buffers.");
}

UniformBufferManager::~UniformBufferManager()
{
}

void UniformBufferManager::UpdateFrameData(const FrameData &data)
{
    if (m_frameBuffer)
        m_frameBuffer->Update(&data);
}

void UniformBufferManager::UpdateCameraData(const CameraData &data)
{
    if (m_cameraBuffer)
        m_cameraBuffer->Update(&data);
}

void UniformBufferManager::UpdateObjectData(const ObjectData &data)
{
    if (m_objectBuffer)
        m_objectBuffer->Update(&data);
}

void UniformBufferManager::UpdateLightData(const LightData *lights, unsigned int count)
{
    if (!m_lightBuffer)
        return;

    if (count > MAX_LIGHTS)
    {
        WARNING("Light count " << count << " exceeds max " << MAX_LIGHTS << "; clamping.");
        count = MAX_LIGHTS;
    }

    // always rewrite the full light buffer so removed lights do not leave stale GPU values
    std::array<LightData, MAX_LIGHTS> packedLights{};
    for (unsigned int i = 0; i < count; ++i)
    {
        packedLights[i] = lights[i];
    }

    glBindBuffer(GL_UNIFORM_BUFFER, m_lightBuffer->GetHandle());
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightData) * packedLights.size(), packedLights.data());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UniformBufferManager::UpdateMaterialData(const MaterialData &data)
{
    if (m_materialBuffer)
        m_materialBuffer->Update(&data);
}

void UniformBufferManager::BindAllBuffers()
{
    // GL_UNIFORM_BUFFER bindings are indexed slots (0, 1, 2, ...), not a single global slot.
    // Each BindToPoint() writes one slot, so these calls do not override each other unless
    // two buffers use the same binding index.
    if (m_frameBuffer)
        m_frameBuffer->BindToPoint();
    if (m_cameraBuffer)
        m_cameraBuffer->BindToPoint();
    if (m_objectBuffer)
        m_objectBuffer->BindToPoint();
    if (m_lightBuffer)
        m_lightBuffer->BindToPoint();
    if (m_materialBuffer)
        m_materialBuffer->BindToPoint();
}
