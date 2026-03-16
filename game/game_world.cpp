#include "game_world.hpp"

#include <string>
#include <stdexcept>

#include "../engine/ecs/systems/physic_system.hpp"
#include "../engine/ecs/systems/render_system.hpp"
#include "systems/rotating_light_system.hpp"
#include "../engine/ecs/components/light.hpp"
#include "../engine/ecs/components/camera.hpp"
#include "../engine/input/input_manager.hpp"
#include "../engine/resources/loaders/loader.hpp"

using Meshes = std::vector<Mesh>;

GameWorld::GameWorld(int width, int height, ResourceManager *resourceManager)
    : m_resourceManager(resourceManager),
      m_width(width),
      m_height(height)
{
}

GameWorld::~GameWorld()
{
}

void GameWorld::Init()
{
    // Add engine systems
    m_world.AddSystem<PhysicSystem>();
    m_world.AddSystem<RenderSystem>();

    // Add game-specific systems, linked to the current scene
    m_world.AddSystem<RotatingLightSystem>();

    // Load resources
    m_resourceManager->Load<Shader>("model", "assets/shaders/model.vert", "assets/shaders/model.frag");
    m_resourceManager->Load<Shader>("light", "assets/shaders/light.vert", "assets/shaders/light.frag");

    // Set up main camera
    Entity cameraEntity = m_world.CreateEntity();
    m_world.AddComponent<CameraComponent>(cameraEntity,
                                          PerspectiveCamera::Frustrum{45.0f, (float)m_width, (float)m_height, 0.1f, 150.0f},
                                          glm::vec3(0.0f, 0.0f, 9.0f),
                                          glm::vec3(0.0f, 0.0f, 0.0f),
                                          glm::vec3(0.0f, 1.0f, 0.0f),
                                          true);

    // TODO: needed for this scene, may need to check where it's supposed to be filled again
    m_resourceManager->Get<Shader>("model")->Use();
    m_resourceManager->Get<Shader>("model")->Upload("material.shininess", 32.0f);

    m_resourceManager->Get<Shader>("light")->Use();
    m_resourceManager->Get<Shader>("light")->Upload("projection", GetCamera().GetProjectionMatrix());
    m_resourceManager->Get<Shader>("light")->Upload("color", glm::vec3(1.0f));

    Entity backpack = m_world.CreateEntity();
    Transform *backpackTransform = m_world.AddComponent<Transform>(backpack, glm::vec3(0.0f, 0.0f, 0.0f));

    const std::string backpackMeshId = "assets/meshes/backpack/backpack.obj";
    auto backpackMeshes = m_resourceManager->Load<Meshes>(backpackMeshId, Loader::Load(backpackMeshId));
    if (!backpackMeshes->empty())
    {
        MeshRenderer *meshRenderer = m_world.AddComponent<MeshRenderer>(backpack);
        meshRenderer->SetMeshes(backpackMeshes);
        meshRenderer->SetShader(m_resourceManager->Get<Shader>("model").get());
    }

    Entity lightEntity = m_world.CreateEntity();
    auto *lightTransform = m_world.AddComponent<Transform>(lightEntity, glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f));

    auto *light = m_world.AddComponent<Light>(lightEntity);
    light->ambient = glm::vec3(0.2f, 0.2f, 0.2f);
    light->diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
    light->specular = glm::vec3(0.7f, 0.7f, 0.7f);
    light->position = lightTransform->position;

    const std::string cubeMeshId = "assets/meshes/cube/cube.obj";
    auto cubeMeshes = m_resourceManager->Load<Meshes>(cubeMeshId, Loader::Load(cubeMeshId));
    if (!cubeMeshes->empty())
    {
        MeshRenderer *meshRenderer = m_world.AddComponent<MeshRenderer>(lightEntity);
        meshRenderer->SetMeshes(cubeMeshes);
        meshRenderer->SetShader(m_resourceManager->Get<Shader>("light").get());
    }
}

void GameWorld::Update(float deltaTime)
{
    const float cameraSpeed = 12.5f;
    PerspectiveCamera &camera = GetCamera();

    // Not sure where to put these controls with the current ecs set up
    // I will probably need to rework the camera thing anyway

    // Camera controls
    if (InputManager::Get().IsActionActive("move_forward"))
        camera.MoveForward(cameraSpeed * deltaTime);
    if (InputManager::Get().IsActionActive("move_backward"))
        camera.MoveBackward(cameraSpeed * deltaTime);
    if (InputManager::Get().IsActionActive("move_left"))
        camera.MoveLeft(cameraSpeed * deltaTime);
    if (InputManager::Get().IsActionActive("move_right"))
        camera.MoveRight(cameraSpeed * deltaTime);

    // Scroll zoom
    if (InputManager::Get().GetMouseScroll().y > 0)
        camera.MoveForward(cameraSpeed * deltaTime * 2.0f);
    if (InputManager::Get().GetMouseScroll().y < 0)
        camera.MoveBackward(cameraSpeed * deltaTime * 2.0f);

    m_world.UpdateSystems(deltaTime);
}

void GameWorld::Draw(float deltaTime)
{
    m_world.RenderSystems(deltaTime);
}

PerspectiveCamera &GameWorld::GetCamera()
{
    auto cameraEntities = m_world.GetEntitiesWith<CameraComponent>();
    for (Entity cameraEntity : cameraEntities)
    {
        CameraComponent *cameraComponent = m_world.GetComponent<CameraComponent>(cameraEntity);
        if (cameraComponent && cameraComponent->main)
            return cameraComponent->GetCamera();
    }

    throw std::runtime_error("Main camera was requested but no CameraComponent with main=true exists.");
}
