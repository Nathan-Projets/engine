#include "application.hpp"
#include <iostream>

extern "C"
{
    void update(float dt)
    {
        std::cout << "Updating game, dt = " << dt << " seconds" << std::endl;
    }

    void render()
    {
        std::cout << "Rendering game frame" << std::endl;
    }

    Application *createApplication()
    {
        static Application app;
        app.update = update;
        app.render = render;
        return &app;
    }

    void destroyApplication(Application *app)
    {
        // nothing to do here for static app
    }
}