#include "mesh.hpp"

Mesh::Mesh() : vertices({}), indices({}), computedTangents(false)
{
}

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, bool computeTangents)
{
    this->vertices = vertices;
    this->indices = indices;
    this->computedTangents = computeTangents;

    SetupMesh(computeTangents);
}

void Mesh::Draw(Shader &shader) const
{
    shader.Use();

    int total = 0;
    unsigned int ambientNr = 0;
    unsigned int diffuseNr = 0;
    unsigned int specularNr = 0;
    unsigned int normalNr = 0;

    for (unsigned int i = 0; i < materials.size(); i++)
    {
        const Material &material = materials[i];
        if (material.texture_ambiant.type != TextureType::UNKNOWN)
        {
            glActiveTexture(GL_TEXTURE0 + total);
            shader.Upload(("material.ambient[" + std::to_string(ambientNr) + "]").c_str(), total);
            glBindTexture(GL_TEXTURE_2D, material.texture_ambiant.id);
            ambientNr++;
            total++;
        }
        if (material.texture_diffuse.type != TextureType::UNKNOWN)
        {
            glActiveTexture(GL_TEXTURE0 + total);
            shader.Upload(("material.diffuse[" + std::to_string(diffuseNr) + "]").c_str(), total);
            glBindTexture(GL_TEXTURE_2D, material.texture_diffuse.id);
            diffuseNr++;
            total++;
        }
        if (material.texture_specular.type != TextureType::UNKNOWN)
        {
            glActiveTexture(GL_TEXTURE0 + total);
            shader.Upload(("material.specular[" + std::to_string(specularNr) + "]").c_str(), total);
            glBindTexture(GL_TEXTURE_2D, material.texture_specular.id);
            specularNr++;
            total++;
        }
        if (material.texture_normal.type != TextureType::UNKNOWN)
        {
            glActiveTexture(GL_TEXTURE0 + total);
            shader.Upload(("material.normal[" + std::to_string(normalNr) + "]").c_str(), total);
            glBindTexture(GL_TEXTURE_2D, material.texture_normal.id);
            normalNr++;
            total++;
        }
    }

    shader.Upload("material.ambientCount", ambientNr);
    shader.Upload("material.diffuseCount", diffuseNr);
    shader.Upload("material.specularCount", specularNr);
    shader.Upload("material.normalCount", normalNr);

    if (computedTangents)
    {
        shader.Upload("use_tbn", 1.0f);
    }
    else
    {
        shader.Upload("use_tbn", 0.0f);
    }
    // shader.Upload("use_tbn", 0.0f);

    glActiveTexture(GL_TEXTURE0);

    // draw mesh
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::AddMaterials(const std::vector<Material> &iMaterials)
{
    materials.insert(materials.end(), iMaterials.begin(), iMaterials.end());
}

void Mesh::AddMaterial(const Material &iMaterial)
{
    materials.push_back(iMaterial);
}

void Mesh::SetupMesh(bool computeTangents)
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
    // vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
    // vertex texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, tcoords));

    // I don't like how this is done the mix between private member and flag given in the method doesn't sit right with me
    if (computeTangents)
    {
        computedTangents = computeTangents;
        SetupTangents();
    }
    else if (computedTangents)
    {
        SetupTangents();
    }

    // vertex tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, tangent));
    // vertex bitangent
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, bitangent));

    glBindVertexArray(0);
}

void Mesh::SetupTangents()
{
    // this is just not working I need to properly look into how to calculate these tangents, and bitangents
}