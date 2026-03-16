#include "scene_loader.hpp"

ComponentLoaderMap &SceneLoader::GetComponentLoaders()
{
    static ComponentLoaderMap loaders;
    return loaders;
}

float SceneLoader::ReadFloat(simdjson::ondemand::object componentJson, const char *fieldName, float defaultValue)
{
    simdjson::ondemand::value value = componentJson[fieldName];
    if (value.is_null())
        return defaultValue;

    return static_cast<float>(double(value));
}

glm::vec3 SceneLoader::ReadVec3(simdjson::ondemand::object componentJson, const char *fieldName, const glm::vec3 &defaultValue)
{
    glm::vec3 result = defaultValue;
    simdjson::ondemand::array values = componentJson[fieldName].get_array();
    int index = 0;
    for (double value : values)
    {
        if (index >= 3)
            break;

        result[index++] = static_cast<float>(value);
    }

    return result;
}

std::vector<float> SceneLoader::ReadFloatArray(simdjson::ondemand::array values, size_t maxValues, float defaultValue)
{
    std::vector<float> result(maxValues, defaultValue);
    size_t index = 0;
    for (double value : values)
    {
        if (index >= maxValues)
            break;

        result[index++] = static_cast<float>(value);
    }

    return result;
}

PerspectiveCamera::Frustrum SceneLoader::ReadFrustrum(simdjson::ondemand::object componentJson)
{
    const std::vector<float> values = ReadFloatArray(componentJson["frustrum"].get_array(), 5, 0.0f);
    return {
        values[0],
        values[1],
        values[2],
        values[3],
        values[4]};
}

void SceneLoader::RegisterBuiltInComponentLoaders()
{
    static bool registered = false;
    if (registered)
        return;

    registered = true;
    ComponentLoaderMap &loaders = GetComponentLoaders();

    loaders.emplace("Transform",
                    [](Entity entity, simdjson::ondemand::object componentJson, World &world, ResourceManager &resourceManager)
                    {
                        world.AddComponent<Transform>(entity,
                                                      ReadVec3(componentJson, "position", glm::vec3(0.0f)),
                                                      ReadVec3(componentJson, "rotation", glm::vec3(0.0f)),
                                                      ReadVec3(componentJson, "scale", glm::vec3(1.0f)));
                    });

    loaders.emplace("Render",
                    [](Entity entity, simdjson::ondemand::object componentJson, World &world, ResourceManager &resourceManager)
                    {
                        const std::string modelLocation(componentJson["model"].get_string().value());
                        const std::string fullModelPath = "assets/" + modelLocation;
                        std::shared_ptr<Meshes> meshes = resourceManager.LoadLazy<Meshes>(
                            fullModelPath,
                            [&fullModelPath]()
                            {
                                return std::make_shared<Meshes>(Loader::Load(fullModelPath));
                            });

                        const std::string shaderLocation(componentJson["shader"].get_string().value());
                        const std::string vertPath = "assets/" + shaderLocation + ".vert";
                        const std::string fragPath = "assets/" + shaderLocation + ".frag";
                        std::shared_ptr<Shader> shader = resourceManager.Load<Shader>(shaderLocation, vertPath, fragPath);

                        MeshRenderer *meshRenderer = world.AddComponent<MeshRenderer>(entity);
                        meshRenderer->SetMeshes(meshes);
                        meshRenderer->SetShader(shader.get());
                    });

    loaders.emplace("Light",
                    [](Entity entity, simdjson::ondemand::object componentJson, World &world, ResourceManager &resourceManager)
                    {
                        Light *light = world.AddComponent<Light>(entity);
                        light->ambient = ReadVec3(componentJson, "ambient", glm::vec3(1.0f));
                        light->diffuse = ReadVec3(componentJson, "diffuse", glm::vec3(1.0f));
                        light->specular = ReadVec3(componentJson, "specular", glm::vec3(1.0f));
                        light->color = ReadVec3(componentJson, "color", glm::vec3(1.0f));
                        light->position = ReadVec3(componentJson, "position", glm::vec3(0.0f));
                    });

    loaders.emplace("Camera",
                    [](Entity entity, simdjson::ondemand::object componentJson, World &world, ResourceManager &resourceManager)
                    {
                        world.AddComponent<CameraComponent>(entity,
                                                            ReadFrustrum(componentJson),
                                                            ReadVec3(componentJson, "position", glm::vec3(0.0f)),
                                                            ReadVec3(componentJson, "lookAt", glm::vec3(0.0f)),
                                                            ReadVec3(componentJson, "upVector", glm::vec3(0.0f, 1.0f, 0.0f)),
                                                            true);
                    });
}

void SceneLoader::RegisterComponentLoader(std::string_view type, ComponentLoader loader)
{
    RegisterBuiltInComponentLoaders();
    GetComponentLoaders()[std::string(type)] = std::move(loader);
}

bool SceneLoader::LoadIntoWorld(const std::string &filepath, World *world, ResourceManager *resourceManager)
{
    RegisterBuiltInComponentLoaders();

    using namespace simdjson;
    static ondemand::parser parser; // will it be enough to not reallocate memory for the parser each time we load a scene ?
    padded_string json = padded_string::load(filepath);
    ondemand::document document = parser.iterate(json);

    INFO("Loading scene from " << filepath);
    ondemand::array entities = document["entities"].get_array();
    if (!entities.is_empty())
    {
        for (ondemand::object entityJson : entities)
        {
            std::string_view name = entityJson["name"].get_string();
            DEBUG("Detected entity: " << name);
            Entity entity = world->CreateEntity();

            ondemand::array components = entityJson["components"].get_array();
            if (!components.is_empty())
            {
                for (ondemand::object componentJson : components)
                {
                    std::string_view type = componentJson["type"].get_string();

                    ComponentLoaderMap &loaders = GetComponentLoaders();
                    auto loaderIt = loaders.find(std::string(type));
                    if (loaderIt == loaders.end())
                    {
                        WARNING("\tNo component of type: " << type);
                        continue;
                    }

                    DEBUG("\tAdding component: " << type);
                    loaderIt->second(entity, componentJson, *world, *resourceManager);
                }
            }
            else
            {
                DEBUG("\tNo components to load.");
            }
        }
    }
    else
    {
        DEBUG("No entities to load.");
    }

    return true;
}

std::shared_ptr<Scene> SceneLoader::LoadScene(const std::string &filepath, World *world, ResourceManager *resourceManager)
{
    if (!LoadIntoWorld(filepath, world, resourceManager))
        return nullptr;

    return std::make_shared<BaseScene>(world, resourceManager);
}
