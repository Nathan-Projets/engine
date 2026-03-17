#pragma once

#include "scene.hpp"
#include "../ecs/world.hpp"
#include "../resources/manager.hpp"
#include "../engine/ecs/systems/physics_system.hpp"
#include "../engine/ecs/systems/render_system.hpp"

class BaseScene : public Scene
{
public:
    BaseScene(World *world, ResourceManager *resourceManager);
    ~BaseScene() override = default;

    void Init() override;
    void Update(float deltaTime) override;
    void Draw(float deltaTime) override;

private:
    World *m_world;
    ResourceManager *m_resourceManager;
};
