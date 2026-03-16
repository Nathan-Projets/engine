#include <print>

#include <core/application.hpp>
#include <resources/manager.hpp>
#include <resources/loaders/scene_loader.hpp>
#include <game/game_world.hpp>
#include <helpers/log.hpp>

#include <simdjson.h>

int main(int argc, char const *argv[])
{
    Application app;
    if (!app.Init())
    {
        return EXIT_FAILURE;
    }

    World world;
    ResourceManager resourceManager;

    std::shared_ptr<Scene> scene = SceneLoader::LoadScene("assets/scenes/level_1.json", &world, &resourceManager);

    if (!scene)
    {
        ERROR("Failed to load the scene.");
        return EXIT_FAILURE;
    }

    app.SetScene(scene.get());

    if (!app.Run())
    {
        ERROR("Application stopped unexpectedly!");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
