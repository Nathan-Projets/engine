#pragma once

#include <string>

#include "../component.hpp"

class AnimationPlayer : public Component
{
public:
    std::string clipName;
    int clipIndex = -1;
    double currentTimeSeconds = 0.0;
    float speed = 1.0f;
    bool loop = true;
    bool paused = false;
};