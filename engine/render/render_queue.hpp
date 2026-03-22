#pragma once

#include <cstdint>

enum class RenderQueue : uint8_t
{
    Lit,
    Transparent,
    Unlit,
    Debug
};
