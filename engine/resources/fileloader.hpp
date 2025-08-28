#pragma once

#include <print>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <optional>
#include <iostream>
#include <filesystem>
#include <unordered_map>

#include <glm/glm.hpp>

class Mesh;
struct FaceIndex;

namespace tools
{
    std::string LoadFile(const std::string &iFilepath);
    std::vector<Mesh> LoadFileOBJ(const std::string &iFilepath);
    FaceIndex parseFaceVertex(const std::string &token);
    glm::vec2 ParseVec2(const std::string &line, size_t startPos = 0);
    glm::vec3 ParseVec3(const std::string &line, size_t startPos = 0);

    std::string ltrim(const std::string &s);
    std::string rtrim(const std::string &s);
    std::string trim(const std::string &s);
};
