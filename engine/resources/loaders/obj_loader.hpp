#pragma once

#include <print>
#include <fstream>
#include <string>
#include <optional>

#include <glm/glm.hpp>

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

public:
    std::optional<Mesh> Load(const std::string &path) override;

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