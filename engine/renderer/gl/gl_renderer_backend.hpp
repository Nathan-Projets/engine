#pragma once

#include "renderer/renderer_backend.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "helpers/log.hpp"

class GlRendererBackend : public IRendererBackend
{
public:
    explicit GlRendererBackend(IPlatformBackend &platform);
    ~GlRendererBackend() override = default;

    bool Init() override;
    void Clear() override;
};