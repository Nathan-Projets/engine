#pragma once

#include <string>
#include <optional>

#include "render/mesh.hpp"

class ILoader
{
public:
    virtual ~ILoader() = default;

    virtual std::vector<Mesh> Load(const std::string &path) = 0;
};