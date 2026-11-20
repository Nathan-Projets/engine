#pragma once

class IInputBackend
{
public:
    virtual ~IInputBackend() = default;

    virtual bool Init() = 0;
    virtual void Shutdown() = 0;
};