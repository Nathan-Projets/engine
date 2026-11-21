#include "renderer.hpp"

#include <stdexcept>
#include <utility>

Renderer::Renderer(std::unique_ptr<IRendererBackend> backend) : m_backend(std::move(backend))
{
    if (!m_backend)
    {
        throw std::invalid_argument("Renderer backend must not be null");
    }
}

bool Renderer::Init()
{
    return m_backend->Init();
}

void Renderer::Clear()
{
    m_backend->Clear();
}
