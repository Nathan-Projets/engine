#pragma once

#include <print>
#include <fstream>
#include <string>
#include <optional>
#include <filesystem>

#include <glm/glm.hpp>

#include <stb_image.h>

#include "auto_loader.hpp"
#include "render/mesh.hpp"
#include "helpers/log.hpp"

DEFINE_LOADER(OBJLoader, obj)
{
public:
    struct Triplet
    {
        int v = -1;
        int vt = -1;
        int vn = -1;

        bool operator==(const Triplet &other) const noexcept
        {
            return v == other.v && vt == other.vt && vn == other.vn;
        }
    };

    struct TripletHash
    {
        std::size_t operator()(const Triplet &t) const noexcept
        {
            std::size_t h1 = std::hash<int>{}(t.v);
            std::size_t h2 = std::hash<int>{}(t.vt);
            std::size_t h3 = std::hash<int>{}(t.vn);

            // mix bits (boost::hash_combine style)
            std::size_t seed = h1;
            seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };

    // struct Material
    // {
    //     std::string name = "";                       // newmtl
    //     glm::vec3 ambient_color = glm::vec3(-1.0f);  // Ka
    //     glm::vec3 diffuse_color = glm::vec3(-1.0f);  // Kd
    //     glm::vec3 emissive_color = glm::vec3(-1.0f); // Ke
    //     glm::vec3 specular_color = glm::vec3(-1.0f); // Ks
    //     float specular_exponent = -1.0f;             // Ns
    //     float optic_density = -1.0f;                 // Ni
    //     float alpha = -1.0f;                         // d

    //     Texture texture_ambiant;  // map_Ka
    //     Texture texture_diffuse;  // map_Kd
    //     Texture texture_specular; // map_Ks
    //     Texture texture_normal;   // map_Bump
    //     Texture texture_disp;     // disp
    //     Texture texture_stencil;  // decal
    // };

public:
    std::vector<Mesh> Load(const std::string &path) override;
    std::vector<Material> LoadMaterial(const std::string &path);
    std::optional<Texture> LoadTexture(const std::string &path);

private:
    std::optional<OBJLoader::Triplet> readTriplet(std::ifstream & stream);
    std::optional<glm::vec2> readVec2(std::ifstream & stream);
    std::optional<glm::vec3> readVec3(std::ifstream & stream);
    std::optional<float> readNumber(std::ifstream & stream);
    std::optional<int> readInteger(std::ifstream & stream, bool strict = true);
    std::string readLine(std::ifstream & stream);
    std::string readWord(std::ifstream & stream);
    void ignoreSpaces(std::ifstream & stream);
    void skipLine(std::ifstream & stream);
};