#pragma once

#include <memory>
#include <string>
#include <vector>

#include <simdjson.h>
#include <glm/glm.hpp>

#include "../../core/scene.hpp"
#include "../../core/base_scene.hpp"
#include "../../ecs/world.hpp"
#include "../../resources/manager.hpp"
#include "../engine/ecs/world.hpp"
#include "../engine/ecs/components/transform.hpp"
#include "../engine/ecs/components/mesh_renderer.hpp"
#include "../engine/ecs/components/light.hpp"
#include "../engine/resources/manager.hpp"
#include "../engine/resources/loaders/loader.hpp"
#include "../engine/render/camera/camera_perspective.hpp"
#include "../engine/render/mesh.hpp"
#include "../engine/helpers/log.hpp"

class SceneLoader
{
public:
    static std::shared_ptr<Scene> LoadScene(const std::string &filepath, World *world, ResourceManager *resourceManager);
};