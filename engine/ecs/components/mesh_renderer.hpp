#pragma once

#include <memory>
#include <string>

#include "../component.hpp"
#include "../../render/render_queue.hpp"
#include "../../render/shader_domain.hpp"

class Mesh;
class Shader;

using Meshes = std::vector<Mesh>;

class MeshRenderer : public Component
{
public:
    MeshRenderer() : m_meshes(), m_shader(nullptr), m_materialData(), m_queue(RenderQueue::Opaque) {}
    MeshRenderer(std::shared_ptr<Meshes> meshes, Shader *shader) : m_meshes(meshes), m_shader(shader), m_materialData(), m_queue(RenderQueue::Opaque) {}

    std::shared_ptr<Meshes> GetMeshes() const { return m_meshes; }
    void SetMeshes(std::shared_ptr<Meshes> meshes) { m_meshes = meshes; }

    Shader *GetShader() const { return m_shader; }
    void SetShader(Shader *shader) { m_shader = shader; }

    const MaterialData &GetMaterialData() const { return m_materialData; }
    void SetMaterialData(const MaterialData &materialData) { m_materialData = materialData; }

    RenderQueue GetQueue() const { return m_queue; }
    void SetQueue(RenderQueue queue) { m_queue = queue; }

private:
    std::shared_ptr<Meshes> m_meshes;
    Shader *m_shader;
    MaterialData m_materialData;
    RenderQueue m_queue;
};
