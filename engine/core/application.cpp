#include "core/application.hpp"

#include "core/engine.hpp"

Application::Application() : m_engine(std::make_unique<Engine>())
{
}

void Application::Run()
{
    m_engine->Run(*this);
}

bool Application::IsRunning() const
{
    return m_running;
}

Engine &Application::GetEngine()
{
    return *m_engine;
}
