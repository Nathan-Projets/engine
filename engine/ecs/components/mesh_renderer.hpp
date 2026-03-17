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
    MeshRenderer() : m_meshes(), m_shader(nullptr), m_shininess(32.0f) {}
    MeshRenderer(std::shared_ptr<Meshes> meshes, Shader *shader) : m_meshes(meshes), m_shader(shader), m_shininess(32.0f) {}

    std::shared_ptr<Meshes> GetMeshes() const { return m_meshes; }
    void SetMeshes(std::shared_ptr<Meshes> meshes) { m_meshes = meshes; }

    Shader *GetShader() const { return m_shader; }
    void SetShader(Shader *shader) { m_shader = shader; }

    float GetShininess() const { return m_shininess; }
    void SetShininess(float shininess) { m_shininess = shininess; }

private:
    std::shared_ptr<Meshes> m_meshes;
    Shader *m_shader;
    // TODO: for now I put the shininess here to avoid hardcoding it in the rendering pass but a whole refactor of the materials handling will be needed
    float m_shininess;
};
