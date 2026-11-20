#pragma once

#include "platform/platform_backend.hpp"

class IRendererBackend
{
public:
    IRendererBackend(IPlatformBackend &platform);
    virtual ~IRendererBackend() = default;

    virtual void Init() = 0;
    virtual void Clear(int r, int g, int b, int a = 1) = 0;

protected:
    IPlatformBackend &m_platform;
};