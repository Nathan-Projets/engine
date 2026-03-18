#include "game_scene.hpp"

#include "../../engine/helpers/log.hpp"
#include "../../engine/input/input_manager.hpp"
#include "../../engine/ecs/systems/physics_system.hpp"
#include "../../engine/ecs/systems/render_system.hpp"
#include "../../engine/resources/loaders/scene_loader.hpp"
#include "../systems/orbit_camera_controller_system.hpp"
#include "../systems/rotating_light_system.hpp"

GameScene::GameScene(std::string scenePath, resources::ResourceManager *resourceManager)
    : m_resourceManager(resourceManager), m_scenePath(std::move(scenePath))
{
}

void GameScene::Init()
{
    ReloadScene();
}

void GameScene::Update(float deltaTime)
{
    if (InputManager::Get().IsActionJustPressed("reload_scene"))
    {
        ReloadScene();
    }

    m_world.UpdateSystems(deltaTime);
}

void GameScene::Draw(float deltaTime)
{
    m_world.RenderSystems(deltaTime);
}

void GameScene::ReloadScene()
{
    World reloadedWorld;
    if (!SceneLoader::LoadIntoWorld(m_scenePath, &reloadedWorld, m_resourceManager))
    {
        ERROR("Failed to hot reload scene: " << m_scenePath);
        return;
    }

    m_world = std::move(reloadedWorld);
    ConfigureSystems();
    INFO("Reloaded scene: " << m_scenePath);
}

void GameScene::ConfigureSystems()
{
    m_world.AddSystem<PhysicsSystem>();
    m_world.AddSystem<OrbitCameraControllerSystem>();
    m_world.AddSystem<RotatingLightSystem>();
    m_world.AddSystem<RenderSystem>(m_resourceManager);
}