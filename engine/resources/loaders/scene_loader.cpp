#include "scene_loader.hpp"

#include <algorithm>

ComponentLoaderMap &SceneLoader::GetComponentLoaders()
{
    static ComponentLoaderMap loaders;
    return loaders;
}

bool SceneLoader::readOptionalString(simdjson::ondemand::object componentJson, const char *fieldName, std::string &outValue)
{
    try
    {
        simdjson::simdjson_result<simdjson::ondemand::value> valueResult = componentJson[fieldName];
        if (valueResult.error())
            return false;

        simdjson::ondemand::value value = valueResult.value();
        if (value.is_null())
            return false;

        simdjson::simdjson_result<std::string_view> stringResult = value.get_string();
        if (stringResult.error())
            return false;

        outValue = std::string(stringResult.value());
        return true;
    }
    catch (const simdjson::simdjson_error &)
    {
        return false;
    }
}

float SceneLoader::ReadFloat(simdjson::ondemand::object componentJson, const char *fieldName, float defaultValue)
{
    try
    {
        simdjson::simdjson_result<simdjson::ondemand::value> valueResult = componentJson[fieldName];
        if (valueResult.error())
            return defaultValue;

        simdjson::ondemand::value value = valueResult.value();
        if (value.is_null())
            return defaultValue;

        simdjson::simdjson_result<double> result = value.get_double();
        if (result.error())
            return defaultValue;

        return static_cast<float>(result.value());
    }
    catch (const simdjson::simdjson_error &)
    {
        return defaultValue;
    }
}

glm::vec3 SceneLoader::ReadVec3(simdjson::ondemand::object componentJson, const char *fieldName, const glm::vec3 &defaultValue)
{
    glm::vec3 result = defaultValue;
    try
    {
        simdjson::simdjson_result<simdjson::ondemand::array> valuesResult = componentJson[fieldName].get_array();
        if (valuesResult.error())
            return result;

        simdjson::ondemand::array values = valuesResult.value();
        int index = 0;
        for (auto value : values)
        {
            if (index >= 3)
                break;

            simdjson::simdjson_result<double> number = value.get_double();
            if (number.error())
                continue;

            result[index++] = static_cast<float>(number.value());
        }
    }
    catch (const simdjson::simdjson_error &)
    {
        return result;
    }

    return result;
}

RenderQueue SceneLoader::ReadRenderQueue(simdjson::ondemand::object componentJson, RenderQueue defaultValue)
{
    try
    {
        simdjson::simdjson_result<std::string_view> valueResult = componentJson["queue"].get_string();
        if (valueResult.error())
            return defaultValue;

        std::string queueName(valueResult.value());
        std::transform(queueName.begin(), queueName.end(), queueName.begin(),
                       [](unsigned char c)
                       { return static_cast<char>(std::tolower(c)); });

        if (queueName == "lit")
            return RenderQueue::Lit;
        if (queueName == "transparent")
            return RenderQueue::Transparent;
        if (queueName == "unlit")
            return RenderQueue::Unlit;
        if (queueName == "debug")
            return RenderQueue::Debug;
    }
    catch (const simdjson::simdjson_error &)
    {
    }

    return defaultValue;
}

std::vector<float> SceneLoader::ReadFloatArray(simdjson::ondemand::array values, size_t maxValues, float defaultValue)
{
    std::vector<float> result(maxValues, defaultValue);
    try
    {
        size_t index = 0;
        for (auto value : values)
        {
            if (index >= maxValues)
                break;

            simdjson::simdjson_result<double> number = value.get_double();
            if (number.error())
                continue;

            result[index++] = static_cast<float>(number.value());
        }
    }
    catch (const simdjson::simdjson_error &)
    {
        return result;
    }

    return result;
}

PerspectiveProjection::Frustrum SceneLoader::ReadFrustrum(simdjson::ondemand::object componentJson)
{
    const PerspectiveProjection::Frustrum defaultFrustrum{45.0f, 1.0f, 1.0f, 0.1f, 150.0f};

    try
    {
        simdjson::simdjson_result<simdjson::ondemand::array> valuesResult = componentJson["frustrum"].get_array();
        if (valuesResult.error())
            return defaultFrustrum;

        const std::vector<float> values = ReadFloatArray(valuesResult.value(), 5, 0.0f);
        if (values.size() < 5)
            return defaultFrustrum;

        if (values[1] <= 0.0f || values[2] <= 0.0f || values[3] <= 0.0f || values[4] <= values[3])
            return defaultFrustrum;

        return {
            values[0],
            values[1],
            values[2],
            values[3],
            values[4]};
    }
    catch (const simdjson::simdjson_error &)
    {
        return defaultFrustrum;
    }
}

