#include <print>

#include <core/application.hpp>
#include <resources/manager.hpp>
#include <game/game_world.hpp>
#include <helpers/log.hpp>

int main(int argc, char const *argv[])
{
    Application app;
    ResourceManager resourceManager;
    GameWorld gameWorld{800, 800, &resourceManager};
    app.SetScene(&gameWorld);
    if (!app.Run())
    {
        ERROR("Application stopped unexpectedly!");
    }
}
