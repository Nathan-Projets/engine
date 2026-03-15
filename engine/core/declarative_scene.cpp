#include "declarative_scene.hpp"

DeclarativeScene::DeclarativeScene(World *world, ResourceManager *resourceManager) : m_world(world), m_resourceManager(resourceManager)
{
}

void DeclarativeScene::Init()
{
    m_world->AddSystem<PhysicSystem>();
    m_world->AddSystem<RenderSystem>();
}

void DeclarativeScene::Update(float deltaTime)
{
    m_world->UpdateSystems(deltaTime);
}

void DeclarativeScene::Draw(float deltaTime)
{
    m_world->RenderSystems(deltaTime);
}