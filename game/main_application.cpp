#include "main_application.hpp"

MainApplication::MainApplication()
{
}

MainApplication::~MainApplication()
{
}

void MainApplication::OnInit()
{
    WindowConfig config;

    config.title = "Test";
    config.width = 600;
    config.height = 400;

    m_window = &GetEngine().GetWindowManager().CreateWindow(config);
}

void MainApplication::OnUpdate(float deltaTime)
{
    if (m_window->ShouldClose())
    {
        m_running = false;
    }
}

void MainApplication::OnShutdown()
{
}