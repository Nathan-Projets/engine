#include "game_scene.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>

#include <glm/gtc/matrix_inverse.hpp>

#include "../../engine/helpers/log.hpp"
#include "../../engine/input/input_manager.hpp"
#include "../../engine/ecs/animation/animation_runtime.hpp"
#include "../../engine/ecs/components/animation_player.hpp"
#include "../../engine/ecs/components/camera.hpp"
#include "../../engine/ecs/components/light.hpp"
#include "../../engine/ecs/components/mesh_renderer.hpp"
#include "../../engine/ecs/components/name.hpp"
#include "../../engine/ecs/components/skeleton_pose.hpp"
#include "../../engine/ecs/components/skybox.hpp"
#include "../../engine/ecs/components/transform.hpp"
#include "../../engine/ecs/systems/animation_system.hpp"
#include "../../engine/ecs/systems/physics_system.hpp"
#include "../../engine/ecs/systems/render_system.hpp"
#include "../../engine/resources/loaders/scene_loader.hpp"
#include "../components/light_orbit_controller.hpp"
#include "../components/orbit_camera_controller.hpp"
#include "runtime_scene_support.hpp"
#include "../../engine/physics/collider.hpp"
#include "../../engine/physics/rigidbody.hpp"
#include "../systems/light_orbit_controller_system.hpp"
#include "../systems/orbit_camera_controller_system.hpp"

namespace
{
    struct Ray
    {
        glm::vec3 origin{0.0f};
        glm::vec3 direction{0.0f, 0.0f, -1.0f};
    };

    struct SceneBounds
    {
        bool valid = false;
        bool complete = true;
        int contributorCount = 0;
        glm::vec3 min = glm::vec3(0.0f);
        glm::vec3 max = glm::vec3(0.0f);
    };

    void WriteIndent(std::ostream &stream, int indent)
    {
        for (int index = 0; index < indent; ++index)
        {
            stream.put(' ');
        }
    }

    std::string EscapeJson(std::string_view value)
    {
        std::string escaped;
        escaped.reserve(value.size() + 8);
        for (char character : value)
        {
            switch (character)
            {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped.push_back(character);
                break;
            }
        }
        return escaped;
    }

    std::string FloatToJson(float value)
    {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(4) << value;
        std::string text = stream.str();
        while (!text.empty() && text.back() == '0')
        {
            text.pop_back();
        }
        if (!text.empty() && text.back() == '.')
        {
            text.push_back('0');
        }
        if (text.empty())
        {
            return "0.0";
        }
        return text;
    }

    void WriteVec3(std::ostream &stream, const glm::vec3 &value)
    {
        stream << "[" << FloatToJson(value.x) << ", " << FloatToJson(value.y) << ", " << FloatToJson(value.z) << "]";
    }

    std::string StripAssetsPrefix(const std::string &path)
    {
        const std::string normalized = std::filesystem::path(path).generic_string();
        const std::string prefix = "assets/";
        if (normalized.rfind(prefix, 0) == 0)
        {
            return normalized.substr(prefix.size());
        }
        return normalized;
    }

    bool HasAnyCollider(const World &world, Entity entity)
    {
        return world.HasComponent<BoxCollider>(entity) ||
               world.HasComponent<SphereCollider>(entity) ||
               world.HasComponent<CapsuleCollider>(entity);
    }

    std::string RigidbodyTypeToJson(RigidbodyType type)
    {
        switch (type)
        {
        case RigidbodyType::Static:
            return "static";
        case RigidbodyType::Kinematic:
            return "kinematic";
        default:
            return "dynamic";
        }
    }

    void WriteStringField(std::ostream &stream, int indent, std::string_view key, std::string_view value, bool withComma = true)
    {
        WriteIndent(stream, indent);
        stream << '"' << key << "\": \"" << EscapeJson(value) << '\"';
        if (withComma)
        {
            stream << ',';
        }
        stream << '\n';
    }

    void WriteFloatField(std::ostream &stream, int indent, std::string_view key, float value, bool withComma = true)
    {
        WriteIndent(stream, indent);
        stream << '"' << key << "\": " << FloatToJson(value);
        if (withComma)
        {
            stream << ',';
        }
        stream << '\n';
    }

    void WritePhysicsMaterialObject(std::ostream &stream, int indent, const PhysicsMaterial &material, bool withComma = true)
    {
        WriteIndent(stream, indent);
        stream << "\"material\": {\n";
        WriteFloatField(stream, indent + 4, "friction", material.friction);
        WriteFloatField(stream, indent + 4, "restitution", material.restitution, false);
        WriteIndent(stream, indent);
        stream << '}';
        if (withComma)
        {
            stream << ',';
        }
        stream << '\n';
    }

    void WriteBoolField(std::ostream &stream, int indent, std::string_view key, bool value, bool withComma = true)
    {
        WriteIndent(stream, indent);
        stream << '"' << key << "\": " << (value ? "true" : "false");
        if (withComma)
        {
            stream << ',';
        }
        stream << '\n';
    }

    void WriteVec3Field(std::ostream &stream, int indent, std::string_view key, const glm::vec3 &value, bool withComma = true)
    {
        WriteIndent(stream, indent);
        stream << '"' << key << "\": ";
        WriteVec3(stream, value);
        if (withComma)
        {
            stream << ',';
        }
        stream << '\n';
    }

    std::optional<EditorViewportCameraState> BuildActiveCameraState(const World &world)
    {
        for (Entity entity : world.GetEntitiesWith<Camera>())
        {
            const Camera *camera = world.GetComponent<Camera>(entity);
            const Transform *transform = world.GetComponent<Transform>(entity);
            if (!camera || !transform || !camera->main)
            {
                continue;
            }

            EditorViewportCameraState state;
            state.valid = true;
            state.view = camera->GetProjection().BuildViewMatrix(transform->position);
            state.projection = camera->GetProjection().GetProjectionMatrix();
            state.viewProjection = state.projection * state.view;
            state.invViewProjection = glm::inverse(state.viewProjection);
            state.position = transform->position;

            const PerspectiveProjection::Frustrum &frustrum = camera->GetProjection().GetFrustrum();
            state.viewportWidth = std::max(frustrum.width, 1.0f);
            state.viewportHeight = std::max(frustrum.height, 1.0f);
            return state;
        }

        return std::nullopt;
    }

    Ray BuildViewportRay(const EditorViewportCameraState &cameraState, float viewportX, float viewportY)
    {
        const float ndcX = (2.0f * viewportX / cameraState.viewportWidth) - 1.0f;
        const float ndcY = 1.0f - (2.0f * viewportY / cameraState.viewportHeight);

        glm::vec4 nearWorld = cameraState.invViewProjection * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 farWorld = cameraState.invViewProjection * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
        nearWorld /= std::max(nearWorld.w, 0.00001f);
        farWorld /= std::max(farWorld.w, 0.00001f);

        Ray ray;
        ray.origin = cameraState.position;
        ray.direction = glm::normalize(glm::vec3(farWorld - nearWorld));
        return ray;
    }

    bool IntersectRaySphere(const Ray &ray, const glm::vec3 &center, float radius, float &distance)
    {
        const glm::vec3 offset = ray.origin - center;
        const float a = glm::dot(ray.direction, ray.direction);
        const float b = 2.0f * glm::dot(offset, ray.direction);
        const float c = glm::dot(offset, offset) - radius * radius;
        const float discriminant = b * b - 4.0f * a * c;
        if (discriminant < 0.0f)
        {
            return false;
        }

        const float sqrtDiscriminant = std::sqrt(discriminant);
        const float nearDistance = (-b - sqrtDiscriminant) / (2.0f * a);
        const float farDistance = (-b + sqrtDiscriminant) / (2.0f * a);
        const float hitDistance = nearDistance >= 0.0f ? nearDistance : farDistance;
        if (hitDistance < 0.0f)
        {
            return false;
        }

        distance = hitDistance;
        return true;
    }

