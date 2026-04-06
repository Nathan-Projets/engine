#include "game_scene.hpp"

#include "../../engine/helpers/log.hpp"
#include "../../engine/input/input_manager.hpp"
#include "../../engine/ecs/components/camera.hpp"
#include "../../engine/ecs/systems/animation_system.hpp"
#include "../../engine/ecs/systems/physics_system.hpp"
#include "../../engine/ecs/systems/render_system.hpp"
#include "../../engine/resources/loaders/scene_loader.hpp"
#include "../systems/light_orbit_controller_system.hpp"
#include "../systems/orbit_camera_controller_system.hpp"

GameScene::GameScene(std::string scenePath, resources::ResourceManager *resourceManager) : m_resourceManager(resourceManager), m_scenePath(std::move(scenePath))
{
}

void GameScene::Init()
{
    ReloadScene();
}

void GameScene::Update(float deltaTime)
{
    if (InputManager::Get().IsActionJustPressed("reload_scene_clean"))
    {
        ReloadScene(true);
    }

    if (InputManager::Get().IsActionJustPressed("reload_scene"))
    {
        ReloadScene(false);
    }

    m_world.UpdateSystems(deltaTime);
}

void GameScene::Draw(float deltaTime)
{
    m_world.RenderSystems(deltaTime);
}

void GameScene::OnResize(int width, int height)
{
    m_viewportWidth = width;
    m_viewportHeight = height;

    if (width <= 0 || height <= 0)
    {
        return;
    }

    for (Entity entity : m_world.GetEntitiesWith<Camera>())
    {
        Camera *cam = m_world.GetComponent<Camera>(entity);
        if (cam && cam->main)
        {
            cam->GetProjection().SetViewportSize(static_cast<float>(width), static_cast<float>(height));
            break;
        }
    }
}

void GameScene::ReloadScene(bool clearResourceCache)
{
    if (clearResourceCache && m_resourceManager)
    {
        if (!m_resourceManager->ResetForCleanReload())
        {
            ERROR("Aborted clean reload: resource manager failed to reach idle state");
            return;
        }
        INFO("Reset resource manager before scene reload");
    }

    World reloadedWorld;
    if (!SceneLoader::LoadIntoWorld(m_scenePath, &reloadedWorld, m_resourceManager))
    {
        ERROR("Failed to hot reload scene: " << m_scenePath);
        return;
    }

    m_world = std::move(reloadedWorld);
    ConfigureSystems();

    if (m_viewportWidth > 0 && m_viewportHeight > 0)
    {
        OnResize(m_viewportWidth, m_viewportHeight);
    }

    INFO("Reloaded scene: " << m_scenePath);
}

void GameScene::ConfigureSystems()
{
    m_world.AddSystem<PhysicsSystem>();
    m_world.AddSystem<OrbitCameraControllerSystem>();
    m_world.AddSystem<LightOrbitControllerSystem>();
    m_world.AddSystem<AnimationSystem>(m_resourceManager);
    m_world.AddSystem<RenderSystem>(m_resourceManager);
}