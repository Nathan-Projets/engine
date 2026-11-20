#pragma once

#include "input/input_backend.hpp"

class GlfwInputBackend : public IInputBackend
{
public:
    bool Init() override;
    void Shutdown() override;
};