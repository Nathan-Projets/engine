#pragma once

#include <memory>
#include <string>

#include "../component.hpp"

class Mesh;
class Shader;

using Meshes = std::vector<Mesh>;

class MeshRenderer : public Component
{
public:
    MeshRenderer() : m_meshes(), m_shader(nullptr) {}
    MeshRenderer(std::shared_ptr<Meshes> meshes, Shader *shader) : m_meshes(meshes), m_shader(shader) {}

    std::shared_ptr<Meshes> GetMeshes() const { return m_meshes; }
    void SetMeshes(std::shared_ptr<Meshes> meshes) { m_meshes = meshes; }

    Shader *GetShader() const { return m_shader; }
    void SetShader(Shader *shader) { m_shader = shader; }

private:
    std::shared_ptr<Meshes> m_meshes;
    Shader *m_shader;
};
