#include "simple_game_scene.hpp"

#include <algorithm>

#include <audio/audio_engine.hpp>
#include <ecs/components/camera.hpp>
#include <ecs/components/light.hpp>
#include <ecs/components/mesh_renderer.hpp>
#include <ecs/components/name.hpp>
#include <ecs/components/transform.hpp>
#include <physics/collider.hpp>
#include <physics/rigidbody.hpp>
#include <ecs/systems/render_system.hpp>
#include <helpers/log.hpp>
#include <resources/resource_manager.hpp>
#include <resources/units/material.hpp>
#include <resources/units/model.hpp>

#include "../components/physics_listener.hpp"
#include "../components/player_controller.hpp"
#include "../scene/runtime_scene_support.hpp"
#include "../systems/physics_listener_demo_system.hpp"
#include "../systems/player_controller_system.hpp"

SimpleGameScene::SimpleGameScene(resources::ResourceManager *resourceManager) : m_resourceManager(resourceManager)
{
}

SimpleGameScene::~SimpleGameScene()
{
    AudioEngine::Get().StopMusic();
}

void SimpleGameScene::Init()
{
    BuildScene();
}

void SimpleGameScene::Update(float deltaTime)
{
    if (!m_gamePaused)
    {
        m_world.UpdateSystems(deltaTime);
    }
}

void SimpleGameScene::Draw(float deltaTime)
{
    m_world.RenderSystems(deltaTime);
}

void SimpleGameScene::OnResize(int width, int height)
{
    ApplyViewportSize(width, height);
}

RuntimeUIState SimpleGameScene::GetRuntimeUIState() const
{
    RuntimeUIState state;
    state.valid = true;
    state.paused = m_gamePaused;
    state.title = "Trigger Door Demo";
    state.objective = "Push the player cube into the trigger zone to open the door.";
    state.hint = "WASD move | ESC pause | Enter the glowing zone behind the player.";

    bool doorOpen = false;
    for (Entity entity : m_world.GetEntitiesWith<Transform, Name>())
    {
        const Name *name = m_world.GetComponent<Name>(entity);
        const Transform *transform = m_world.GetComponent<Transform>(entity);
        if (!name || !transform || name->value != "GoalDoor")
        {
            continue;
        }

        doorOpen = transform->position.y > 2.0f;
        break;
    }

    state.objectiveCompleted = doorOpen;
    state.status = doorOpen ? "Door open: objective complete" : "Door closed: move into the trigger zone";
    return state;
}

bool SimpleGameScene::SetGamePaused(bool paused)
{
    m_gamePaused = paused;
    if (paused)
    {
        AudioEngine::Get().PauseMusic();
    }
    else
    {
        AudioEngine::Get().ResumeMusic();
    }
    return true;
}

bool SimpleGameScene::ResetRuntimeState()
{
    BuildScene();
    ApplyViewportSize(m_viewportWidth, m_viewportHeight);
    return true;
}

void SimpleGameScene::PresentToScreen()
{
    runtime_scene_support::PresentRenderSystem(m_renderSystem);
}

