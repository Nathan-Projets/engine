#pragma once

#include <memory>

class Engine;

class Application
{
public:
    Application();
    virtual ~Application() = default;

    void Run();
    bool IsRunning() const;
    
    virtual void OnInit() = 0;
    virtual void OnUpdate(float deltaTime) = 0;
    virtual void OnShutdown() = 0;

protected:
    Engine &GetEngine();

    bool m_running = true;

private:
    std::unique_ptr<Engine> m_engine;
};