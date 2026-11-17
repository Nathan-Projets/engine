#pragma once

#include <core/application.hpp>
#include <core/engine.hpp>
#include <platform/window.hpp>

class MainApplication final : public Application
{
public:
    MainApplication();
    ~MainApplication() override;

    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnShutdown() override;

private:
    Window *m_window = nullptr;
};