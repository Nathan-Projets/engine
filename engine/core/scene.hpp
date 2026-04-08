#pragma once

#include <string_view>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "../ecs/entity.hpp"

namespace resources
{
    class ResourceManager;
}

struct AnimationDebugEntry
{
    Entity entity;
    std::string modelPath;
    std::string activeClipName;
    int activeClipIndex = -1;
    std::string targetClipName;
    int targetClipIndex = -1;
    double currentTimeSeconds = 0.0;
    double targetTimeSeconds = 0.0;
    double clipDurationSeconds = 0.0;
    double targetClipDurationSeconds = 0.0;
    double ticksPerSecond = 0.0;
    size_t boneCount = 0;
    size_t channelCount = 0;
    bool loop = false;
    bool paused = false;
    bool poseValid = false;
    bool transitionActive = false;
    float transitionAlpha = 0.0f;
    float transitionDurationSeconds = 0.0f;
    std::vector<std::string> availableClips;
};

struct AnimationDebugSnapshot
{
    std::vector<AnimationDebugEntry> entries;
};

struct EditorEntitySummary
{
    Entity entity;
    std::string name;
};

struct EditorTransformState
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
};

struct EditorLightState
{
    glm::vec3 ambient = glm::vec3(0.2f);
    glm::vec3 diffuse = glm::vec3(0.5f);
    glm::vec3 specular = glm::vec3(0.7f);
    glm::vec3 color = glm::vec3(1.0f);
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
    float intensity = 1.0f;
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    float innerCutoff = 12.5f;
    float outerCutoff = 17.5f;
    int type = 0;
    bool castShadows = true;
};

struct EditorCameraState
{
    glm::vec3 lookAt = glm::vec3(0.0f);
    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
    float angle = 45.0f;
    float width = 1.0f;
    float height = 1.0f;
    float nearPlane = 0.1f;
    float farPlane = 150.0f;
    bool main = false;
};

struct EditorRigidbodyState
{
    int type = 1;
    float mass = 1.0f;
    float gravityScale = 1.0f;
    float linearDamping = 0.05f;
    float angularDamping = 0.8f;
    bool useGravity = true;
    bool lockRotation = false;
    bool isTrigger = false;
    glm::vec3 linearVelocity = glm::vec3(0.0f);
    glm::vec3 angularVelocity = glm::vec3(0.0f);
};

struct EditorColliderMaterialState
{
    float friction = 0.5f;
    float restitution = 0.0f;
};

struct EditorBoxColliderState
{
    glm::vec3 center = glm::vec3(0.0f);
    glm::vec3 halfExtents = glm::vec3(0.5f);
    bool isTrigger = false;
    EditorColliderMaterialState material;
};

struct EditorSphereColliderState
{
    glm::vec3 center = glm::vec3(0.0f);
    float radius = 0.5f;
    bool isTrigger = false;
    EditorColliderMaterialState material;
};

struct EditorCapsuleColliderState
{
    glm::vec3 center = glm::vec3(0.0f);
    float radius = 0.5f;
    float height = 1.0f;
    bool isTrigger = false;
    EditorColliderMaterialState material;
};

struct EditorViewportCameraState
{
    bool valid = false;
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 viewProjection = glm::mat4(1.0f);
    glm::mat4 invViewProjection = glm::mat4(1.0f);
    glm::vec3 position = glm::vec3(0.0f);
    float viewportWidth = 1.0f;
    float viewportHeight = 1.0f;
};

struct EditorMeshRendererState
{
    std::string modelPath;
    std::string shaderPath;
    std::string materialPath;
    int queue = 0;
    bool loadTextures = true;
    bool usesResourcePipeline = false;
};

struct EditorAnimationState
{
    std::string clipName;
    int clipIndex = -1;
    float speed = 1.0f;
    bool loop = true;
    bool paused = false;
    bool transitionActive = false;
};

struct EditorEntityInspectorState
{
    bool valid = false;
    Entity entity;
    std::string name;
    bool hasTransform = false;
    bool hasLight = false;
    bool hasCamera = false;
    bool hasRigidbody = false;
    bool hasMeshRenderer = false;
    bool hasAnimation = false;
    bool hasPhysicsListener = false;
    bool hasBoxCollider = false;
    bool hasSphereCollider = false;
    bool hasCapsuleCollider = false;
    EditorTransformState transform;
    EditorLightState light;
    EditorCameraState camera;
    EditorRigidbodyState rigidbody;
    EditorMeshRendererState meshRenderer;
    EditorAnimationState animation;
    EditorBoxColliderState boxCollider;
    EditorSphereColliderState sphereCollider;
    EditorCapsuleColliderState capsuleCollider;
};

struct RuntimeUIState
{
    bool valid = false;
    bool paused = false;
    bool objectiveCompleted = false;
    std::string title;
    std::string objective;
    std::string status;
    std::string hint;
};