    bool IntersectRayAabbLocal(const Ray &ray, const glm::mat4 &worldMatrix, const glm::vec3 &boundsMin, const glm::vec3 &boundsMax, float &distance)
    {
        const glm::mat4 inverseWorld = glm::inverse(worldMatrix);
        const glm::vec3 localOrigin = glm::vec3(inverseWorld * glm::vec4(ray.origin, 1.0f));
        const glm::vec3 localDirection = glm::vec3(inverseWorld * glm::vec4(ray.direction, 0.0f));

        float tMin = 0.0f;
        float tMax = std::numeric_limits<float>::max();
        for (int axis = 0; axis < 3; ++axis)
        {
            const float direction = localDirection[axis];
            if (std::abs(direction) < 0.00001f)
            {
                if (localOrigin[axis] < boundsMin[axis] || localOrigin[axis] > boundsMax[axis])
                {
                    return false;
                }
                continue;
            }

            const float inverseDirection = 1.0f / direction;
            float t0 = (boundsMin[axis] - localOrigin[axis]) * inverseDirection;
            float t1 = (boundsMax[axis] - localOrigin[axis]) * inverseDirection;
            if (t0 > t1)
            {
                std::swap(t0, t1);
            }

            tMin = std::max(tMin, t0);
            tMax = std::min(tMax, t1);
            if (tMax < tMin)
            {
                return false;
            }
        }

        const glm::vec3 localHit = localOrigin + localDirection * tMin;
        const glm::vec3 worldHit = glm::vec3(worldMatrix * glm::vec4(localHit, 1.0f));
        distance = glm::length(worldHit - ray.origin);
        return true;
    }

    void ExpandBounds(SceneBounds &bounds, const glm::vec3 &point)
    {
        if (!bounds.valid)
        {
            bounds.valid = true;
            bounds.min = point;
            bounds.max = point;
            return;
        }

        bounds.min = glm::min(bounds.min, point);
        bounds.max = glm::max(bounds.max, point);
    }

    SceneBounds BuildSceneBounds(const World &world, resources::ResourceManager *resourceManager)
    {
        SceneBounds bounds;
        if (!resourceManager)
        {
            return bounds;
        }

        for (Entity entity : world.GetEntitiesWith<Transform, MeshRenderer>())
        {
            const Transform *transform = world.GetComponent<Transform>(entity);
            const MeshRenderer *renderer = world.GetComponent<MeshRenderer>(entity);
            if (!transform || !renderer)
            {
                continue;
            }

            if (renderer->GetQueue() == RenderQueue::Unlit && world.HasComponent<Light>(entity))
            {
                continue;
            }

            const resources::Model *model = resourceManager->Get(renderer->GetModelHandle());
            if (!model)
            {
                bounds.complete = false;
                continue;
            }

            if (!model->IsLoaded())
            {
                if (!model->HasFailed())
                {
                    bounds.complete = false;
                }
                continue;
            }

            if (model->IsEmpty())
            {
                continue;
            }

            const glm::vec3 modelMin = model->GetBoundsMin();
            const glm::vec3 modelMax = model->GetBoundsMax();
            const glm::mat4 worldMatrix = transform->GetMatrix();
            for (int x = 0; x < 2; ++x)
            {
                for (int y = 0; y < 2; ++y)
                {
                    for (int z = 0; z < 2; ++z)
                    {
                        const glm::vec3 corner(
                            x == 0 ? modelMin.x : modelMax.x,
                            y == 0 ? modelMin.y : modelMax.y,
                            z == 0 ? modelMin.z : modelMax.z);
                        ExpandBounds(bounds, glm::vec3(worldMatrix * glm::vec4(corner, 1.0f)));
                    }
                }
            }

            ++bounds.contributorCount;
        }

        return bounds;
    }

    float ComputeSceneBoundsRadius(const SceneBounds &bounds)
    {
        if (!bounds.valid)
        {
            return 0.0f;
        }

        return std::max(glm::length(bounds.max - bounds.min) * 0.5f, 0.5f);
    }

    float ComputeFitDistanceForBounds(const SceneBounds &bounds, const PerspectiveProjection::Frustrum &frustrum)
    {
        const float sceneRadius = ComputeSceneBoundsRadius(bounds);
        const float aspectRatio = std::max(frustrum.width / std::max(frustrum.height, 1.0f), 0.1f);
        const float halfVerticalFov = glm::radians(frustrum.angle) * 0.5f;
        const float halfHorizontalFov = std::atan(std::tan(halfVerticalFov) * aspectRatio);
        const float fitDistanceVertical = sceneRadius / std::max(std::tan(halfVerticalFov), 0.1f);
        const float fitDistanceHorizontal = sceneRadius / std::max(std::tan(halfHorizontalFov), 0.1f);
        return std::max(std::max(fitDistanceVertical, fitDistanceHorizontal) * 1.15f, 2.0f);
    }

    SceneBounds BuildEntityBounds(const World &world, resources::ResourceManager *resourceManager, Entity entity)
    {
        SceneBounds bounds;
        const Transform *transform = world.GetComponent<Transform>(entity);
        if (!transform)
        {
            return bounds;
        }

        const MeshRenderer *renderer = world.GetComponent<MeshRenderer>(entity);
        if (!renderer || !resourceManager)
        {
            ExpandBounds(bounds, transform->position);
            bounds.contributorCount = 1;
            return bounds;
        }

        const resources::Model *model = resourceManager->Get(renderer->GetModelHandle());
        if (!model || !model->IsLoaded() || model->IsEmpty())
        {
            ExpandBounds(bounds, transform->position);
            bounds.complete = model && model->HasFailed();
            bounds.contributorCount = 1;
            return bounds;
        }

        const glm::vec3 modelMin = model->GetBoundsMin();
        const glm::vec3 modelMax = model->GetBoundsMax();
        const glm::mat4 worldMatrix = transform->GetMatrix();
        for (int x = 0; x < 2; ++x)
        {
            for (int y = 0; y < 2; ++y)
            {
                for (int z = 0; z < 2; ++z)
                {
                    const glm::vec3 corner(
                        x == 0 ? modelMin.x : modelMax.x,
                        y == 0 ? modelMin.y : modelMax.y,
                        z == 0 ? modelMin.z : modelMax.z);
                    ExpandBounds(bounds, glm::vec3(worldMatrix * glm::vec4(corner, 1.0f)));
                }
            }
        }

        bounds.contributorCount = 1;
        return bounds;
    }

    void CopySupportedEditorComponents(World &world, Entity source, Entity target)
    {
        if (const Transform *transform = world.GetComponent<Transform>(source))
        {
            world.AddComponent<Transform>(target, transform->position, transform->rotation, transform->scale);
        }

        if (const Light *light = world.GetComponent<Light>(source))
        {
            *world.AddComponent<Light>(target) = *light;
        }

        if (const Camera *camera = world.GetComponent<Camera>(source))
        {
            Camera *targetCamera = world.AddComponent<Camera>(target,
                                                              camera->GetProjection().GetFrustrum(),
                                                              camera->GetProjection().GetLookAt(),
                                                              camera->GetProjection().GetUpVector(),
                                                              false);
            targetCamera->main = false;
        }

        if (const Rigidbody *rigidbody = world.GetComponent<Rigidbody>(source))
        {
            *world.AddComponent<Rigidbody>(target) = *rigidbody;
        }

        if (const BoxCollider *boxCollider = world.GetComponent<BoxCollider>(source))
        {
            *world.AddComponent<BoxCollider>(target) = *boxCollider;
        }

        if (const SphereCollider *sphereCollider = world.GetComponent<SphereCollider>(source))
        {
            *world.AddComponent<SphereCollider>(target) = *sphereCollider;
        }

        if (const CapsuleCollider *capsuleCollider = world.GetComponent<CapsuleCollider>(source))
        {
            *world.AddComponent<CapsuleCollider>(target) = *capsuleCollider;
        }

        if (const MeshRenderer *meshRenderer = world.GetComponent<MeshRenderer>(source))
        {
            MeshRenderer *targetRenderer = world.AddComponent<MeshRenderer>(target);
            targetRenderer->SetModelHandle(meshRenderer->GetModelHandle());
            targetRenderer->SetShaderHandle(meshRenderer->GetShaderHandle());
            targetRenderer->SetMaterialHandle(meshRenderer->GetMaterialHandle());
            targetRenderer->SetMaterialData(meshRenderer->GetMaterialData());
            targetRenderer->SetQueue(meshRenderer->GetQueue());
            targetRenderer->SetLoadTextures(meshRenderer->ShouldLoadTextures());
        }

        if (const AnimationPlayer *animation = world.GetComponent<AnimationPlayer>(source))
        {
            *world.AddComponent<AnimationPlayer>(target) = *animation;
        }

        if (const SkeletonPose *pose = world.GetComponent<SkeletonPose>(source))
        {
            *world.AddComponent<SkeletonPose>(target) = *pose;
        }

        if (const Skybox *skybox = world.GetComponent<Skybox>(source))
        {
            *world.AddComponent<Skybox>(target) = *skybox;
        }

        if (const OrbitCameraController *orbit = world.GetComponent<OrbitCameraController>(source))
        {
            *world.AddComponent<OrbitCameraController>(target) = *orbit;
        }

        if (const LightOrbitController *lightOrbit = world.GetComponent<LightOrbitController>(source))
        {
            *world.AddComponent<LightOrbitController>(target) = *lightOrbit;
        }
    }

