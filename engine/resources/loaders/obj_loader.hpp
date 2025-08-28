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
        std::string key = "";
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