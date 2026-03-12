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

#include "helpers/log.hpp"

class Mesh;
struct FaceIndex;

namespace tools
{
    std::string LoadFile(const std::string &iFilepath);

    std::string ltrim(const std::string &s);
    std::string rtrim(const std::string &s);
    std::string trim(const std::string &s);
};
