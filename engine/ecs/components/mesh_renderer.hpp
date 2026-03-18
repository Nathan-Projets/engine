#pragma once

#include <string>

#include "../component.hpp"
#include "../../render/render_queue.hpp"
#include "../../render/shader_domain.hpp"
#include "../../resources/handle.hpp"
#include "../../resources/units/mesh.hpp"
#include "../../resources/units/shader.hpp"
#include "../../resources/units/material.hpp"

class MeshRenderer : public Component
{
public:
    MeshRenderer() : m_materialData(), m_queue(RenderQueue::Opaque) {}

    resources::Handle<resources::Mesh> GetMeshHandle() const noexcept { return m_meshHandle; }
    void SetMeshHandle(resources::Handle<resources::Mesh> meshHandle) noexcept { m_meshHandle = meshHandle; }

    resources::Handle<resources::Shader> GetShaderHandle() const noexcept { return m_shaderHandle; }
    void SetShaderHandle(resources::Handle<resources::Shader> shaderHandle) noexcept { m_shaderHandle = shaderHandle; }

    resources::Handle<resources::Material> GetMaterialHandle() const noexcept { return m_materialHandle; }
    void SetMaterialHandle(resources::Handle<resources::Material> materialHandle) noexcept { m_materialHandle = materialHandle; }

    bool UsesResourcePipeline() const noexcept
    {
        return m_meshHandle.IsValid() && (m_shaderHandle.IsValid() || m_materialHandle.IsValid());
    }

    const MaterialData &GetMaterialData() const { return m_materialData; }
    void SetMaterialData(const MaterialData &materialData) { m_materialData = materialData; }

    RenderQueue GetQueue() const { return m_queue; }
    void SetQueue(RenderQueue queue) { m_queue = queue; }

private:
    resources::Handle<resources::Mesh> m_meshHandle;
    resources::Handle<resources::Shader> m_shaderHandle;
    resources::Handle<resources::Material> m_materialHandle;
    MaterialData m_materialData;
    RenderQueue m_queue;
};