void SimpleGameScene::BuildScene()
{
    m_world = World{};
    m_gamePaused = false;
    AudioEngine::Get().StopMusic();
    AudioEngine::Get().PlayMusic("assets/audio/runtime_loop.wav", true, 0.55f);

    Entity groundEntity = m_world.CreateEntity();
    m_world.AddComponent<Name>(groundEntity, "Ground");
    m_world.AddComponent<Transform>(groundEntity, glm::vec3(0.0f, -1.05f, 0.0f), glm::vec3(0.0f), glm::vec3(12.0f, 0.25f, 12.0f));
    BoxCollider *groundCollider = m_world.AddComponent<BoxCollider>(groundEntity);
    groundCollider->halfExtents = glm::vec3(1.0f);
    groundCollider->material.friction = 1.0f;
    groundCollider->material.restitution = 0.0f;
    MeshRenderer *groundRenderer = m_world.AddComponent<MeshRenderer>(groundEntity);
    groundRenderer->SetLoadTextures(true);

    m_cubeEntity = m_world.CreateEntity();
    m_world.AddComponent<Name>(m_cubeEntity, "Player");
    m_world.AddComponent<Transform>(m_cubeEntity, glm::vec3(0.0f, 1.8f, 0.0f), glm::vec3(60.0f, 75.0f, 0.0f), glm::vec3(1.0f));
    Rigidbody *playerBody = m_world.AddComponent<Rigidbody>(m_cubeEntity);
    playerBody->type = RigidbodyType::Dynamic;
    playerBody->mass = 1.0f;
    playerBody->linearDamping = 0.12f;
    playerBody->lockRotation = false;
    playerBody->useGravity = true;
    playerBody->gravityScale = 1.0f;
    playerBody->linearVelocity = glm::vec3(0.0f);
    BoxCollider *playerCollider = m_world.AddComponent<BoxCollider>(m_cubeEntity);
    playerCollider->halfExtents = glm::vec3(1.0f);
    playerCollider->material.friction = 0.8f;
    playerCollider->material.restitution = 0.0f;
    m_world.AddComponent<PlayerController>(m_cubeEntity);
    m_world.AddComponent<PhysicsListener>(m_cubeEntity);
    MeshRenderer *cubeRenderer = m_world.AddComponent<MeshRenderer>(m_cubeEntity);
    cubeRenderer->SetLoadTextures(true);

    Entity triggerEntity = m_world.CreateEntity();
    m_world.AddComponent<Name>(triggerEntity, "GoalTrigger");
    m_world.AddComponent<Transform>(triggerEntity, glm::vec3(0.0f, 0.2f, -4.5f), glm::vec3(0.0f), glm::vec3(1.4f, 1.4f, 1.4f));
    Rigidbody *triggerBody = m_world.AddComponent<Rigidbody>(triggerEntity);
    triggerBody->type = RigidbodyType::Static;
    triggerBody->isTrigger = true;
    BoxCollider *triggerCollider = m_world.AddComponent<BoxCollider>(triggerEntity);
    triggerCollider->halfExtents = glm::vec3(1.0f);
    triggerCollider->isTrigger = true;
    triggerCollider->material.friction = 0.0f;
    triggerCollider->material.restitution = 0.0f;
    MeshRenderer *triggerRenderer = m_world.AddComponent<MeshRenderer>(triggerEntity);
    triggerRenderer->SetLoadTextures(true);

    Entity doorEntity = m_world.CreateEntity();
    m_world.AddComponent<Name>(doorEntity, "GoalDoor");
    m_world.AddComponent<Transform>(doorEntity, glm::vec3(0.0f, 0.65f, -7.0f), glm::vec3(0.0f), glm::vec3(2.2f, 2.0f, 0.45f));
    Rigidbody *doorBody = m_world.AddComponent<Rigidbody>(doorEntity);
    doorBody->type = RigidbodyType::Static;
    BoxCollider *doorCollider = m_world.AddComponent<BoxCollider>(doorEntity);
    doorCollider->halfExtents = glm::vec3(1.0f);
    doorCollider->material.friction = 0.9f;
    doorCollider->material.restitution = 0.0f;
    MeshRenderer *doorRenderer = m_world.AddComponent<MeshRenderer>(doorEntity);
    doorRenderer->SetLoadTextures(true);

    if (m_resourceManager)
    {
        groundRenderer->SetModelHandle(m_resourceManager->Load<resources::Model>("assets/meshes/cube/cube.obj"));
        groundRenderer->SetMaterialHandle(m_resourceManager->Load<resources::Material>("assets/materials/lit_pavement.json"));

        cubeRenderer->SetModelHandle(m_resourceManager->Load<resources::Model>("assets/meshes/cube/cube.obj"));
        cubeRenderer->SetMaterialHandle(m_resourceManager->Load<resources::Material>("assets/materials/lit_cube.json"));

        triggerRenderer->SetModelHandle(m_resourceManager->Load<resources::Model>("assets/meshes/cube/cube.obj"));
        triggerRenderer->SetMaterialHandle(m_resourceManager->Load<resources::Material>("assets/materials/lit_cube.json"));

        doorRenderer->SetModelHandle(m_resourceManager->Load<resources::Model>("assets/meshes/cube/cube.obj"));
        doorRenderer->SetMaterialHandle(m_resourceManager->Load<resources::Material>("assets/materials/lit_pavement.json"));
    }

    Entity lightEntity = m_world.CreateEntity();
    m_world.AddComponent<Name>(lightEntity, "SunLight");
    Light *light = m_world.AddComponent<Light>(lightEntity);
    light->type = LightType::Directional;
    light->direction = glm::normalize(glm::vec3(-0.7f, -1.0f, -0.4f));
    light->color = glm::vec3(1.0f, 0.97f, 0.92f);
    light->ambient = glm::vec3(0.08f, 0.08f, 0.09f);
    light->diffuse = glm::vec3(0.75f, 0.72f, 0.68f);
    light->specular = glm::vec3(0.45f, 0.45f, 0.45f);
    light->intensity = 1.0f;
    light->castShadows = true;

    Entity fillLightEntity = m_world.CreateEntity();
    m_world.AddComponent<Name>(fillLightEntity, "FillLight");
    m_world.AddComponent<Transform>(fillLightEntity, glm::vec3(2.5f, 1.8f, 2.5f));
    Light *fillLight = m_world.AddComponent<Light>(fillLightEntity);
    fillLight->type = LightType::Point;
    fillLight->color = glm::vec3(0.45f, 0.55f, 1.0f);
    fillLight->ambient = glm::vec3(0.01f, 0.01f, 0.02f);
    fillLight->diffuse = glm::vec3(0.35f, 0.4f, 0.75f);
    fillLight->specular = glm::vec3(0.4f, 0.45f, 0.8f);
    fillLight->intensity = 2.5f;
    fillLight->castShadows = false;

    Entity cameraEntity = m_world.CreateEntity();
    m_world.AddComponent<Name>(cameraEntity, "MainCamera");
    m_world.AddComponent<Transform>(cameraEntity, glm::vec3(0.0f, 3.0f, 6.5f));
    m_world.AddComponent<Camera>(cameraEntity,
                                 PerspectiveProjection::Frustrum{45.0f, static_cast<float>(m_viewportWidth), static_cast<float>(m_viewportHeight), 0.1f, 150.0f},
                                 glm::vec3(0.0f, 1.0f, 0.0f),
                                 glm::vec3(0.0f, 1.0f, 0.0f),
                                 true);

    m_world.AddSystem<PlayerControllerSystem>();
    m_world.AddSystem<PhysicsListenerDemoSystem>();
    runtime_scene_support::ConfigureBasicRuntimeSystems(m_world, m_resourceManager, m_renderSystem);
    runtime_scene_support::ApplyViewportToMainCameraAndRenderer(m_world, m_renderSystem, m_viewportWidth, m_viewportHeight);
}

void SimpleGameScene::ApplyViewportSize(int width, int height)
{
    m_viewportWidth = std::max(width, 1);
    m_viewportHeight = std::max(height, 1);
    runtime_scene_support::ApplyViewportToMainCameraAndRenderer(m_world, m_renderSystem, m_viewportWidth, m_viewportHeight);
}