class Scene
{
public:
    virtual ~Scene() = default;

    virtual void Init() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Draw(float deltaTime) = 0;
    virtual void OnResize(int width, int height) {}
    virtual resources::ResourceManager *GetResourceManager() { return nullptr; }
    virtual AnimationDebugSnapshot GetAnimationDebugSnapshot() const { return {}; }
    virtual std::vector<EditorEntitySummary> GetEditorEntitySummaries() const { return {}; }
    virtual EditorEntityInspectorState GetEditorEntityInspectorState(Entity entity) const
    {
        (void)entity;
        return {};
    }
    virtual bool SetEditorEntityName(Entity entity, std::string_view name)
    {
        (void)entity;
        (void)name;
        return false;
    }
    virtual bool SetEditorTransform(Entity entity, const EditorTransformState &transform)
    {
        (void)entity;
        (void)transform;
        return false;
    }
    virtual bool SetEditorLight(Entity entity, const EditorLightState &light)
    {
        (void)entity;
        (void)light;
        return false;
    }
    virtual bool SetEditorCamera(Entity entity, const EditorCameraState &camera)
    {
        (void)entity;
        (void)camera;
        return false;
    }
    virtual bool SetEditorRigidbody(Entity entity, const EditorRigidbodyState &rigidbody)
    {
        (void)entity;
        (void)rigidbody;
        return false;
    }
    virtual bool SetEditorBoxCollider(Entity entity, const EditorBoxColliderState &collider)
    {
        (void)entity;
        (void)collider;
        return false;
    }
    virtual bool SetEditorSphereCollider(Entity entity, const EditorSphereColliderState &collider)
    {
        (void)entity;
        (void)collider;
        return false;
    }
    virtual bool SetEditorCapsuleCollider(Entity entity, const EditorCapsuleColliderState &collider)
    {
        (void)entity;
        (void)collider;
        return false;
    }
    virtual void SetEditorViewportSize(int width, int height)
    {
        (void)width;
        (void)height;
    }
    virtual uint32_t GetEditorViewportTextureId() const
    {
        return 0;
    }
    virtual EditorViewportCameraState GetEditorViewportCameraState() const
    {
        return {};
    }
    virtual bool OrbitEditorViewportCamera(float deltaX, float deltaY)
    {
        (void)deltaX;
        (void)deltaY;
        return false;
    }
    virtual bool PanEditorViewportCamera(float deltaX, float deltaY)
    {
        (void)deltaX;
        (void)deltaY;
        return false;
    }
    virtual bool DollyEditorViewportCamera(float deltaY)
    {
        (void)deltaY;
        return false;
    }
    virtual Entity PickEditorEntityInViewport(float viewportX, float viewportY) const
    {
        (void)viewportX;
        (void)viewportY;
        return {};
    }
    virtual bool AddEditorComponent(Entity entity, std::string_view componentType)
    {
        (void)entity;
        (void)componentType;
        return false;
    }
    virtual bool RemoveEditorComponent(Entity entity, std::string_view componentType)
    {
        (void)entity;
        (void)componentType;
        return false;
    }
    virtual std::string ExportEditorSceneSnapshot() const { return {}; }
    virtual bool RestoreEditorSceneSnapshot(std::string_view snapshot)
    {
        (void)snapshot;
        return false;
    }
    virtual Entity CreateEditorEntity(std::string_view name)
    {
        (void)name;
        return {};
    }
    virtual bool DeleteEditorEntity(Entity entity)
    {
        (void)entity;
        return false;
    }
    virtual Entity DuplicateEditorEntity(Entity entity)
    {
        (void)entity;
        return {};
    }
    virtual bool FrameEditorEntity(Entity entity)
    {
        (void)entity;
        return false;
    }
    virtual std::string GetEditorScenePath() const { return {}; }
    virtual std::vector<std::string> GetEditorSceneFiles() const { return {}; }
    virtual bool SaveEditorScene() { return false; }
    virtual bool LoadEditorScene(std::string_view scenePath)
    {
        (void)scenePath;
        return false;
    }
    virtual bool RequestAnimationTransition(Entity entity, int clipIndex, float durationSeconds)
    {
        (void)entity;
        (void)clipIndex;
        (void)durationSeconds;
        return false;
    }
    virtual bool StartEditorPlayMode() { return false; }
    virtual bool StopEditorPlayMode() { return false; }
    virtual bool IsEditorPlayMode() const { return false; }
    virtual RuntimeUIState GetRuntimeUIState() const { return {}; }
    virtual bool SetGamePaused(bool paused)
    {
        (void)paused;
        return false;
    }
    virtual bool IsGamePaused() const { return false; }
    virtual bool ResetRuntimeState() { return false; }
    virtual void PresentToScreen() {}

    void Render(float deltaTime)
    {
        Update(deltaTime);
        Draw(deltaTime);
    }
};