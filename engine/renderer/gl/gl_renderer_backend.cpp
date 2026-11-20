#include "gl_renderer_backend.hpp"

GlRendererBackend::GlRendererBackend(IPlatformBackend &platform) : IRendererBackend(platform)
{
}

void GlRendererBackend::Init()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))  // I would like  the renderer to not know any of this but lazy to think of something
    {
        ERROR("GLAD not initialized.");
        return;
    }
}

void GlRendererBackend::Clear(int r, int g, int b, int a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}