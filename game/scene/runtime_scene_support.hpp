#pragma once

#include <algorithm>
#include <optional>

#include <ecs/components/camera.hpp>
#include <ecs/systems/physics_system.hpp>
#include <ecs/systems/render_system.hpp>
#include <resources/resource_manager.hpp>

#include <systems/physics_event_listener_system.hpp>

namespace runtime_scene_support
{
    inline void ConfigureBasicRuntimeSystems(World &world,
                                             resources::ResourceManager *resourceManager,
                                             RenderSystem *&renderSystem,
                                             bool includePhysics = true)
    {
        if (includePhysics)
        {
            world.AddSystem<PhysicsSystem>();
            world.AddSystem<PhysicsEventListenerSystem>();
        }

        renderSystem = world.AddSystem<RenderSystem>(resourceManager);
        if (renderSystem)
        {
            renderSystem->SetCameraOverride(std::nullopt);
        }
    }

    inline void ApplyViewportToMainCameraAndRenderer(World &world,
                                                     RenderSystem *renderSystem,
                                                     int width,
                                                     int height)
    {
        const int safeWidth = std::max(width, 1);
        const int safeHeight = std::max(height, 1);

        for (Entity entity : world.GetEntitiesWith<Camera>())
        {
            Camera *camera = world.GetComponent<Camera>(entity);
            if (camera && camera->main)
            {
                camera->GetProjection().SetViewportSize(static_cast<float>(safeWidth), static_cast<float>(safeHeight));
                break;
            }
        }

        if (renderSystem)
        {
            renderSystem->SetViewportSize(static_cast<uint32_t>(safeWidth), static_cast<uint32_t>(safeHeight));
        }
    }

    inline void PresentRenderSystem(RenderSystem *renderSystem)
    {
        if (renderSystem)
        {
            renderSystem->PresentToScreen();
        }
    }
}