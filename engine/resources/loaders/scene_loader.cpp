#include "scene_loader.hpp"

std::shared_ptr<Scene> SceneLoader::LoadScene(const std::string &filepath, World *world, ResourceManager *resourceManager)
{
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
            INFO("Detected entity: " << name);
            Entity entity = world->CreateEntity();

            ondemand::array components = entityJson["components"].get_array();
            if (!components.is_empty())
            {
                for (ondemand::object componentJson : components)
                {
                    // TODO: refactor to use one method per component (maybe even including these handler inside their own component)
                    std::string_view type = componentJson["type"].get_string();
                    if (type == "Transform")
                    {
                        glm::vec3 position(0.0f);
                        ondemand::array positionJson = componentJson["position"].get_array();
                        int i = 0;
                        for (double value : positionJson)
                        {
                            if (i < 3)
                                position[i++] = value;
                            else
                                break;
                        }

                        glm::vec3 rotation(0.0f);
                        ondemand::array rotationJson = componentJson["rotation"].get_array();
                        i = 0;
                        for (double value : rotationJson)
                        {
                            if (i < 3)
                                rotation[i++] = value;
                            else
                                break;
                        }

                        glm::vec3 scale(1.0f);
                        ondemand::array scaleJson = componentJson["scale"].get_array();
                        i = 0;
                        for (double value : scaleJson)
                        {
                            if (i < 3)
                                scale[i++] = value;
                            else
                                break;
                        }

                        world->AddComponent<Transform>(entity, position, rotation, scale);
                    }
                    else if (type == "Render")
                    {
                        std::string_view modelLocation = componentJson["model"].get_string();
                        std::string fullModelPath = "assets/" + std::string{modelLocation};
                        std::shared_ptr<Meshes> meshes = resourceManager->Load<Meshes>(fullModelPath, Loader::Load(fullModelPath));

                        std::string_view shaderLocation = componentJson["shader"].get_string();
                        std::string shaderName = shaderLocation.data();
                        std::string vertPath = "assets/" + std::string{shaderLocation} + ".vert";
                        std::string fragPath = "assets/" + std::string{shaderLocation} + ".frag";
                        std::shared_ptr<Shader> shader = resourceManager->Load<Shader>(shaderName, vertPath, fragPath);

                        MeshRenderer *meshRenderer = world->AddComponent<MeshRenderer>(entity);
                        meshRenderer->SetMeshes(meshes);
                        meshRenderer->SetShader(shader.get());
                    }
                    else if (type == "Light")
                    {
                        glm::vec3 ambient(1.0f);
                        ondemand::array ambientJson = componentJson["ambient"].get_array();
                        int i = 0;
                        for (double value : ambientJson)
                        {
                            if (i < 3)
                                ambient[i++] = value;
                            else
                                break;
                        }

                        glm::vec3 diffuse(1.0f);
                        ondemand::array diffuseJson = componentJson["diffuse"].get_array();
                        i = 0;
                        for (double value : diffuseJson)
                        {
                            if (i < 3)
                                diffuse[i++] = value;
                            else
                                break;
                        }

                        glm::vec3 specular(1.0f);
                        ondemand::array specularJson = componentJson["specular"].get_array();
                        i = 0;
                        for (double value : specularJson)
                        {
                            if (i < 3)
                                specular[i++] = value;
                            else
                                break;
                        }

                        glm::vec3 color(1.0f);
                        ondemand::array colorJson = componentJson["color"].get_array();
                        i = 0;
                        for (double value : colorJson)
                        {
                            if (i < 3)
                                color[i++] = value;
                            else
                                break;
                        }

                        glm::vec3 position(0.0f);
                        ondemand::array positionJson = componentJson["position"].get_array();
                        i = 0;
                        for (double value : positionJson)
                        {
                            if (i < 3)
                                position[i++] = value;
                            else
                                break;
                        }

                        Light *light = world->AddComponent<Light>(entity);
                        light->ambient = ambient;
                        light->diffuse = diffuse;
                        light->specular = specular;
                        light->color = color;
                        light->position = position;
                    }
                    else if (type == "Camera")
                    {
                        // need to pass by vector for this one because there is no vec5 I guess I'll need to unify a bit what is used
                        std::vector<float> frustrumBuffer = {};
                        frustrumBuffer.assign(5, 0.0f);
                        ondemand::array frustrumJson = componentJson["frustrum"].get_array();
                        int i = 0;
                        for (double value : frustrumJson)
                        {
                            if (i < 5)
                                frustrumBuffer[i++] = value;
                            else
                                break;
                        }
                        PerspectiveCamera::Frustrum frustrum = {
                            frustrumBuffer.at(0),
                            frustrumBuffer.at(1),
                            frustrumBuffer.at(2),
                            frustrumBuffer.at(3),
                            frustrumBuffer.at(4)};

                        glm::vec3 position{0.0f};
                        ondemand::array positionJson = componentJson["position"].get_array();
                        i = 0;
                        for (double value : positionJson)
                        {
                            if (i < 3)
                                position[i++] = value;
                            else
                                break;
                        }

                        glm::vec3 lookAt{0.0f};
                        ondemand::array lookAtJson = componentJson["lookAt"].get_array();
                        i = 0;
                        for (double value : lookAtJson)
                        {
                            if (i < 3)
                                lookAt[i++] = value;
                            else
                                break;
                        }

                        glm::vec3 upVector{0.0f};
                        ondemand::array upVectorJson = componentJson["upVector"].get_array();
                        i = 0;
                        for (double value : upVectorJson)
                        {
                            if (i < 3)
                                upVector[i++] = value;
                            else
                                break;
                        }

                        // TODO: IMPORTANT FIX THIS I JUST WANT TO SEE THE SCENE NOW
                        static PerspectiveCamera camera = {frustrum, position, lookAt, upVector};
                        world->AddComponent<CameraComponent>(entity, camera, true);
                    }
                    else
                    {
                        WARNING("No component of type: " << type);
                    }
                }
            }
            else
            {
                DEBUG("No components to load.");
            }
        }
    }
    else
    {
        DEBUG("No entities to load.");
    }

    return std::make_shared<DeclarativeScene>(world, resourceManager);
}