LightType SceneLoader::ReadLightType(simdjson::ondemand::object componentJson, LightType defaultValue)
{
    try
    {
        simdjson::simdjson_result<std::string_view> typeResult = componentJson["lightType"].get_string();
        if (typeResult.error())
            return defaultValue;

        std::string_view lightType = typeResult.value();

        if (lightType == "directional")
        {
            return LightType::Directional;
        }
        else if (lightType == "point")
        {
            return LightType::Point;
        }
        else if (lightType == "spot" || lightType == "spotlight")
        {
            return LightType::Spot;
        }
    }
    catch (const simdjson::simdjson_error &)
    {
    }
    return defaultValue;
}

std::array<float, 3> SceneLoader::ReadLightAttenuation(simdjson::ondemand::object componentJson)
{
    try
    {
        simdjson::simdjson_result<simdjson::ondemand::object> attenuationResult = componentJson["attenuation"].get_object();
        if (!attenuationResult.error())
        {
            // default values cover distance of 50
            simdjson::ondemand::object attenuationJson = attenuationResult.value();
            float constant = ReadFloat(attenuationJson, "constant", 1.0);
            float linear = ReadFloat(attenuationJson, "linear", 0.09);
            float quadratic = ReadFloat(attenuationJson, "quadratic", 0.032);
            return {constant, linear, quadratic};
        }
    }
    catch (const simdjson::simdjson_error &)
    {
    }
    return {1.0f, 0.09f, 0.032f};
}

std::array<float, 2> SceneLoader::ReadSpotValues(simdjson::ondemand::object componentJson)
{
    try
    {
        simdjson::simdjson_result<simdjson::ondemand::object> spotResult = componentJson["spot"].get_object();
        if (!spotResult.error())
        {
            simdjson::ondemand::object spotJson = spotResult.value();
            float innerCutoff = ReadFloat(spotJson, "innerCutoff", 12.5f);
            float outerCutoff = ReadFloat(spotJson, "outerCutoff", 17.5f);
            return {innerCutoff, outerCutoff};
        }
    }
    catch (const simdjson::simdjson_error &)
    {
    }

    return {12.5f, 17.5f};
}

