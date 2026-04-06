#pragma once

#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "../ecs/entity.hpp"

namespace resources
{
    class ResourceManager;
}

struct AnimationDebugEntry
{
    Entity entity;
    std::string modelPath;
    std::string activeClipName;
    int activeClipIndex = -1;
    std::string targetClipName;
    int targetClipIndex = -1;
    double currentTimeSeconds = 0.0;
    double targetTimeSeconds = 0.0;
    double clipDurationSeconds = 0.0;
    double targetClipDurationSeconds = 0.0;
    double ticksPerSecond = 0.0;
    size_t boneCount = 0;
    size_t channelCount = 0;
    bool loop = false;
    bool paused = false;
    bool poseValid = false;
    bool transitionActive = false;
    float transitionAlpha = 0.0f;
    float transitionDurationSeconds = 0.0f;
    std::vector<std::string> availableClips;
};

struct AnimationDebugSnapshot
{
    std::vector<AnimationDebugEntry> entries;
};

class Scene
{
public:
    virtual ~Scene() = default;

    virtual void Init() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Draw(float deltaTime) = 0;
    virtual void OnResize(int width, int height) {}
    virtual resources::ResourceManager *GetResourceManager() { return nullptr; }
    virtual AnimationDebugSnapshot GetAnimationDebugSnapshot() const { return {}; }
    virtual bool RequestAnimationTransition(Entity entity, int clipIndex, float durationSeconds)
    {
        (void)entity;
        (void)clipIndex;
        (void)durationSeconds;
        return false;
    }

    void Render(float deltaTime)
    {
        Update(deltaTime);
        Draw(deltaTime);
    }
};