#pragma once

#include "entity.hpp"

class Component
{
public:
    virtual ~Component() = default;

    virtual void OnAttach(Entity entity) {}
    virtual void OnDetach(Entity entity) {}
};
