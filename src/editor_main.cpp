#include <core/application.hpp>
#include <resources/resource_manager.hpp>
#include <game/components/component_registration.hpp>
#include <game/scene/editor_scene.hpp>
#include <helpers/log.hpp>

int main(int argc, char const *argv[])
{
    const char *scenePath = "assets/scenes/backpack_scene.json";
    if (argc > 1)
    {
        scenePath = argv[1];
    }

    Application app(1600, 900, {.enableEditorUI = true, .enableDebugUI = true, .windowTitle = "Engine Editor"});
    if (!app.Init())
    {
        return EXIT_FAILURE;
    }

    resources::ResourceManager resourceManager{3};
    GameComponentLoaders::RegisterAll();

    std::shared_ptr<Scene> scene = std::make_shared<EditorScene>(scenePath, &resourceManager, false);
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