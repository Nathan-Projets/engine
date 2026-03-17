#pragma once

#include <cstdint>

enum class RenderQueue : uint8_t
{
    Opaque,
    Transparent,
    Unlit,
    Debug
};
