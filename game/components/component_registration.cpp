#include "component_registration.hpp"

#include <simdjson.h>

#include <helpers/log.hpp>
#include <resources/loaders/scene_loader.hpp>
#include <ecs/components/transform.hpp>
#include <ecs/components/skybox.hpp>

#include "../components/light_orbit_controller.hpp"
#include "../components/orbit_camera_controller.hpp"
#include "../components/player_controller.hpp"

namespace GameComponentLoaders
{
    void RegisterAll()
    {
        SceneLoader::RegisterComponentLoader("OrbitCameraController",
                                             [](Entity entity,
                                                simdjson::ondemand::object componentJson,
                                                World &world,
                                                resources::ResourceManager *resourceManager)
                                             {
                                                 (void)resourceManager;

                                                 OrbitCameraController *controller = world.AddComponent<OrbitCameraController>(entity);
                                                 controller->target = SceneLoader::ReadVec3(componentJson, "target", glm::vec3(0.0f));
                                                 controller->upVector = SceneLoader::ReadVec3(componentJson, "upVector", glm::vec3(0.0f, 1.0f, 0.0f));
                                                 controller->radius = SceneLoader::ReadFloat(componentJson, "radius", 9.0f);
                                                 controller->yaw = SceneLoader::ReadFloat(componentJson, "yaw", 0.0f);
                                                 controller->pitch = SceneLoader::ReadFloat(componentJson, "pitch", 0.0f);
                                                 controller->yawSpeed = SceneLoader::ReadFloat(componentJson, "yawSpeed", glm::quarter_pi<float>() * 0.5f);
                                                 controller->pitchSpeed = SceneLoader::ReadFloat(componentJson, "pitchSpeed", glm::quarter_pi<float>());
                                                 controller->zoomSpeed = SceneLoader::ReadFloat(componentJson, "zoomSpeed", 0.75f);
                                                 controller->minRadius = SceneLoader::ReadFloat(componentJson, "minRadius", 1.5f);
                                                 controller->maxRadius = SceneLoader::ReadFloat(componentJson, "maxRadius", 25.0f);
                                                 if (controller->maxRadius < controller->minRadius)
                                                     std::swap(controller->minRadius, controller->maxRadius);
                                                 controller->radius = glm::clamp(controller->radius, controller->minRadius, controller->maxRadius);
                                                 controller->maxPitch = SceneLoader::ReadFloat(componentJson, "maxPitch", glm::radians(75.0f));
                                             });

        SceneLoader::RegisterComponentLoader("LightOrbitController",
                                             [](Entity entity,
                                                simdjson::ondemand::object componentJson,
                                                World &world,
                                                resources::ResourceManager *resourceManager)
                                             {
                                                 (void)resourceManager;

                                                 auto readOptionalFloat = [](simdjson::ondemand::object json,
                                                                             const char *fieldName,
                                                                             float *outValue) -> bool
                                                 {
                                                     try
                                                     {
                                                         simdjson::simdjson_result<simdjson::ondemand::value> valueResult = json[fieldName];
                                                         if (valueResult.error())
                                                             return false;

                                                         simdjson::ondemand::value value = valueResult.value();
                                                         if (value.is_null())
                                                             return false;

                                                         simdjson::simdjson_result<double> number = value.get_double();
                                                         if (number.error())
                                                             return false;

                                                         *outValue = static_cast<float>(number.value());
                                                         return true;
                                                     }
                                                     catch (const simdjson::simdjson_error &)
                                                     {
                                                         return false;
                                                     }
                                                 };

                                                 LightOrbitController *controller = world.AddComponent<LightOrbitController>(entity);
                                                 Transform *transform = world.GetComponent<Transform>(entity);
                                                 controller->center = SceneLoader::ReadVec3(componentJson,
                                                                                            "center",
                                                                                            transform ? transform->position : glm::vec3(0.0f));
                                                 controller->axisRadii = SceneLoader::ReadVec3(componentJson, "axisRadii", glm::vec3(2.5f, 1.25f, 2.5f));
                                                 controller->angularSpeeds = SceneLoader::ReadVec3(componentJson,
                                                                                                   "angularSpeeds",
                                                                                                   glm::vec3(glm::radians(35.0f), glm::radians(55.0f), glm::radians(25.0f)));
                                                 controller->angles = SceneLoader::ReadVec3(componentJson,
                                                                                            "angles",
                                                                                            glm::vec3(0.0f, glm::quarter_pi<float>(), glm::half_pi<float>()));

                                                 float radiusXZ = controller->axisRadii.x;
                                                 float radiusY = controller->axisRadii.y;
                                                 float speedXZ = controller->angularSpeeds.x;
                                                 float speedY = controller->angularSpeeds.y;
                                                 float phaseXZ = controller->angles.x;
                                                 float phaseY = controller->angles.y;

                                                 const bool hasRadiusXZ = readOptionalFloat(componentJson, "radiusXZ", &radiusXZ);
                                                 const bool hasRadiusY = readOptionalFloat(componentJson, "radiusY", &radiusY);
                                                 const bool hasSpeedXZ = readOptionalFloat(componentJson, "speedXZ", &speedXZ);
                                                 const bool hasSpeedY = readOptionalFloat(componentJson, "speedY", &speedY);
                                                 const bool hasPhaseXZ = readOptionalFloat(componentJson, "phaseXZ", &phaseXZ);
                                                 const bool hasPhaseY = readOptionalFloat(componentJson, "phaseY", &phaseY);

                                                 if (hasRadiusXZ || hasRadiusY || hasSpeedXZ || hasSpeedY || hasPhaseXZ || hasPhaseY)
                                                 {
                                                     controller->axisRadii = glm::vec3(radiusXZ, radiusY, radiusXZ);
                                                     controller->angularSpeeds = glm::vec3(speedXZ, speedY, speedXZ);
                                                     controller->angles = glm::vec3(phaseXZ, phaseY, phaseXZ + glm::half_pi<float>());
                                                 }

                                                 controller->initialized = true;
                                             });

        SceneLoader::RegisterComponentLoader("PlayerController",
                                             [](Entity entity,
                                                simdjson::ondemand::object componentJson,
                                                World &world,
                                                resources::ResourceManager *resourceManager)
                                             {
                                                 (void)resourceManager;

                                                 PlayerController *controller = world.AddComponent<PlayerController>(entity);
                                                 controller->moveSpeed = SceneLoader::ReadFloat(componentJson, "moveSpeed", controller->moveSpeed);
                                                 controller->turnSpeedDegrees = SceneLoader::ReadFloat(componentJson, "turnSpeedDegrees", controller->turnSpeedDegrees);
                                                 controller->cameraDistance = SceneLoader::ReadFloat(componentJson, "cameraDistance", controller->cameraDistance);
                                                 controller->cameraHeight = SceneLoader::ReadFloat(componentJson, "cameraHeight", controller->cameraHeight);
                                                 controller->lookAtHeight = SceneLoader::ReadFloat(componentJson, "lookAtHeight", controller->lookAtHeight);
                                                 controller->cameraForward = glm::normalize(SceneLoader::ReadVec3(componentJson, "cameraForward", controller->cameraForward));
                                             });
    }
}
