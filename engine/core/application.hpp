#pragma once

extern "C"
{
    struct Application
    {
        void (*update)(float dt);
        void (*render)();
    };

    // Factory function to create the application instance
    Application *createApplication();
    void destroyApplication(Application *app);
}