    std::string SerializeWorldToSceneJson(const World &world, resources::ResourceManager *resourceManager, std::string_view scenePath)
    {
        std::ostringstream output;
        std::vector<Entity> entities(world.GetAllEntities().begin(), world.GetAllEntities().end());
        std::sort(entities.begin(), entities.end(), [](Entity left, Entity right)
                  { return left.GetID() < right.GetID(); });

        const std::string sceneName = std::filesystem::path(scenePath).stem().string();
        output << "{\n";
        WriteStringField(output, 4, "name", sceneName);
        WriteStringField(output, 4, "version", "1.0");
        WriteIndent(output, 4);
        output << "\"entities\": [\n";

        for (size_t entityIndex = 0; entityIndex < entities.size(); ++entityIndex)
        {
            const Entity entity = entities[entityIndex];
            const Name *name = world.GetComponent<Name>(entity);
            const Transform *transform = world.GetComponent<Transform>(entity);
            const MeshRenderer *meshRenderer = world.GetComponent<MeshRenderer>(entity);
            const Light *light = world.GetComponent<Light>(entity);
            const Camera *camera = world.GetComponent<Camera>(entity);
            const AnimationPlayer *animation = world.GetComponent<AnimationPlayer>(entity);
            const Skybox *skybox = world.GetComponent<Skybox>(entity);
            const OrbitCameraController *orbitCamera = world.GetComponent<OrbitCameraController>(entity);
            const LightOrbitController *lightOrbit = world.GetComponent<LightOrbitController>(entity);

            WriteIndent(output, 8);
            output << "{\n";
            WriteStringField(output, 12, "name", name ? name->value : ("Entity " + std::to_string(entity.GetID())));
            WriteIndent(output, 12);
            output << "\"components\": [\n";

            bool wroteAnyComponent = false;
            auto writeComponentSeparator = [&output, &wroteAnyComponent]()
            {
                if (wroteAnyComponent)
                {
                    output << ",\n";
                }
                wroteAnyComponent = true;
            };

            if (transform)
            {
                writeComponentSeparator();
                WriteIndent(output, 16);
                output << "{\n";
                WriteStringField(output, 20, "type", "Transform");
                WriteVec3Field(output, 20, "position", transform->position);
                WriteVec3Field(output, 20, "rotation", transform->rotation);
                WriteVec3Field(output, 20, "scale", transform->scale, false);
                WriteIndent(output, 16);
                output << '}';
            }

            if (const Rigidbody *rigidbody = world.GetComponent<Rigidbody>(entity))
            {
                writeComponentSeparator();
                WriteIndent(output, 16);
                output << "{\n";
                WriteStringField(output, 20, "type", "Rigidbody");
                WriteStringField(output, 20, "bodyType", RigidbodyTypeToJson(rigidbody->type));
                WriteFloatField(output, 20, "mass", rigidbody->mass);
                WriteFloatField(output, 20, "gravityScale", rigidbody->gravityScale);
                WriteFloatField(output, 20, "linearDamping", rigidbody->linearDamping);
                WriteFloatField(output, 20, "angularDamping", rigidbody->angularDamping);
                WriteBoolField(output, 20, "useGravity", rigidbody->useGravity);
                WriteBoolField(output, 20, "lockRotation", rigidbody->lockRotation);
                WriteBoolField(output, 20, "isTrigger", rigidbody->isTrigger);
                WriteVec3Field(output, 20, "linearVelocity", rigidbody->linearVelocity);
                WriteVec3Field(output, 20, "angularVelocity", rigidbody->angularVelocity, false);
                WriteIndent(output, 16);
                output << '}';
            }

            if (const BoxCollider *boxCollider = world.GetComponent<BoxCollider>(entity))
            {
                writeComponentSeparator();
                WriteIndent(output, 16);
                output << "{\n";
                WriteStringField(output, 20, "type", "BoxCollider");
                WriteVec3Field(output, 20, "center", boxCollider->center);
                WriteVec3Field(output, 20, "halfExtents", boxCollider->halfExtents);
                WriteBoolField(output, 20, "isTrigger", boxCollider->isTrigger);
                WritePhysicsMaterialObject(output, 20, boxCollider->material, false);
                WriteIndent(output, 16);
                output << '}';
            }

            if (const SphereCollider *sphereCollider = world.GetComponent<SphereCollider>(entity))
            {
                writeComponentSeparator();
                WriteIndent(output, 16);
                output << "{\n";
                WriteStringField(output, 20, "type", "SphereCollider");
                WriteVec3Field(output, 20, "center", sphereCollider->center);
                WriteFloatField(output, 20, "radius", sphereCollider->radius);
                WriteBoolField(output, 20, "isTrigger", sphereCollider->isTrigger);
                WritePhysicsMaterialObject(output, 20, sphereCollider->material, false);
                WriteIndent(output, 16);
                output << '}';
            }

            if (const CapsuleCollider *capsuleCollider = world.GetComponent<CapsuleCollider>(entity))
            {
                writeComponentSeparator();
                WriteIndent(output, 16);
                output << "{\n";
                WriteStringField(output, 20, "type", "CapsuleCollider");
                WriteVec3Field(output, 20, "center", capsuleCollider->center);
                WriteFloatField(output, 20, "radius", capsuleCollider->radius);
                WriteFloatField(output, 20, "height", capsuleCollider->height);
                WriteBoolField(output, 20, "isTrigger", capsuleCollider->isTrigger);
                WritePhysicsMaterialObject(output, 20, capsuleCollider->material, false);
                WriteIndent(output, 16);
                output << '}';
            }

            if (meshRenderer)
            {
                writeComponentSeparator();
                WriteIndent(output, 16);
                output << "{\n";
                WriteStringField(output, 20, "type", "Render");

                std::string modelPath;
                std::string materialPath;
                std::string shaderPath;
                if (resourceManager)
                {
                    if (const resources::Model *model = resourceManager->Get(meshRenderer->GetModelHandle()))
                    {
                        modelPath = StripAssetsPrefix(model->GetPath());
                    }
                    if (const resources::Material *material = resourceManager->Get(meshRenderer->GetMaterialHandle()))
                    {
                        materialPath = StripAssetsPrefix(material->GetPath());
                    }
                    if (const resources::Shader *shader = resourceManager->Get(meshRenderer->GetShaderHandle()))
                    {
                        shaderPath = StripAssetsPrefix(shader->GetPath());
                    }
                }

                WriteStringField(output, 20, "model", modelPath.empty() ? "" : modelPath);
                if (!materialPath.empty())
                {
                    WriteStringField(output, 20, "material", materialPath);
                }
                else if (!shaderPath.empty())
                {
                    WriteStringField(output, 20, "shader", shaderPath);
                }

                if (meshRenderer->GetQueue() != RenderQueue::Lit)
                {
                    std::string queueName = "lit";
                    if (meshRenderer->GetQueue() == RenderQueue::Transparent)
                        queueName = "transparent";
                    else if (meshRenderer->GetQueue() == RenderQueue::Unlit)
                        queueName = "unlit";
                    else if (meshRenderer->GetQueue() == RenderQueue::Debug)
                        queueName = "debug";
                    WriteStringField(output, 20, "queue", queueName);
                }

                WriteBoolField(output, 20, "loadTextures", meshRenderer->ShouldLoadTextures(), false);
                WriteIndent(output, 16);
                output << '}';
            }

            if (light)
            {
                writeComponentSeparator();
                WriteIndent(output, 16);
                output << "{\n";
                WriteStringField(output, 20, "type", "Light");
                std::string lightType = "point";
                if (light->type == LightType::Directional)
                    lightType = "directional";
                else if (light->type == LightType::Spot)
                    lightType = "spot";
                WriteStringField(output, 20, "lightType", lightType);
                WriteVec3Field(output, 20, "direction", light->direction);
                WriteVec3Field(output, 20, "color", light->color);
                WriteFloatField(output, 20, "intensity", light->intensity);
                WriteVec3Field(output, 20, "ambient", light->ambient);
                WriteVec3Field(output, 20, "diffuse", light->diffuse);
                WriteVec3Field(output, 20, "specular", light->specular);
                WriteBoolField(output, 20, "castShadows", light->castShadows, light->type != LightType::Directional);

                if (light->type != LightType::Directional)
                {
                    WriteIndent(output, 20);
                    output << "\"attenuation\": {\n";
                    WriteFloatField(output, 24, "constant", light->constant);
                    WriteFloatField(output, 24, "linear", light->linear);
                    WriteFloatField(output, 24, "quadratic", light->quadratic, false);
                    WriteIndent(output, 20);
                    output << "}";
                    if (light->type == LightType::Spot)
                    {
                        output << ",\n";
                        WriteIndent(output, 20);
                        output << "\"spot\": {\n";
                        WriteFloatField(output, 24, "innerCutoff", light->innerCutoff);
                        WriteFloatField(output, 24, "outerCutoff", light->outerCutoff, false);
                        WriteIndent(output, 20);
                        output << "}\n";
                    }
                    else
                    {
                        output << '\n';
                    }
                }

                WriteIndent(output, 16);
                output << '}';
            }

            if (camera)
            {
                writeComponentSeparator();
                const PerspectiveProjection &projection = camera->GetProjection();
                const PerspectiveProjection::Frustrum &frustrum = projection.GetFrustrum();
                WriteIndent(output, 16);
                output << "{\n";
                WriteStringField(output, 20, "type", "Camera");
                WriteIndent(output, 20);
                output << "\"frustrum\": ["
                       << FloatToJson(frustrum.angle) << ", "
                       << FloatToJson(frustrum.width) << ", "
                       << FloatToJson(frustrum.height) << ", "
                       << FloatToJson(frustrum.near) << ", "
                       << FloatToJson(frustrum.far) << "],\n";
                WriteVec3Field(output, 20, "lookAt", projection.GetLookAt());
                WriteVec3Field(output, 20, "upVector", projection.GetUpVector(), false);
                WriteIndent(output, 16);
                output << '}';
            }

            if (animation)
            {
                writeComponentSeparator();
                WriteIndent(output, 16);
                output << "{\n";
                WriteStringField(output, 20, "type", "Animation");
                if (!animation->clipName.empty())
                {
                    WriteStringField(output, 20, "clip", animation->clipName);
                }
                WriteFloatField(output, 20, "speed", animation->speed);
                WriteBoolField(output, 20, "loop", animation->loop);
                WriteBoolField(output, 20, "paused", animation->paused);
                WriteFloatField(output, 20, "startTime", static_cast<float>(animation->currentTimeSeconds), false);
                WriteIndent(output, 16);
                output << '}';
            }

            if (skybox)
            {
                writeComponentSeparator();
                WriteIndent(output, 16);
                output << "{\n";
                WriteStringField(output, 20, "type", "Skybox");
                if (resourceManager)
                {
                    if (const resources::Material *material = resourceManager->Get(skybox->materialHandle))
                    {
                        WriteStringField(output, 20, "material", material->GetPath());
                    }
                    if (const resources::Model *model = resourceManager->Get(skybox->modelHandle))
                    {
                        WriteStringField(output, 20, "model", model->GetPath());
                    }
                }
                WriteFloatField(output, 20, "intensity", skybox->intensity);
                WriteFloatField(output, 20, "scale", skybox->scale, false);
                WriteIndent(output, 16);
                output << '}';
            }

            if (orbitCamera)
            {
                writeComponentSeparator();
                WriteIndent(output, 16);
                output << "{\n";
                WriteStringField(output, 20, "type", "OrbitCameraController");
                WriteVec3Field(output, 20, "target", orbitCamera->target);
                WriteVec3Field(output, 20, "upVector", orbitCamera->upVector);
                WriteFloatField(output, 20, "radius", orbitCamera->radius);
                WriteFloatField(output, 20, "minRadius", orbitCamera->minRadius);
                WriteFloatField(output, 20, "maxRadius", orbitCamera->maxRadius);
                WriteFloatField(output, 20, "zoomSpeed", orbitCamera->zoomSpeed);
                WriteFloatField(output, 20, "yaw", orbitCamera->yaw);
                WriteFloatField(output, 20, "pitch", orbitCamera->pitch);
                WriteFloatField(output, 20, "yawSpeed", orbitCamera->yawSpeed);
                WriteFloatField(output, 20, "pitchSpeed", orbitCamera->pitchSpeed);
                WriteFloatField(output, 20, "maxPitch", orbitCamera->maxPitch, false);
                WriteIndent(output, 16);
                output << '}';
            }

            if (lightOrbit)
            {
                writeComponentSeparator();
                WriteIndent(output, 16);
                output << "{\n";
                WriteStringField(output, 20, "type", "LightOrbitController");
                WriteVec3Field(output, 20, "center", lightOrbit->center);
                WriteVec3Field(output, 20, "axisRadii", lightOrbit->axisRadii);
                WriteVec3Field(output, 20, "angularSpeeds", lightOrbit->angularSpeeds);
                WriteVec3Field(output, 20, "angles", lightOrbit->angles, false);
                WriteIndent(output, 16);
                output << '}';
            }

            if (wroteAnyComponent)
            {
                output << '\n';
            }
            WriteIndent(output, 12);
            output << "]\n";
            WriteIndent(output, 8);
            output << '}';
            if (entityIndex + 1 < entities.size())
            {
                output << ',';
            }
            output << '\n';
        }

        WriteIndent(output, 4);
        output << "]\n";
        output << "}\n";
        return output.str();
    }

}

