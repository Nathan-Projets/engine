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

    config.title = "Window 1";
    config.width = 600;
    config.height = 400;
    m_window = &GetEngine().GetWindowManager().CreateWindow(config);

    config.title = "Window 2";
    m_window_2 = &GetEngine().GetWindowManager().CreateWindow(config);
}

void MainApplication::OnUpdate(float deltaTime)
{
    if (m_window && m_window->ShouldClose())
    {
        GetEngine().GetWindowManager().DestroyWindow(m_window->GetId());
        m_window = nullptr;
    }

    if (m_window_2 && m_window_2->ShouldClose())
    {
        GetEngine().GetWindowManager().DestroyWindow(m_window_2->GetId());
        m_window_2 = nullptr;
    }

    if (!m_window && !m_window_2)
    {
        m_running = false;
    }
}

void MainApplication::OnShutdown()
{
}