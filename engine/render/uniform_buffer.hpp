#pragma once

#include <array>
#include <memory>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader_domain.hpp"
#include "helpers/log.hpp"

#define MAX_LIGHTS 128

/**
 * UniformBufferObject: Encapsulates a GPU buffer bound to a fixed UBO binding point.
 * Handles creation, updates, and cleanup.
 */
class UniformBufferObject
{
public:
    UniformBufferObject(UniformBufferBinding binding, size_t dataSize);
    ~UniformBufferObject();

    void Update(const void *data);

    void BindToPoint();
    void BindToPoint(unsigned int bindPoint);

    GLuint GetHandle() const { return m_handle; }
    UniformBufferBinding GetBinding() const { return m_binding; }
    size_t GetSize() const { return m_size; }

private:
    GLuint m_handle = 0;
    UniformBufferBinding m_binding;
    size_t m_size = 0;
};

/**
 * UniformBufferManager: Centralized management of standard uniform buffers.
 * Creates one buffer per standard binding point and updates them each frame.
 */
class UniformBufferManager
{
public:
    UniformBufferManager();
    ~UniformBufferManager();

    void UpdateFrameData(const FrameData &data);
    void UpdateCameraData(const CameraData &data);
    void UpdateObjectData(const ObjectData &data);
    void UpdateLightData(const LightData *lights, unsigned int count);
    void UpdateMaterialData(const MaterialData &data);

    void BindAllBuffers();

private:
    std::unique_ptr<UniformBufferObject> m_frameBuffer = nullptr;
    std::unique_ptr<UniformBufferObject> m_cameraBuffer = nullptr;
    std::unique_ptr<UniformBufferObject> m_objectBuffer = nullptr;
    std::unique_ptr<UniformBufferObject> m_lightBuffer = nullptr;
    std::unique_ptr<UniformBufferObject> m_materialBuffer = nullptr;
};
