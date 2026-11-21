#pragma once

#include <memory>

#include "renderer/renderer_backend.hpp"

class Renderer
{
public:
    Renderer(std::unique_ptr<IRendererBackend> backend);

    bool Init();
    void Clear();

private:
    std::unique_ptr<IRendererBackend> m_backend;
};