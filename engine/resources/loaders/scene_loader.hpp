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
#include "ecs/components/name.hpp"
#include "ecs/components/transform.hpp"
#include "ecs/components/animation_player.hpp"
#include "ecs/components/mesh_renderer.hpp"
#include "ecs/components/light.hpp"
#include "resources/resource_manager.hpp"
#include "ecs/components/skeleton_pose.hpp"
#include "render/camera/perspective_projection.hpp"
#include "helpers/log.hpp"

// TODO: when implementing serialization, review if using these lambdas are the cleanest way, maybe incorporating this logic inside the component class?
using ComponentLoader = std::function<void(Entity entity,
                                           simdjson::ondemand::object componentJson,
                                           World &world,
                                           resources::ResourceManager *resourceManager)>;
using ComponentLoaderMap = std::unordered_map<std::string, ComponentLoader>;

/**
 * @class SceneLoader
 * @brief Loads scene definitions from JSON files and populates a World with entities
 * @details
 *
 * ## Scene Format
 * Scenes are JSON files containing an array of entity definitions:
 * ```json
 * {
 *   "name": "EntityName",
 *   "components": [
 *     { "type": "Transform", "position": [0, 0, 0] },
 *     { "type": "Render", "mesh": "models/mesh.obj", "material": "materials/lit.json" },
 *     { "type": "Light", "color": [1, 1, 1] }
 *   ]
 * }
 * ```
 *
 * ## Material Resolution
 * When a Render component specifies a "material" field, the material is loaded asynchronously:
 * - Material JSON defines shader path and properties
 * - Shader path uses auto-discovery: "shaders/proto/lit" finds lit.vert + lit.frag
 * - Texture paths and properties are extracted from material JSON
 * - All resources are loaded via the ResourceManager with deduplication
 *
 * ## Built-in Component Loaders
 * - Transform: position, rotation (Euler), scale
 * - Render: mesh, shader (optional), material (optional), queue
 * - Light: color, ambient, diffuse, specular
 * - Camera: fov, znear, zfar, aspect, main flag
 *
 * ## Custom Component Loaders
 * Use RegisterComponentLoader() to add support for custom components.
 */
class SceneLoader
{
public:
    /// @brief Register a custom component loader
    /// @param type Component type name (must match "type" field in JSON)
    /// @param loader Lambda that instantiates and initializes the component
    static void RegisterComponentLoader(std::string_view type, ComponentLoader loader);

    /// @brief Load a scene from JSON file into an existing world
    /// @param filepath Path to the scene JSON file
    /// @param world Target world to populate with entities
    /// @param resourceManager Resource manager for async asset loading (required for materials)
    /// @return true if load succeeded, false on parse/IO errors
    static bool LoadIntoWorld(const std::string &filepath, World *world, resources::ResourceManager *resourceManager = nullptr);

    /// @brief Load a scene from an in-memory JSON string into an existing world
    /// @param json Raw scene JSON
    /// @param world Target world to populate with entities
    /// @param resourceManager Resource manager for async asset loading
    /// @return true if load succeeded, false on parse errors
    static bool LoadIntoWorldFromJson(std::string_view json, World *world, resources::ResourceManager *resourceManager = nullptr);

    /// @brief Load a scene from JSON file and create a new Scene object
    /// @param filepath Path to the scene JSON file
    /// @param world Target world to populate
    /// @param resourceManager Resource manager for asset loading
    /// @return Newly constructed Scene containing the world
    static std::shared_ptr<Scene> LoadScene(const std::string &filepath, World *world, resources::ResourceManager *resourceManager = nullptr);

    /// @brief Get the registry of all component loaders
    static ComponentLoaderMap &GetComponentLoaders();

    /// @brief Helper: Read an optional string field from componentJson
    static bool readOptionalString(simdjson::ondemand::object componentJson, const char *fieldName, std::string &outValue);

    /// @brief Helper: Read a float field from component JSON with default fallback
    static float ReadFloat(simdjson::ondemand::object componentJson, const char *fieldName, float defaultValue);

    /// @brief Helper: Read a bool field from component JSON with default fallback
    static bool ReadBool(simdjson::ondemand::object componentJson, const char *fieldName, bool defaultValue);

    /// @brief Helper: Read a vec3 field from component JSON with default fallback
    static glm::vec3 ReadVec3(simdjson::ondemand::object componentJson, const char *fieldName, const glm::vec3 &defaultValue);

    /// @brief Helper: Read render queue field with default fallback
    static RenderQueue ReadRenderQueue(simdjson::ondemand::object componentJson, RenderQueue defaultValue);

    /// @brief Helper: Read array of floats from JSON array
    static std::vector<float> ReadFloatArray(simdjson::ondemand::array values, size_t maxValues, float defaultValue);

    /// @brief Helper: Parse camera frustum parameters from JSON
    static PerspectiveProjection::Frustrum ReadFrustrum(simdjson::ondemand::object componentJson);

    /// @brief Helper: Read light type from JSON with default fallback
    static LightType ReadLightType(simdjson::ondemand::object componentJson, LightType defaultValue);

    /// @brief Helper: Read attenuation attributes constant, linear, and quadratic, returned in this specific order
    static std::array<float, 3> ReadLightAttenuation(simdjson::ondemand::object componentJson);

    /// @brief Helper: Read spotlight attributes innerCutoff and outerCutoff, returned in this specific order
    static std::array<float, 2> ReadSpotValues(simdjson::ondemand::object componentJson);

    /// @brief Helper: Read castShadows value from JSON with default fallback
    static bool ReadCastShadow(simdjson::ondemand::object componentJson, bool defaultValue);

    /// @brief Register all built-in component loaders (Transform, Render, Light, Camera)
    static void RegisterBuiltInComponentLoaders();
};