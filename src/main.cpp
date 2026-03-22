#include <print>

#include <core/application.hpp>
#include <resources/resource_manager.hpp>
#include <resources/loaders/scene_loader.hpp>
#include <game/components/component_registration.hpp>
#include <game/scene/game_scene.hpp>
#include <helpers/log.hpp>

#include <simdjson.h>

int main(int argc, char const *argv[])
{
    const char *scenePath = "assets/scenes/backpack_scene.json";
    if (argc > 1)
    {
        scenePath = argv[1];
    }

    Application app;
    if (!app.Init())
    {
        return EXIT_FAILURE;
    }

    // TODO: implement a logic that search for the optimal number of available threads
    resources::ResourceManager resourceManager{3};

    GameSceneComponents::RegisterAll();

    std::shared_ptr<Scene> scene = std::make_shared<GameScene>(scenePath, &resourceManager);

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
