#include <core/application.hpp>
#include <resources/resource_manager.hpp>
#include <game/components/component_registration.hpp>
#include <game/scene/editor_scene.hpp>
#include <game/scene/simple_game_scene.hpp>
#include <helpers/log.hpp>

int main(int argc, char const *argv[])
{
    Application app(1280, 720, {.enableEditorUI = false, .enableDebugUI = false, .windowTitle = "Game"});
    if (!app.Init())
    {
        return EXIT_FAILURE;
    }

    resources::ResourceManager resourceManager{3};
    GameComponentLoaders::RegisterAll();

    std::shared_ptr<Scene> scene;
    if (argc > 1)
    {
        scene = std::make_shared<EditorScene>(argv[1], &resourceManager, true);
    }
    else
    {
        scene = std::make_shared<SimpleGameScene>(&resourceManager);
    }

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