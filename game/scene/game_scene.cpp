#include "game_scene.hpp"

#include <algorithm>

#include "../../engine/helpers/log.hpp"
#include "../../engine/input/input_manager.hpp"
#include "../../engine/ecs/animation/animation_runtime.hpp"
#include "../../engine/ecs/components/animation_player.hpp"
#include "../../engine/ecs/components/camera.hpp"
#include "../../engine/ecs/components/mesh_renderer.hpp"
#include "../../engine/ecs/components/skeleton_pose.hpp"
#include "../../engine/ecs/systems/animation_system.hpp"
#include "../../engine/ecs/systems/physics_system.hpp"
#include "../../engine/ecs/systems/render_system.hpp"
#include "../../engine/resources/loaders/scene_loader.hpp"
#include "../systems/light_orbit_controller_system.hpp"
#include "../systems/orbit_camera_controller_system.hpp"

GameScene::GameScene(std::string scenePath, resources::ResourceManager *resourceManager) : m_resourceManager(resourceManager), m_scenePath(std::move(scenePath))
{
}

void GameScene::Init()
{
    ReloadScene();
}

void GameScene::Update(float deltaTime)
{
    if (InputManager::Get().IsActionJustPressed("reload_scene_clean"))
    {
        ReloadScene(true);
    }

    if (InputManager::Get().IsActionJustPressed("reload_scene"))
    {
        ReloadScene(false);
    }

    m_world.UpdateSystems(deltaTime);
}

void GameScene::Draw(float deltaTime)
{
    m_world.RenderSystems(deltaTime);
}

void GameScene::OnResize(int width, int height)
{
    m_viewportWidth = width;
    m_viewportHeight = height;

    if (width <= 0 || height <= 0)
    {
        return;
    }

    for (Entity entity : m_world.GetEntitiesWith<Camera>())
    {
        Camera *cam = m_world.GetComponent<Camera>(entity);
        if (cam && cam->main)
        {
            cam->GetProjection().SetViewportSize(static_cast<float>(width), static_cast<float>(height));
            break;
        }
    }
}

AnimationDebugSnapshot GameScene::GetAnimationDebugSnapshot() const
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

bool GameScene::RequestAnimationTransition(Entity entity, int clipIndex, float durationSeconds)
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

void GameScene::ReloadScene(bool clearResourceCache)
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

    if (m_viewportWidth > 0 && m_viewportHeight > 0)
    {
        OnResize(m_viewportWidth, m_viewportHeight);
    }

    INFO("Reloaded scene: " << m_scenePath);
}

void GameScene::ConfigureSystems()
{
    m_world.AddSystem<PhysicsSystem>();
    m_world.AddSystem<OrbitCameraControllerSystem>();
    m_world.AddSystem<LightOrbitControllerSystem>();
    m_world.AddSystem<AnimationSystem>(m_resourceManager);
    m_world.AddSystem<RenderSystem>(m_resourceManager);
}