EditorScene::EditorScene(std::string scenePath, resources::ResourceManager *resourceManager, bool runtimeMode) : m_resourceManager(resourceManager), m_scenePath(std::move(scenePath)), m_runtimeMode(runtimeMode)
{
}

void EditorScene::Init()
{
    ReloadScene();
}

void EditorScene::Update(float deltaTime)
{
    if (InputManager::Get().IsActionJustPressed("reload_scene_clean"))
    {
        ReloadScene(true);
    }

    if (InputManager::Get().IsActionJustPressed("reload_scene"))
    {
        ReloadScene(false);
    }

    if (m_pendingEditorCameraRefit)
    {
        m_pendingEditorCameraRefit = !TryFitEditorCameraToSceneBounds();
    }

    if (m_runtimeMode || m_editorPlayMode)
    {
        m_world.UpdateSystems(deltaTime);
    }
}

void EditorScene::Draw(float deltaTime)
{
    m_world.RenderSystems(deltaTime);
}

void EditorScene::OnResize(int width, int height)
{
    ApplyViewportSize(width, height);
}

void EditorScene::ApplyViewportSize(int width, int height)
{
    m_viewportWidth = width;
    m_viewportHeight = height;

    if (width <= 0 || height <= 0)
    {
        return;
    }

    runtime_scene_support::ApplyViewportToMainCameraAndRenderer(m_world, m_renderSystem, width, height);

    m_editorCameraFrustrum.width = static_cast<float>(width);
    m_editorCameraFrustrum.height = static_cast<float>(height);
    RebuildEditorCameraState();
}

AnimationDebugSnapshot EditorScene::GetAnimationDebugSnapshot() const
{
    AnimationDebugSnapshot snapshot;

    std::vector<Entity> entities = m_world.GetEntitiesWith<MeshRenderer, AnimationPlayer>();
    std::sort(entities.begin(), entities.end(), [](Entity left, Entity right)
              { return left.GetID() < right.GetID(); });

    snapshot.entries.reserve(entities.size());
    for (Entity entity : entities)
    {
        const MeshRenderer *meshRenderer = m_world.GetComponent<MeshRenderer>(entity);
        const AnimationPlayer *animationPlayer = m_world.GetComponent<AnimationPlayer>(entity);
        const SkeletonPose *skeletonPose = m_world.GetComponent<SkeletonPose>(entity);
        if (!meshRenderer || !animationPlayer)
            continue;

        AnimationDebugEntry entry;
        entry.entity = entity;
        entry.currentTimeSeconds = animationPlayer->currentTimeSeconds;
        entry.targetTimeSeconds = animationPlayer->targetTimeSeconds;
        entry.loop = animationPlayer->loop;
        entry.paused = animationPlayer->paused;
        entry.transitionActive = animationPlayer->transitionActive;
        entry.transitionDurationSeconds = animationPlayer->transitionDurationSeconds;
        entry.transitionAlpha = animationPlayer->transitionDurationSeconds > 0.0f
                                    ? std::clamp(animationPlayer->transitionElapsedSeconds / animationPlayer->transitionDurationSeconds, 0.0f, 1.0f)
                                    : (animationPlayer->transitionActive ? 1.0f : 0.0f);
        entry.poseValid = skeletonPose && skeletonPose->valid;

        resources::Model *model = m_resourceManager ? m_resourceManager->Get(meshRenderer->GetModelHandle()) : nullptr;
        if (!model)
        {
            entry.modelPath = "(model unavailable)";
            snapshot.entries.push_back(std::move(entry));
            continue;
        }
        entry.modelPath = model->GetPath();
        entry.boneCount = model->GetBones().size();
        const auto &clips = model->GetAnimationClips();
        entry.availableClips.reserve(clips.size());
        for (const resources::AnimationClip &clip : clips)
        {
            entry.availableClips.push_back(clip.name.empty() ? "(unnamed clip)" : clip.name);
        }

        if (const std::optional<uint32_t> clipIndex = animation::ResolveClipIndex(*model, animationPlayer->clipName, animationPlayer->clipIndex))
        {
            entry.activeClipIndex = static_cast<int>(clipIndex.value());
            if (const resources::AnimationClip *clip = model->GetAnimationClip(clipIndex.value()))
            {
                entry.activeClipName = clip->name;
                entry.clipDurationSeconds = animation::GetDurationSeconds(*clip);
                entry.ticksPerSecond = animation::GetTicksPerSecond(*clip);
                entry.channelCount = clip->channels.size();
            }
        }

        if (animationPlayer->transitionActive)
        {
            if (const std::optional<uint32_t> targetClipIndex = animation::ResolveClipIndex(*model, animationPlayer->targetClipName, animationPlayer->targetClipIndex))
            {
                entry.targetClipIndex = static_cast<int>(targetClipIndex.value());
                if (const resources::AnimationClip *targetClip = model->GetAnimationClip(targetClipIndex.value()))
                {
                    entry.targetClipName = targetClip->name;
                    entry.targetClipDurationSeconds = animation::GetDurationSeconds(*targetClip);
                }
            }
        }

        snapshot.entries.push_back(std::move(entry));
    }

    return snapshot;
}

