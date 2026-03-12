#include "game_world.hpp"

#include "../ecs/systems/transform_system.hpp"
#include "../ecs/systems/render_system.hpp"
#include "../game/systems/rotating_light_system.hpp"
#include "../game/components/light.hpp"
#include "../game/components/camera.hpp"
#include "../input/input_manager.hpp"
#include "../resources/loaders/loader.hpp"

GameWorld::GameWorld(int width, int height, ResourceManager *resourceManager)
    : m_camera({45.0f, (float)width, (float)height, 0.1f, 150.0f},
               glm::vec3(0.0f, 0.0f, 9.0f),
               glm::vec3(0, 0, 0),
               glm::vec3(0, 1, 0)),
      m_resourceManager(resourceManager),
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
    m_world.AddSystem<TransformSystem>();
    m_world.AddSystem<RenderSystem>();

    // Add game-specific systems, linked to the current scene
    m_world.AddSystem<RotatingLightSystem>();

    // Load resources
    m_resourceManager->Load<Shader>("model", "assets/shaders/model.vert", "assets/shaders/model.frag");
    m_resourceManager->Load<Shader>("light", "assets/shaders/light.vert", "assets/shaders/light.frag");

    // Set up main camera
    Entity cameraEntity = m_world.CreateEntity();
    m_world.AddComponent<CameraComponent>(cameraEntity, m_camera, true);

    // TODO: needed for this scene, may need to check where it's supposed to be filled again
    m_resourceManager->Get<Shader>("model")->Use();
    m_resourceManager->Get<Shader>("model")->Upload("material.shininess", 32.0f);

    m_resourceManager->Get<Shader>("light")->Use();
    m_resourceManager->Get<Shader>("light")->Upload("projection", m_camera.GetProjectionMatrix());
    m_resourceManager->Get<Shader>("light")->Upload("color", glm::vec3(1.0f));

    Entity backpack = m_world.CreateEntity();
    Transform *backpackTransform = m_world.AddComponent<Transform>(backpack, glm::vec3(0.0f, 0.0f, 0.0f));

    m_backpackMeshes = Loader::Load("assets/meshes/backpack/backpack.obj");
    if (!m_backpackMeshes.empty())
    {
        MeshRenderer *meshRenderer = m_world.AddComponent<MeshRenderer>(backpack);
        for (auto &mesh : m_backpackMeshes)
        {
            meshRenderer->AddMesh(&mesh);
        }
        meshRenderer->SetShader(m_resourceManager->Get<Shader>("model").get());
    }

    Entity lightEntity = m_world.CreateEntity();
    auto *lightTransform = m_world.AddComponent<Transform>(lightEntity, glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.1f));

    auto *light = m_world.AddComponent<Light>(lightEntity);
    light->ambient = glm::vec3(0.2f, 0.2f, 0.2f);
    light->diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
    light->specular = glm::vec3(0.7f, 0.7f, 0.7f);
    light->position = lightTransform->position;

    m_cubeMeshes = Loader::Load("assets/meshes/cube/cube.obj");
    if (!m_cubeMeshes.empty())
    {
        // TODO: look into loading meshes into a registry instead of having a copy on the stack which I need to add one by one like here
        MeshRenderer *meshRenderer = m_world.AddComponent<MeshRenderer>(lightEntity);
        for (auto &mesh : m_cubeMeshes)
        {
            meshRenderer->AddMesh(&mesh);
        }
        meshRenderer->SetShader(m_resourceManager->Get<Shader>("light").get());
    }
}

void GameWorld::Update(float deltaTime)
{
    const float cameraSpeed = 12.5f;

    // Not sure where to put these controls with the current ecs set up
    // I will probably need to rework the camera thing anyway

    // Camera controls
    if (InputManager::Get().IsActionActive("move_forward"))
        m_camera.MoveForward(cameraSpeed * deltaTime);
    if (InputManager::Get().IsActionActive("move_backward"))
        m_camera.MoveBackward(cameraSpeed * deltaTime);
    if (InputManager::Get().IsActionActive("move_left"))
        m_camera.MoveLeft(cameraSpeed * deltaTime);
    if (InputManager::Get().IsActionActive("move_right"))
        m_camera.MoveRight(cameraSpeed * deltaTime);

    // Scroll zoom
    if (InputManager::Get().GetMouseScroll().y > 0)
        m_camera.MoveForward(cameraSpeed * deltaTime * 2.0f);
    if (InputManager::Get().GetMouseScroll().y < 0)
        m_camera.MoveBackward(cameraSpeed * deltaTime * 2.0f);

    m_world.UpdateSystems(deltaTime);
}

void GameWorld::Render(float deltaTime)
{
    m_world.RenderSystems(deltaTime);
}
