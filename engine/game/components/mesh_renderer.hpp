#pragma once

#include <memory>
#include <string>

#include "../ecs/component.hpp"

class Mesh;
class Shader;

class MeshRenderer : public Component
{
public:
    MeshRenderer() : m_meshes(), m_shader(nullptr) {}

    MeshRenderer(std::vector<Mesh *> meshes, Shader *shader) : m_meshes(meshes), m_shader(shader) {}

    const std::vector<Mesh *> &GetMeshes() const { return m_meshes; }
    void SetMeshes(std::vector<Mesh *> meshes) { m_meshes = meshes; }
    void AddMesh(Mesh *mesh) { m_meshes.push_back(mesh); }

    Shader *GetShader() const { return m_shader; }
    void SetShader(Shader *shader) { m_shader = shader; }

private:
    std::vector<Mesh *> m_meshes;
    Shader *m_shader;
};
