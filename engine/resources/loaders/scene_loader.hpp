#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

#include <simdjson.h>
#include <glm/glm.hpp>

#include "core/scene.hpp"
#include "core/base_scene.hpp"
#include "ecs/world.hpp"
#include "ecs/components/transform.hpp"
#include "ecs/components/mesh_renderer.hpp"
#include "ecs/components/light.hpp"
#include "resources/manager.hpp"
#include "resources/loaders/loader.hpp"
#include "render/camera/perspective_projection.hpp"
#include "render/mesh.hpp"
#include "helpers/log.hpp"

// TODO: when implementing serialization, review if using these lambdas are the cleanest way, maybe incorporating this logic inside the component class?
using ComponentLoader = std::function<void(Entity entity,
                                           simdjson::ondemand::object componentJson,
                                           World &world,
                                           ResourceManager &resourceManager)>;
using ComponentLoaderMap = std::unordered_map<std::string, ComponentLoader>;

class SceneLoader
{
public:
    static void RegisterComponentLoader(std::string_view type, ComponentLoader loader);
    static bool LoadIntoWorld(const std::string &filepath, World *world, ResourceManager *resourceManager);
    static std::shared_ptr<Scene> LoadScene(const std::string &filepath, World *world, ResourceManager *resourceManager);

    static ComponentLoaderMap &GetComponentLoaders();
    static float ReadFloat(simdjson::ondemand::object componentJson, const char *fieldName, float defaultValue);
    static glm::vec3 ReadVec3(simdjson::ondemand::object componentJson, const char *fieldName, const glm::vec3 &defaultValue);
    static RenderQueue ReadRenderQueue(simdjson::ondemand::object componentJson, RenderQueue defaultValue);
    static std::vector<float> ReadFloatArray(simdjson::ondemand::array values, size_t maxValues, float defaultValue);
    static PerspectiveProjection::Frustrum ReadFrustrum(simdjson::ondemand::object componentJson);
    static void RegisterBuiltInComponentLoaders();
};