std::vector<EditorEntitySummary> EditorScene::GetEditorEntitySummaries() const
{
    std::vector<EditorEntitySummary> entities;
    entities.reserve(m_world.GetAllEntities().size());

    for (Entity entity : m_world.GetAllEntities())
    {
        EditorEntitySummary summary;
        summary.entity = entity;

        if (const Name *name = m_world.GetComponent<Name>(entity))
        {
            summary.name = name->value;
        }
        else
        {
            summary.name = "Entity " + std::to_string(entity.GetID());
        }

        entities.push_back(std::move(summary));
    }

    return entities;
}

EditorEntityInspectorState EditorScene::GetEditorEntityInspectorState(Entity entity) const
{
    EditorEntityInspectorState state;
    if (!m_world.IsEntityActive(entity))
    {
        return state;
    }

    state.valid = true;
    state.entity = entity;
    if (const Name *name = m_world.GetComponent<Name>(entity))
    {
        state.name = name->value;
    }
    else
    {
        state.name = "Entity " + std::to_string(entity.GetID());
    }

    if (const Transform *transform = m_world.GetComponent<Transform>(entity))
    {
        state.hasTransform = true;
        state.transform.position = transform->position;
        state.transform.rotation = transform->rotation;
        state.transform.scale = transform->scale;
    }

    if (const Light *light = m_world.GetComponent<Light>(entity))
    {
        state.hasLight = true;
        state.light.ambient = light->ambient;
        state.light.diffuse = light->diffuse;
        state.light.specular = light->specular;
        state.light.color = light->color;
        state.light.direction = light->direction;
        state.light.intensity = light->intensity;
        state.light.constant = light->constant;
        state.light.linear = light->linear;
        state.light.quadratic = light->quadratic;
        state.light.innerCutoff = light->innerCutoff;
        state.light.outerCutoff = light->outerCutoff;
        state.light.type = static_cast<int>(light->type);
        state.light.castShadows = light->castShadows;
    }

    if (const Camera *camera = m_world.GetComponent<Camera>(entity))
    {
        state.hasCamera = true;
        const PerspectiveProjection &projection = camera->GetProjection();
        const PerspectiveProjection::Frustrum &frustrum = projection.GetFrustrum();
        state.camera.lookAt = projection.GetLookAt();
        state.camera.upVector = projection.GetUpVector();
        state.camera.angle = frustrum.angle;
        state.camera.width = frustrum.width;
        state.camera.height = frustrum.height;
        state.camera.nearPlane = frustrum.near;
        state.camera.farPlane = frustrum.far;
        state.camera.main = camera->main;
    }

    if (const Rigidbody *rigidbody = m_world.GetComponent<Rigidbody>(entity))
    {
        state.hasRigidbody = true;
        state.rigidbody.type = static_cast<int>(rigidbody->type);
        state.rigidbody.mass = rigidbody->mass;
        state.rigidbody.gravityScale = rigidbody->gravityScale;
        state.rigidbody.linearDamping = rigidbody->linearDamping;
        state.rigidbody.angularDamping = rigidbody->angularDamping;
        state.rigidbody.useGravity = rigidbody->useGravity;
        state.rigidbody.lockRotation = rigidbody->lockRotation;
        state.rigidbody.isTrigger = rigidbody->isTrigger;
        state.rigidbody.linearVelocity = rigidbody->linearVelocity;
        state.rigidbody.angularVelocity = rigidbody->angularVelocity;
    }

    if (const BoxCollider *boxCollider = m_world.GetComponent<BoxCollider>(entity))
    {
        state.hasBoxCollider = true;
        state.boxCollider.center = boxCollider->center;
        state.boxCollider.halfExtents = boxCollider->halfExtents;
        state.boxCollider.isTrigger = boxCollider->isTrigger;
        state.boxCollider.material.friction = boxCollider->material.friction;
        state.boxCollider.material.restitution = boxCollider->material.restitution;
    }

    if (const SphereCollider *sphereCollider = m_world.GetComponent<SphereCollider>(entity))
    {
        state.hasSphereCollider = true;
        state.sphereCollider.center = sphereCollider->center;
        state.sphereCollider.radius = sphereCollider->radius;
        state.sphereCollider.isTrigger = sphereCollider->isTrigger;
        state.sphereCollider.material.friction = sphereCollider->material.friction;
        state.sphereCollider.material.restitution = sphereCollider->material.restitution;
    }

    if (const CapsuleCollider *capsuleCollider = m_world.GetComponent<CapsuleCollider>(entity))
    {
        state.hasCapsuleCollider = true;
        state.capsuleCollider.center = capsuleCollider->center;
        state.capsuleCollider.radius = capsuleCollider->radius;
        state.capsuleCollider.height = capsuleCollider->height;
        state.capsuleCollider.isTrigger = capsuleCollider->isTrigger;
        state.capsuleCollider.material.friction = capsuleCollider->material.friction;
        state.capsuleCollider.material.restitution = capsuleCollider->material.restitution;
    }

    if (const MeshRenderer *meshRenderer = m_world.GetComponent<MeshRenderer>(entity))
    {
        state.hasMeshRenderer = true;
        state.meshRenderer.queue = static_cast<int>(meshRenderer->GetQueue());
        state.meshRenderer.loadTextures = meshRenderer->ShouldLoadTextures();
        state.meshRenderer.usesResourcePipeline = meshRenderer->UsesResourcePipeline();

        if (m_resourceManager)
        {
            if (const resources::Model *model = m_resourceManager->Get(meshRenderer->GetModelHandle()))
            {
                state.meshRenderer.modelPath = model->GetPath();
            }

            if (const resources::Material *material = m_resourceManager->Get(meshRenderer->GetMaterialHandle()))
            {
                state.meshRenderer.materialPath = material->GetPath();
            }

            if (const resources::Shader *shader = m_resourceManager->Get(meshRenderer->GetShaderHandle()))
            {
                state.meshRenderer.shaderPath = shader->GetPath();
            }
        }
    }

    if (const AnimationPlayer *animationPlayer = m_world.GetComponent<AnimationPlayer>(entity))
    {
        state.hasAnimation = true;
        state.animation.clipName = animationPlayer->clipName;
        state.animation.clipIndex = animationPlayer->clipIndex;
        state.animation.speed = animationPlayer->speed;
        state.animation.loop = animationPlayer->loop;
        state.animation.paused = animationPlayer->paused;
        state.animation.transitionActive = animationPlayer->transitionActive;
    }

    return state;
}

bool EditorScene::SetEditorEntityName(Entity entity, std::string_view name)
{
    if (!m_world.IsEntityActive(entity))
    {
        return false;
    }

    Name *entityName = m_world.GetComponent<Name>(entity);
    if (!entityName)
    {
        entityName = m_world.AddComponent<Name>(entity);
    }

    entityName->value = std::string(name);
    return true;
}

bool EditorScene::SetEditorTransform(Entity entity, const EditorTransformState &transform)
{
    Transform *target = m_world.GetComponent<Transform>(entity);
    if (!target)
    {
        return false;
    }

    target->position = transform.position;
    target->rotation = transform.rotation;
    target->scale = transform.scale;
    return true;
}

bool EditorScene::SetEditorLight(Entity entity, const EditorLightState &light)
{
    Light *target = m_world.GetComponent<Light>(entity);
    if (!target)
    {
        return false;
    }

    target->ambient = light.ambient;
    target->diffuse = light.diffuse;
    target->specular = light.specular;
    target->color = light.color;
    target->direction = light.direction;
    target->intensity = light.intensity;
    target->constant = light.constant;
    target->linear = light.linear;
    target->quadratic = light.quadratic;
    target->innerCutoff = light.innerCutoff;
    target->outerCutoff = light.outerCutoff;
    target->type = static_cast<LightType>(light.type);
    target->castShadows = light.castShadows;
    return true;
}

