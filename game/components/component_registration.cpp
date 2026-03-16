#include "component_registration.hpp"

#include <simdjson.h>

#include <helpers/log.hpp>
#include <resources/loaders/scene_loader.hpp>

#include "../components/orbit_camera_controller.hpp"

namespace GameSceneComponents
{
    // TODO: same here, not sure about registering custom component like this, but we'll see
    void RegisterAll()
    {
        SceneLoader::RegisterComponentLoader("OrbitCameraController",
                                             [](Entity entity,
                                                simdjson::ondemand::object componentJson,
                                                World &world,
                                                ResourceManager &resourceManager)
                                             {
                                                 OrbitCameraController *controller = world.AddComponent<OrbitCameraController>(entity);
                                                 controller->target = SceneLoader::ReadVec3(componentJson, "target", glm::vec3(0.0f));
                                                 controller->upVector = SceneLoader::ReadVec3(componentJson, "upVector", glm::vec3(0.0f, 1.0f, 0.0f));
                                                 controller->radius = SceneLoader::ReadFloat(componentJson, "radius", 9.0f);
                                                 controller->yaw = SceneLoader::ReadFloat(componentJson, "yaw", 0.0f);
                                                 controller->pitch = SceneLoader::ReadFloat(componentJson, "pitch", 0.0f);
                                                 controller->yawSpeed = SceneLoader::ReadFloat(componentJson, "yawSpeed", glm::quarter_pi<float>() * 0.5f);
                                                 controller->pitchSpeed = SceneLoader::ReadFloat(componentJson, "pitchSpeed", glm::quarter_pi<float>());
                                                 controller->maxPitch = SceneLoader::ReadFloat(componentJson, "maxPitch", glm::radians(75.0f));
                                             });
    }
}