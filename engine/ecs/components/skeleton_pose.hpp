#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "../component.hpp"

class SkeletonPose : public Component
{
public:
    std::vector<glm::mat4> nodeGlobalTransforms;
    std::vector<std::vector<glm::mat4>> primitiveBonePalettes;
    bool valid = false;
};