bool EditorScene::SetEditorCamera(Entity entity, const EditorCameraState &cameraState)
{
    Camera *target = m_world.GetComponent<Camera>(entity);
    if (!target)
    {
        return false;
    }

    if (cameraState.main)
    {
        for (Entity current : m_world.GetEntitiesWith<Camera>())
        {
            if (Camera *otherCamera = m_world.GetComponent<Camera>(current))
            {
                otherCamera->main = current == entity;
            }
        }
    }
    else
    {
        target->main = false;
    }

    PerspectiveProjection::Frustrum frustrum = target->GetProjection().GetFrustrum();
    frustrum.angle = cameraState.angle;
    frustrum.width = cameraState.width;
    frustrum.height = cameraState.height;
    frustrum.near = std::max(cameraState.nearPlane, 0.001f);
    frustrum.far = std::max(cameraState.farPlane, frustrum.near + 0.001f);

    target->GetProjection().SetLookAt(cameraState.lookAt);
    target->GetProjection().SetUpVector(cameraState.upVector);
    target->GetProjection().SetFrustrum(frustrum);
    return true;
}

bool EditorScene::SetEditorRigidbody(Entity entity, const EditorRigidbodyState &rigidbodyState)
{
    Rigidbody *target = m_world.GetComponent<Rigidbody>(entity);
    if (!target)
    {
        return false;
    }

    target->type = static_cast<RigidbodyType>(rigidbodyState.type);
    target->mass = std::max(rigidbodyState.mass, 0.001f);
    target->gravityScale = rigidbodyState.gravityScale;
    target->linearDamping = std::max(rigidbodyState.linearDamping, 0.0f);
    target->angularDamping = std::max(rigidbodyState.angularDamping, 0.0f);
    target->useGravity = rigidbodyState.useGravity;
    target->lockRotation = rigidbodyState.lockRotation;
    target->isTrigger = rigidbodyState.isTrigger;
    target->linearVelocity = rigidbodyState.linearVelocity;
    target->angularVelocity = rigidbodyState.angularVelocity;
    return true;
}

bool EditorScene::SetEditorBoxCollider(Entity entity, const EditorBoxColliderState &colliderState)
{
    BoxCollider *target = m_world.GetComponent<BoxCollider>(entity);
    if (!target)
    {
        return false;
    }

    target->center = colliderState.center;
    target->halfExtents = glm::max(colliderState.halfExtents, glm::vec3(0.001f));
    target->isTrigger = colliderState.isTrigger;
    target->material.friction = colliderState.material.friction;
    target->material.restitution = colliderState.material.restitution;
    return true;
}

bool EditorScene::SetEditorSphereCollider(Entity entity, const EditorSphereColliderState &colliderState)
{
    SphereCollider *target = m_world.GetComponent<SphereCollider>(entity);
    if (!target)
    {
        return false;
    }

    target->center = colliderState.center;
    target->radius = std::max(colliderState.radius, 0.001f);
    target->isTrigger = colliderState.isTrigger;
    target->material.friction = colliderState.material.friction;
    target->material.restitution = colliderState.material.restitution;
    return true;
}

bool EditorScene::SetEditorCapsuleCollider(Entity entity, const EditorCapsuleColliderState &colliderState)
{
    CapsuleCollider *target = m_world.GetComponent<CapsuleCollider>(entity);
    if (!target)
    {
        return false;
    }

    target->center = colliderState.center;
    target->radius = std::max(colliderState.radius, 0.001f);
    target->height = std::max(colliderState.height, target->radius * 2.0f);
    target->isTrigger = colliderState.isTrigger;
    target->material.friction = colliderState.material.friction;
    target->material.restitution = colliderState.material.restitution;
    return true;
}

void EditorScene::SetEditorViewportSize(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    if (width == m_viewportWidth && height == m_viewportHeight)
    {
        return;
    }

    ApplyViewportSize(width, height);
}

uint32_t EditorScene::GetEditorViewportTextureId() const
{
    return m_renderSystem ? m_renderSystem->GetViewportTextureId() : 0;
}

EditorViewportCameraState EditorScene::GetEditorViewportCameraState() const
{
    return m_editorCameraState;
}

bool EditorScene::OrbitEditorViewportCamera(float deltaX, float deltaY)
{
    if (!m_editorCameraState.valid)
    {
        return false;
    }

    constexpr float orbitSensitivity = 0.01f;
    constexpr float maxPitch = glm::radians(85.0f);
    m_editorCameraYaw += deltaX * orbitSensitivity;
    m_editorCameraPitch = std::clamp(m_editorCameraPitch + deltaY * orbitSensitivity, -maxPitch, maxPitch);
    RebuildEditorCameraState();
    return true;
}

bool EditorScene::PanEditorViewportCamera(float deltaX, float deltaY)
{
    if (!m_editorCameraState.valid || m_editorCameraFrustrum.height <= 0.0f)
    {
        return false;
    }

    const glm::vec3 forward = glm::normalize(m_editorCameraTarget - m_editorCameraState.position);
    const glm::vec3 right = glm::normalize(glm::cross(forward, m_editorCameraUpVector));
    const glm::vec3 up = glm::normalize(glm::cross(right, forward));
    const float halfFovRadians = glm::radians(m_editorCameraFrustrum.angle * 0.5f);
    const float worldUnitsPerPixel = (2.0f * std::tan(halfFovRadians) * m_editorCameraDistance) / m_editorCameraFrustrum.height;

    m_editorCameraTarget += (-right * deltaX + up * deltaY) * worldUnitsPerPixel;
    RebuildEditorCameraState();
    return true;
}

bool EditorScene::DollyEditorViewportCamera(float deltaY)
{
    if (!m_editorCameraState.valid)
    {
        return false;
    }

    m_editorCameraDistance = std::clamp(m_editorCameraDistance * std::exp(deltaY * 0.01f), 0.5f, 500.0f);
    RebuildEditorCameraState();
    return true;
}

Entity EditorScene::PickEditorEntityInViewport(float viewportX, float viewportY) const
{
    if (!m_editorCameraState.valid)
    {
        return {};
    }

    const Ray ray = BuildViewportRay(m_editorCameraState, viewportX, viewportY);

    Entity closestEntity;
    float closestDistance = std::numeric_limits<float>::max();
    for (Entity entity : m_world.GetEntitiesWith<Transform>())
    {
        const Transform *transform = m_world.GetComponent<Transform>(entity);
        if (!transform)
        {
            continue;
        }

        float hitDistance = 0.0f;
        bool hit = false;

        const MeshRenderer *renderer = m_world.GetComponent<MeshRenderer>(entity);
        if (renderer && m_resourceManager)
        {
            const resources::Model *model = m_resourceManager->Get(renderer->GetModelHandle());
            if (model && !model->IsEmpty())
            {
                hit = IntersectRayAabbLocal(ray, transform->GetMatrix(), model->GetBoundsMin(), model->GetBoundsMax(), hitDistance);
            }
        }

        if (!hit)
        {
            const float maxScale = std::max({std::abs(transform->scale.x), std::abs(transform->scale.y), std::abs(transform->scale.z), 1.0f});
            hit = IntersectRaySphere(ray, transform->position, 0.5f * maxScale, hitDistance);
        }

        if (hit && hitDistance < closestDistance)
        {
            closestDistance = hitDistance;
            closestEntity = entity;
        }
    }

    return closestEntity;
}

