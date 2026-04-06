#pragma once

#include <string>

#include "../component.hpp"

class AnimationPlayer : public Component
{
public:
    std::string clipName;
    int clipIndex = -1;
    double currentTimeSeconds = 0.0;
    std::string targetClipName;
    int targetClipIndex = -1;
    double targetTimeSeconds = 0.0;
    float transitionDurationSeconds = 0.0f;
    float transitionElapsedSeconds = 0.0f;
    bool transitionActive = false;
    float speed = 1.0f;
    bool loop = true;
    bool paused = false;
};