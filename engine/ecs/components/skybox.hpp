#pragma once

#include "../component.hpp"

#include "../../resources/handle.hpp"
#include "../../resources/units/model.hpp"
#include "../../resources/units/material.hpp"

class Skybox : public Component
{
public:
    resources::Handle<resources::Model> modelHandle;
    resources::Handle<resources::Material> materialHandle;

    float intensity = 1.0f;
    float scale = 50.0f;
};