void SceneLoader::RegisterBuiltInComponentLoaders()
{
    static bool registered = false;
    if (registered)
        return;

    registered = true;
    ComponentLoaderMap &loaders = GetComponentLoaders();

    loaders.emplace("Transform",
                    [](Entity entity, simdjson::ondemand::object componentJson, World &world, resources::ResourceManager *resourceManager)
                    {
                        world.AddComponent<Transform>(entity,
                                                      ReadVec3(componentJson, "position", glm::vec3(0.0f)),
                                                      ReadVec3(componentJson, "rotation", glm::vec3(0.0f)),
                                                      ReadVec3(componentJson, "scale", glm::vec3(1.0f)));
                    });

    loaders.emplace("Render",
                    [](Entity entity, simdjson::ondemand::object componentJson, World &world, resources::ResourceManager *resourceManager)
                    {
                        if (!resourceManager)
                        {
                            WARNING("Render component requires resources::ResourceManager in the new resource pipeline");
                            return;
                        }

                        const std::string modelLocation(componentJson["model"].get_string().value());
                        const std::string fullModelPath = "assets/" + modelLocation;

                        std::string shaderLocation;
                        std::string fullShaderPath;
                        simdjson::simdjson_result<std::string_view> shaderResult = componentJson["shader"].get_string();
                        if (!shaderResult.error())
                        {
                            shaderLocation = std::string(shaderResult.value());
                            fullShaderPath = "assets/" + shaderLocation;
                        }

                        std::string materialLocation;
                        std::string fullMaterialPath;
                        simdjson::simdjson_result<std::string_view> materialResult = componentJson["material"].get_string();
                        if (!materialResult.error())
                        {
                            materialLocation = std::string(materialResult.value());
                            fullMaterialPath = "assets/" + materialLocation;
                        }

                        MeshRenderer *meshRenderer = world.AddComponent<MeshRenderer>(entity);

                        meshRenderer->SetModelHandle(resourceManager->Load<resources::Model>(fullModelPath));

                        if (!fullShaderPath.empty())
                        {
                            meshRenderer->SetShaderHandle(resourceManager->Load<resources::Shader>(fullShaderPath));
                        }

                        if (!fullMaterialPath.empty())
                        {
                            meshRenderer->SetMaterialHandle(resourceManager->Load<resources::Material>(fullMaterialPath));
                        }

                        MaterialData materialData = meshRenderer->GetMaterialData();
                        materialData.roughness = std::clamp(ReadFloat(componentJson, "roughness", 0.5f), 0.02f, 1.0f);
                        meshRenderer->SetMaterialData(materialData);

                        RenderQueue inferredQueue = RenderQueue::Lit;
                        if (!shaderLocation.empty() && shaderLocation.find("unlit") != std::string::npos)
                            inferredQueue = RenderQueue::Unlit;
                        meshRenderer->SetQueue(ReadRenderQueue(componentJson, inferredQueue));
                    });

    loaders.emplace("Light",
                    [](Entity entity, simdjson::ondemand::object componentJson, World &world, resources::ResourceManager *resourceManager)
                    {
                        (void)resourceManager;

                        Light *light = world.AddComponent<Light>(entity);
                        light->ambient = ReadVec3(componentJson, "ambient", glm::vec3(0.2f));
                        light->diffuse = ReadVec3(componentJson, "diffuse", glm::vec3(0.5f));
                        light->specular = ReadVec3(componentJson, "specular", glm::vec3(0.7f));
                        light->color = ReadVec3(componentJson, "color", glm::vec3(1.0f));
                        light->intensity = ReadFloat(componentJson, "intensity", 1.0f);
                        light->direction = ReadVec3(componentJson, "direction", glm::vec3(0.0f, -1.0f, 0.0f));
                        light->type = ReadLightType(componentJson, LightType::Point);
                        std::array<float, 3> attenuation = ReadLightAttenuation(componentJson);
                        light->constant = attenuation[0];
                        light->linear = attenuation[1];
                        light->quadratic = attenuation[2];
                        std::array<float, 2> spotValues = ReadSpotValues(componentJson);
                        light->innerCutoff = spotValues[0];
                        light->outerCutoff = spotValues[1];
                    });

    loaders.emplace("Camera",
                    [](Entity entity, simdjson::ondemand::object componentJson, World &world, resources::ResourceManager *resourceManager)
                    {
                        world.AddComponent<Camera>(entity,
                                                   ReadFrustrum(componentJson),
                                                   ReadVec3(componentJson, "lookAt", glm::vec3(0.0f)),
                                                   ReadVec3(componentJson, "upVector", glm::vec3(0.0f, 1.0f, 0.0f)),
                                                   true);
                    });

    loaders.emplace("Skybox",
                    [](Entity entity, simdjson::ondemand::object componentJson, World &world, resources::ResourceManager *resourceManager)
                    {
                        if (!resourceManager)
                        {
                            return;
                        }

                        Skybox *skybox = world.AddComponent<Skybox>(entity);
                        std::string materialPath = "assets/materials/skybox.json";
                        std::string modelPath = "assets/meshes/cube/cube.obj";
                        readOptionalString(componentJson, "material", materialPath);
                        readOptionalString(componentJson, "model", modelPath);

                        skybox->materialHandle = resourceManager->Load<resources::Material>(materialPath);
                        skybox->modelHandle = resourceManager->Load<resources::Model>(modelPath);
                        skybox->intensity = SceneLoader::ReadFloat(componentJson, "intensity", 1.0f);
                        skybox->scale = SceneLoader::ReadFloat(componentJson, "scale", 50.0f);
                    });
}

void SceneLoader::RegisterComponentLoader(std::string_view type, ComponentLoader loader)
{
    RegisterBuiltInComponentLoaders();
    GetComponentLoaders()[std::string(type)] = std::move(loader);
}

bool SceneLoader::LoadIntoWorld(const std::string &filepath, World *world, resources::ResourceManager *resourceManager)
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
                    loaderIt->second(entity, componentJson, *world, resourceManager);
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

std::shared_ptr<Scene> SceneLoader::LoadScene(const std::string &filepath, World *world, resources::ResourceManager *resourceManager)
{
    if (!LoadIntoWorld(filepath, world, resourceManager))
        return nullptr;

    return std::make_shared<BaseScene>(world, resourceManager);
}
