#pragma once

#include <string>
#include <string_view>

#include <core/scene.hpp>
#include <ecs/world.hpp>
#include <render/camera/perspective_projection.hpp>
#include <resources/resource_manager.hpp>

class RenderSystem;

class EditorScene : public Scene
{
public:
    EditorScene(std::string scenePath, resources::ResourceManager *resourceManager = nullptr, bool runtimeMode = false);
    ~EditorScene() override = default;

    void Init() override;
    void Update(float deltaTime) override;
    void Draw(float deltaTime) override;
    void OnResize(int width, int height) override;
    resources::ResourceManager *GetResourceManager() override { return m_resourceManager; }
    AnimationDebugSnapshot GetAnimationDebugSnapshot() const override;
    std::vector<EditorEntitySummary> GetEditorEntitySummaries() const override;
    EditorEntityInspectorState GetEditorEntityInspectorState(Entity entity) const override;
    bool SetEditorEntityName(Entity entity, std::string_view name) override;
    bool SetEditorTransform(Entity entity, const EditorTransformState &transform) override;
    bool SetEditorLight(Entity entity, const EditorLightState &light) override;
    bool SetEditorCamera(Entity entity, const EditorCameraState &camera) override;
    bool SetEditorRigidbody(Entity entity, const EditorRigidbodyState &rigidbody) override;
    bool SetEditorBoxCollider(Entity entity, const EditorBoxColliderState &collider) override;
    bool SetEditorSphereCollider(Entity entity, const EditorSphereColliderState &collider) override;
    bool SetEditorCapsuleCollider(Entity entity, const EditorCapsuleColliderState &collider) override;
    void SetEditorViewportSize(int width, int height) override;
    uint32_t GetEditorViewportTextureId() const override;
    EditorViewportCameraState GetEditorViewportCameraState() const override;
    bool OrbitEditorViewportCamera(float deltaX, float deltaY) override;
    bool PanEditorViewportCamera(float deltaX, float deltaY) override;
    bool DollyEditorViewportCamera(float deltaY) override;
    Entity PickEditorEntityInViewport(float viewportX, float viewportY) const override;
    bool AddEditorComponent(Entity entity, std::string_view componentType) override;
    bool RemoveEditorComponent(Entity entity, std::string_view componentType) override;
    std::string ExportEditorSceneSnapshot() const override;
    bool RestoreEditorSceneSnapshot(std::string_view snapshot) override;
    Entity CreateEditorEntity(std::string_view name) override;
    bool DeleteEditorEntity(Entity entity) override;
    Entity DuplicateEditorEntity(Entity entity) override;
    bool FrameEditorEntity(Entity entity) override;
    std::string GetEditorScenePath() const override;
    std::vector<std::string> GetEditorSceneFiles() const override;
    bool SaveEditorScene() override;
    bool LoadEditorScene(std::string_view scenePath) override;
    bool RequestAnimationTransition(Entity entity, int clipIndex, float durationSeconds) override;
    bool StartEditorPlayMode() override;
    bool StopEditorPlayMode() override;
    bool IsEditorPlayMode() const override;
    void PresentToScreen() override;

private:
    void ReloadScene(bool clearResourceCache = false);
    bool ReloadSceneFromSnapshot(std::string_view snapshot);
    void ConfigureSystems();
    void ApplyViewportSize(int width, int height);
    bool InitializeEditorCameraFromScene();
    bool TryFitEditorCameraToSceneBounds();
    void RebuildEditorCameraState();
    void SyncEditorCameraOverride();

private:
    World m_world;
    resources::ResourceManager *m_resourceManager;
    RenderSystem *m_renderSystem = nullptr;
    std::string m_scenePath;
    int m_viewportWidth = 0;
    int m_viewportHeight = 0;
    EditorViewportCameraState m_editorCameraState;
    glm::vec3 m_editorCameraTarget = glm::vec3(0.0f);
    glm::vec3 m_editorCameraUpVector = glm::vec3(0.0f, 1.0f, 0.0f);
    float m_editorCameraYaw = 0.0f;
    float m_editorCameraPitch = 0.0f;
    float m_editorCameraDistance = 8.0f;
    PerspectiveProjection::Frustrum m_editorCameraFrustrum{45.0f, 1280.0f, 720.0f, 0.1f, 250.0f};
    bool m_pendingEditorCameraRefit = false;
    bool m_editorPlayMode = false;
    bool m_runtimeMode = false;
    std::string m_playModeSnapshot;
};