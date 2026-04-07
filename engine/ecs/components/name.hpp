#pragma once

#include <string>
#include <utility>

#include "../component.hpp"

class Name : public Component
{
public:
    Name() = default;
    explicit Name(std::string text) : value(std::move(text)) {}

    std::string value;
};