bool EditorScene::AddEditorComponent(Entity entity, std::string_view componentType)
{
    if (!m_world.IsEntityActive(entity))
    {
        return false;
    }

    if (componentType == "Transform")
    {
        if (m_world.HasComponent<Transform>(entity))
            return false;
        m_world.AddComponent<Transform>(entity);
        return true;
    }

    if (componentType == "Light")
    {
        if (m_world.HasComponent<Light>(entity))
            return false;
        m_world.AddComponent<Light>(entity);
        return true;
    }

    if (componentType == "Camera")
    {
        if (m_world.HasComponent<Camera>(entity))
            return false;
        const float width = m_viewportWidth > 0 ? static_cast<float>(m_viewportWidth) : 1280.0f;
        const float height = m_viewportHeight > 0 ? static_cast<float>(m_viewportHeight) : 720.0f;
        m_world.AddComponent<Camera>(entity, PerspectiveProjection::Frustrum{45.0f, width, height, 0.1f, 250.0f});
        return true;
    }

    if (componentType == "Animation")
    {
        if (m_world.HasComponent<AnimationPlayer>(entity))
            return false;
        m_world.AddComponent<AnimationPlayer>(entity);
        if (!m_world.HasComponent<SkeletonPose>(entity))
        {
            m_world.AddComponent<SkeletonPose>(entity);
        }
        return true;
    }

    if (componentType == "Rigidbody")
    {
        if (m_world.HasComponent<Rigidbody>(entity))
            return false;
        m_world.AddComponent<Rigidbody>(entity);
        return true;
    }

    if (componentType == "BoxCollider")
    {
        if (HasAnyCollider(m_world, entity))
            return false;
        m_world.AddComponent<BoxCollider>(entity);
        return true;
    }

    if (componentType == "SphereCollider")
    {
        if (HasAnyCollider(m_world, entity))
            return false;
        m_world.AddComponent<SphereCollider>(entity);
        return true;
    }

    if (componentType == "CapsuleCollider")
    {
        if (HasAnyCollider(m_world, entity))
            return false;
        m_world.AddComponent<CapsuleCollider>(entity);
        return true;
    }

    return false;
}

bool EditorScene::RemoveEditorComponent(Entity entity, std::string_view componentType)
{
    if (!m_world.IsEntityActive(entity))
    {
        return false;
    }

    if (componentType == "Transform")
    {
        return m_world.RemoveComponent<Transform>(entity);
    }

    if (componentType == "Light")
    {
        return m_world.RemoveComponent<Light>(entity);
    }

    if (componentType == "Camera")
    {
        return m_world.RemoveComponent<Camera>(entity);
    }

    if (componentType == "Animation")
    {
        const bool removedAnimation = m_world.RemoveComponent<AnimationPlayer>(entity);
        const bool removedPose = m_world.RemoveComponent<SkeletonPose>(entity);
        return removedAnimation || removedPose;
    }

    if (componentType == "Rigidbody")
    {
        return m_world.RemoveComponent<Rigidbody>(entity);
    }

    if (componentType == "BoxCollider")
    {
        return m_world.RemoveComponent<BoxCollider>(entity);
    }

    if (componentType == "SphereCollider")
    {
        return m_world.RemoveComponent<SphereCollider>(entity);
    }

    if (componentType == "CapsuleCollider")
    {
        return m_world.RemoveComponent<CapsuleCollider>(entity);
    }

    return false;
}

std::string EditorScene::ExportEditorSceneSnapshot() const
{
    return SerializeWorldToSceneJson(m_world, m_resourceManager, m_scenePath);
}

bool EditorScene::RestoreEditorSceneSnapshot(std::string_view snapshot)
{
    if (m_editorPlayMode)
    {
        return false;
    }

    return ReloadSceneFromSnapshot(snapshot);
}

Entity EditorScene::CreateEditorEntity(std::string_view name)
{
    if (m_editorPlayMode)
    {
        return {};
    }

    Entity entity = m_world.CreateEntity();
    m_world.AddComponent<Name>(entity, name.empty() ? "Entity" : std::string(name));
    m_world.AddComponent<Transform>(entity);
    return entity;
}

bool EditorScene::DeleteEditorEntity(Entity entity)
{
    if (m_editorPlayMode || !m_world.IsEntityActive(entity))
    {
        return false;
    }

    m_world.DestroyEntity(entity);
    return true;
}

Entity EditorScene::DuplicateEditorEntity(Entity entity)
{
    if (m_editorPlayMode || !m_world.IsEntityActive(entity))
    {
        return {};
    }

    Entity duplicate = m_world.CreateEntity();
    std::string duplicateName = "Entity " + std::to_string(duplicate.GetID());
    if (const Name *name = m_world.GetComponent<Name>(entity))
    {
        duplicateName = name->value + " Copy";
    }

    m_world.AddComponent<Name>(duplicate, duplicateName);
    CopySupportedEditorComponents(m_world, entity, duplicate);

    if (!m_world.HasComponent<Transform>(duplicate))
    {
        m_world.AddComponent<Transform>(duplicate);
    }

    return duplicate;
}

bool EditorScene::FrameEditorEntity(Entity entity)
{
    if (!m_world.IsEntityActive(entity))
    {
        return false;
    }

    const SceneBounds bounds = BuildEntityBounds(m_world, m_resourceManager, entity);
    if (!bounds.valid)
    {
        return false;
    }

    m_editorCameraTarget = (bounds.min + bounds.max) * 0.5f;
    m_editorCameraDistance = std::max(ComputeFitDistanceForBounds(bounds, m_editorCameraFrustrum), 1.0f);
    RebuildEditorCameraState();
    return true;
}

std::string EditorScene::GetEditorScenePath() const
{
    return m_scenePath;
}

std::vector<std::string> EditorScene::GetEditorSceneFiles() const
{
    std::vector<std::string> sceneFiles;
    const std::filesystem::path sceneRoot("assets/scenes");
    std::error_code errorCode;
    if (!std::filesystem::exists(sceneRoot, errorCode))
    {
        return sceneFiles;
    }

    for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(sceneRoot, errorCode))
    {
        if (errorCode || !entry.is_regular_file())
        {
            continue;
        }

        if (entry.path().extension() != ".json")
        {
            continue;
        }

        sceneFiles.push_back(entry.path().generic_string());
    }

    return sceneFiles;
}

bool EditorScene::SaveEditorScene()
{
    std::ofstream output(m_scenePath, std::ios::out | std::ios::trunc);
    if (!output.is_open())
    {
        ERROR("Failed to open scene for saving: " << m_scenePath);
        return false;
    }
    output << ExportEditorSceneSnapshot();
    INFO("Saved scene: " << m_scenePath);
    return true;
}

bool EditorScene::LoadEditorScene(std::string_view scenePath)
{
    m_scenePath = std::string(scenePath);
    ReloadScene(false);
    return true;
}

bool EditorScene::RequestAnimationTransition(Entity entity, int clipIndex, float durationSeconds)
{
    AnimationPlayer *animationPlayer = m_world.GetComponent<AnimationPlayer>(entity);
    MeshRenderer *meshRenderer = m_world.GetComponent<MeshRenderer>(entity);
    if (!animationPlayer || !meshRenderer || !m_resourceManager)
        return false;

    resources::Model *model = m_resourceManager->Get(meshRenderer->GetModelHandle());
    if (!model || !model->IsLoaded() || !model->HasAnimations())
        return false;

    if (clipIndex < 0 || clipIndex >= static_cast<int>(model->GetAnimationClipCount()))
        return false;

    const resources::AnimationClip *requestedClip = model->GetAnimationClip(static_cast<uint32_t>(clipIndex));
    if (!requestedClip)
        return false;

    const std::optional<uint32_t> currentClipIndex = animation::ResolveClipIndex(*model, animationPlayer->clipName, animationPlayer->clipIndex);
    if (durationSeconds <= 0.0f || (currentClipIndex.has_value() && currentClipIndex.value() == static_cast<uint32_t>(clipIndex)))
    {
        animationPlayer->clipName = requestedClip->name;
        animationPlayer->clipIndex = clipIndex;
        animationPlayer->currentTimeSeconds = 0.0;
        animationPlayer->targetClipName.clear();
        animationPlayer->targetClipIndex = -1;
        animationPlayer->targetTimeSeconds = 0.0;
        animationPlayer->transitionDurationSeconds = 0.0f;
        animationPlayer->transitionElapsedSeconds = 0.0f;
        animationPlayer->transitionActive = false;
        return true;
    }

    animationPlayer->targetClipName = requestedClip->name;
    animationPlayer->targetClipIndex = clipIndex;
    animationPlayer->targetTimeSeconds = 0.0;
    animationPlayer->transitionDurationSeconds = std::max(durationSeconds, 0.0f);
    animationPlayer->transitionElapsedSeconds = 0.0f;
    animationPlayer->transitionActive = true;
    return true;
}

void EditorScene::ReloadScene(bool clearResourceCache)
{
    if (clearResourceCache && m_resourceManager)
    {
        if (!m_resourceManager->ResetForCleanReload())
        {
            ERROR("Aborted clean reload: resource manager failed to reach idle state");
            return;
        }
        INFO("Reset resource manager before scene reload");
    }

    World reloadedWorld;
    if (!SceneLoader::LoadIntoWorld(m_scenePath, &reloadedWorld, m_resourceManager))
    {
        ERROR("Failed to hot reload scene: " << m_scenePath);
        return;
    }

    m_world = std::move(reloadedWorld);
    ConfigureSystems();
    m_pendingEditorCameraRefit = InitializeEditorCameraFromScene();

    if (m_viewportWidth > 0 && m_viewportHeight > 0)
    {
        OnResize(m_viewportWidth, m_viewportHeight);
    }

    INFO("Reloaded scene: " << m_scenePath);
}

