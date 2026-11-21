#pragma once

#include "platform/platform_backend.hpp"

class IRendererBackend
{
public:
    IRendererBackend(IPlatformBackend &platform);
    virtual ~IRendererBackend() = default;

    virtual bool Init() = 0;
    virtual void Clear() = 0;

protected:
    IPlatformBackend &m_platform;
};