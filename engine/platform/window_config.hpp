#pragma once

#include <string>

struct WindowConfig
{
    std::string title = "Default title";
    unsigned int width = 1280;
    unsigned int height = 720;
    bool resizable = false;
};