bool EditorScene::ReloadSceneFromSnapshot(std::string_view snapshot)
{
    World reloadedWorld;
    if (!SceneLoader::LoadIntoWorldFromJson(snapshot, &reloadedWorld, m_resourceManager))
    {
        ERROR("Failed to restore scene snapshot: " << m_scenePath);
        return false;
    }

    m_world = std::move(reloadedWorld);
    ConfigureSystems();
    m_pendingEditorCameraRefit = InitializeEditorCameraFromScene();

    if (m_viewportWidth > 0 && m_viewportHeight > 0)
    {
        OnResize(m_viewportWidth, m_viewportHeight);
    }

    return true;
}

void EditorScene::ConfigureSystems()
{
    m_world.AddSystem<PhysicsSystem>();
    m_world.AddSystem<OrbitCameraControllerSystem>();
    m_world.AddSystem<LightOrbitControllerSystem>();
    m_world.AddSystem<AnimationSystem>(m_resourceManager);
    m_renderSystem = m_world.AddSystem<RenderSystem>(m_resourceManager);
    if (m_viewportWidth > 0 && m_viewportHeight > 0)
    {
        m_renderSystem->SetViewportSize(static_cast<uint32_t>(m_viewportWidth), static_cast<uint32_t>(m_viewportHeight));
    }
    SyncEditorCameraOverride();
}

bool EditorScene::InitializeEditorCameraFromScene()
{
    bool initializedFromScene = false;
    for (Entity entity : m_world.GetEntitiesWith<Camera>())
    {
        const Camera *camera = m_world.GetComponent<Camera>(entity);
        const Transform *transform = m_world.GetComponent<Transform>(entity);
        if (!camera || !transform || !camera->main)
        {
            continue;
        }

        const PerspectiveProjection &projection = camera->GetProjection();
        m_editorCameraTarget = projection.GetLookAt();
        m_editorCameraUpVector = projection.GetUpVector();
        m_editorCameraFrustrum = projection.GetFrustrum();
        if (m_viewportWidth > 0 && m_viewportHeight > 0)
        {
            m_editorCameraFrustrum.width = static_cast<float>(m_viewportWidth);
            m_editorCameraFrustrum.height = static_cast<float>(m_viewportHeight);
        }

        const glm::vec3 offset = transform->position - m_editorCameraTarget;
        const float distance = glm::length(offset);
        if (distance > 0.0001f)
        {
            m_editorCameraDistance = distance;
            m_editorCameraYaw = std::atan2(offset.z, offset.x);
            m_editorCameraPitch = std::asin(glm::clamp(offset.y / distance, -1.0f, 1.0f));
        }
        else
        {
            m_editorCameraDistance = 8.0f;
            m_editorCameraYaw = glm::radians(45.0f);
            m_editorCameraPitch = glm::radians(20.0f);
        }

        initializedFromScene = true;
        break;
    }

    if (!initializedFromScene)
    {
        m_editorCameraTarget = glm::vec3(0.0f);
        m_editorCameraUpVector = glm::vec3(0.0f, 1.0f, 0.0f);
        m_editorCameraYaw = glm::radians(45.0f);
        m_editorCameraPitch = glm::radians(20.0f);
        m_editorCameraDistance = 12.0f;
        if (m_viewportWidth > 0 && m_viewportHeight > 0)
        {
            m_editorCameraFrustrum.width = static_cast<float>(m_viewportWidth);
            m_editorCameraFrustrum.height = static_cast<float>(m_viewportHeight);
        }
    }

    const bool shouldAutoFit = !initializedFromScene;

    if (shouldAutoFit)
    {
        TryFitEditorCameraToSceneBounds();
    }

    RebuildEditorCameraState();
    return shouldAutoFit;
}

bool EditorScene::TryFitEditorCameraToSceneBounds()
{
    const SceneBounds sceneBounds = BuildSceneBounds(m_world, m_resourceManager);
    if (!sceneBounds.valid)
    {
        return false;
    }

    glm::vec3 preferredForward = glm::normalize(m_editorCameraTarget - m_editorCameraState.position);
    if (glm::length(preferredForward) <= 0.0001f)
    {
        preferredForward = glm::normalize(glm::vec3(-1.0f, -0.35f, -1.0f));
    }

    for (Entity entity : m_world.GetEntitiesWith<Camera>())
    {
        const Camera *camera = m_world.GetComponent<Camera>(entity);
        const Transform *transform = m_world.GetComponent<Transform>(entity);
        if (!camera || !transform || !camera->main)
        {
            continue;
        }

        const glm::vec3 viewDirection = camera->GetProjection().GetLookAt() - transform->position;
        if (glm::length(viewDirection) > 0.0001f)
        {
            preferredForward = glm::normalize(viewDirection);
        }
        break;
    }

    m_editorCameraTarget = (sceneBounds.min + sceneBounds.max) * 0.5f;
    m_editorCameraDistance = ComputeFitDistanceForBounds(sceneBounds, m_editorCameraFrustrum);

    const glm::vec3 position = m_editorCameraTarget - preferredForward * m_editorCameraDistance;
    const glm::vec3 offset = position - m_editorCameraTarget;
    const float distance = glm::length(offset);
    if (distance > 0.0001f)
    {
        m_editorCameraYaw = std::atan2(offset.z, offset.x);
        m_editorCameraPitch = std::asin(glm::clamp(offset.y / distance, -1.0f, 1.0f));
    }

    return sceneBounds.complete && sceneBounds.contributorCount > 0;
}

void EditorScene::RebuildEditorCameraState()
{
    const float horizontalRadius = m_editorCameraDistance * std::cos(m_editorCameraPitch);
    m_editorCameraState.position = m_editorCameraTarget + glm::vec3(
                                                              horizontalRadius * std::cos(m_editorCameraYaw),
                                                              m_editorCameraDistance * std::sin(m_editorCameraPitch),
                                                              horizontalRadius * std::sin(m_editorCameraYaw));

    PerspectiveProjection projection(m_editorCameraFrustrum, m_editorCameraTarget, m_editorCameraUpVector);
    m_editorCameraState.valid = true;
    m_editorCameraState.view = projection.BuildViewMatrix(m_editorCameraState.position);
    m_editorCameraState.projection = projection.GetProjectionMatrix();
    m_editorCameraState.viewProjection = m_editorCameraState.projection * m_editorCameraState.view;
    m_editorCameraState.invViewProjection = glm::inverse(m_editorCameraState.viewProjection);
    m_editorCameraState.viewportWidth = std::max(m_editorCameraFrustrum.width, 1.0f);
    m_editorCameraState.viewportHeight = std::max(m_editorCameraFrustrum.height, 1.0f);
    SyncEditorCameraOverride();
}

void EditorScene::SyncEditorCameraOverride()
{
    if (!m_renderSystem)
    {
        return;
    }

    if (m_runtimeMode)
    {
        m_renderSystem->SetCameraOverride(std::nullopt);
        return;
    }

    if (!m_editorCameraState.valid)
    {
        m_renderSystem->SetCameraOverride(std::nullopt);
        return;
    }

    CameraRenderData cameraData;
    cameraData.view = m_editorCameraState.view;
    cameraData.projection = m_editorCameraState.projection;
    cameraData.viewProjection = m_editorCameraState.viewProjection;
    cameraData.invViewProjection = m_editorCameraState.invViewProjection;
    cameraData.position = m_editorCameraState.position;
    cameraData.focusPoint = m_editorCameraTarget;
    cameraData.near = m_editorCameraFrustrum.near;
    cameraData.far = m_editorCameraFrustrum.far;
    m_renderSystem->SetCameraOverride(cameraData);
}

bool EditorScene::StartEditorPlayMode()
{
    if (m_runtimeMode)
    {
        return false;
    }

    if (m_editorPlayMode)
    {
        return false;
    }

    m_playModeSnapshot = ExportEditorSceneSnapshot();
    m_editorPlayMode = true;
    return true;
}

bool EditorScene::StopEditorPlayMode()
{
    if (m_runtimeMode)
    {
        return false;
    }

    if (!m_editorPlayMode)
    {
        return false;
    }

    m_editorPlayMode = false;
    const std::string snapshot = m_playModeSnapshot;
    m_playModeSnapshot.clear();
    if (snapshot.empty())
    {
        return true;
    }

    return ReloadSceneFromSnapshot(snapshot);
}

bool EditorScene::IsEditorPlayMode() const
{
    return m_runtimeMode || m_editorPlayMode;
}

void EditorScene::PresentToScreen()
{
    runtime_scene_support::PresentRenderSystem(m_renderSystem);
}