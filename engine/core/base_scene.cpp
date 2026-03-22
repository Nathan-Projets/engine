#include "base_scene.hpp"

BaseScene::BaseScene(World *world, resources::ResourceManager *resourceManager) : m_world(world), m_resourceManager(resourceManager)
{
}

void BaseScene::Init()
{
    m_world->AddSystem<PhysicsSystem>();
    m_world->AddSystem<RenderSystem>(m_resourceManager);
}

void BaseScene::Update(float deltaTime)
{
    m_world->UpdateSystems(deltaTime);
}

void BaseScene::Draw(float deltaTime)
{
    m_world->RenderSystems(deltaTime);
}
