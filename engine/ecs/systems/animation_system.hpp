#pragma once

#include <algorithm>
#include <optional>
#include <vector>

#include "../animation/animation_runtime.hpp"
#include "../components/animation_player.hpp"
#include "../components/mesh_renderer.hpp"
#include "../components/skeleton_pose.hpp"
#include "../system.hpp"
#include "../world.hpp"
#include "../../resources/resource_manager.hpp"

class AnimationSystem : public System
{
public:
    explicit AnimationSystem(resources::ResourceManager *resourceManager = nullptr) : m_resourceManager(resourceManager) {}

    void Update(World &world, float deltaTime) override
    {
        if (!m_resourceManager)
            return;

        for (Entity entity : world.GetEntitiesWith<MeshRenderer, AnimationPlayer, SkeletonPose>())
        {
            MeshRenderer *meshRenderer = world.GetComponent<MeshRenderer>(entity);
            AnimationPlayer *animationPlayer = world.GetComponent<AnimationPlayer>(entity);
            SkeletonPose *skeletonPose = world.GetComponent<SkeletonPose>(entity);
            if (!meshRenderer || !animationPlayer || !skeletonPose)
                continue;

            resources::Model *model = m_resourceManager->Get(meshRenderer->GetModelHandle());
            if (!model || !model->IsLoaded() || !model->HasSkeleton() || !model->HasAnimations())
            {
                skeletonPose->valid = false;
                ClearTransitionState(*animationPlayer);
                continue;
            }

            const std::optional<uint32_t> clipIndex = animation::ResolveAndCacheClipIndex(*model, *animationPlayer);
            if (!clipIndex.has_value())
            {
                skeletonPose->valid = false;
                ClearTransitionState(*animationPlayer);
                continue;
            }

            const resources::AnimationClip *clip = model->GetAnimationClip(clipIndex.value());
            if (!clip)
            {
                skeletonPose->valid = false;
                ClearTransitionState(*animationPlayer);
                continue;
            }

            if (!animationPlayer->transitionActive)
            {
                animation::AdvanceClipTime(animationPlayer->currentTimeSeconds,
                                           *clip,
                                           animationPlayer->speed,
                                           animationPlayer->loop,
                                           animationPlayer->paused,
                                           deltaTime);
                EvaluateSingleClip(*model, *clip, animationPlayer->currentTimeSeconds, *skeletonPose);
                continue;
            }

            const std::optional<uint32_t> targetClipIndex = animation::ResolveAndCacheTargetClipIndex(*model, *animationPlayer);
            if (!targetClipIndex.has_value())
            {
                ClearTransitionState(*animationPlayer);
                animation::AdvanceClipTime(animationPlayer->currentTimeSeconds,
                                           *clip,
                                           animationPlayer->speed,
                                           animationPlayer->loop,
                                           animationPlayer->paused,
                                           deltaTime);
                EvaluateSingleClip(*model, *clip, animationPlayer->currentTimeSeconds, *skeletonPose);
                continue;
            }

            const resources::AnimationClip *targetClip = model->GetAnimationClip(targetClipIndex.value());
            if (!targetClip || targetClipIndex.value() == clipIndex.value())
            {
                if (targetClip)
                {
                    animationPlayer->clipName = targetClip->name;
                    animationPlayer->clipIndex = static_cast<int>(targetClipIndex.value());
                    animationPlayer->currentTimeSeconds = animationPlayer->targetTimeSeconds;
                }

                ClearTransitionState(*animationPlayer);
                animation::AdvanceClipTime(animationPlayer->currentTimeSeconds,
                                           *clip,
                                           animationPlayer->speed,
                                           animationPlayer->loop,
                                           animationPlayer->paused,
                                           deltaTime);
                EvaluateSingleClip(*model, *clip, animationPlayer->currentTimeSeconds, *skeletonPose);
                continue;
            }

            animation::AdvanceClipTime(animationPlayer->currentTimeSeconds,
                                       *clip,
                                       animationPlayer->speed,
                                       animationPlayer->loop,
                                       animationPlayer->paused,
                                       deltaTime);
            animation::AdvanceClipTime(animationPlayer->targetTimeSeconds,
                                       *targetClip,
                                       animationPlayer->speed,
                                       animationPlayer->loop,
                                       animationPlayer->paused,
                                       deltaTime);

            if (!animationPlayer->paused)
            {
                animationPlayer->transitionElapsedSeconds += deltaTime;
            }

            const float blendAlpha = animationPlayer->transitionDurationSeconds > 0.0f
                                         ? std::clamp(animationPlayer->transitionElapsedSeconds / animationPlayer->transitionDurationSeconds, 0.0f, 1.0f)
                                         : 1.0f;

            EvaluateTransition(*model,
                               *clip,
                               animationPlayer->currentTimeSeconds,
                               *targetClip,
                               animationPlayer->targetTimeSeconds,
                               blendAlpha,
                               *skeletonPose);

            if (blendAlpha >= 1.0f)
            {
                PromoteTransitionTarget(*animationPlayer);
            }
        }
    }

private:
    static void ClearTransitionState(AnimationPlayer &animationPlayer)
    {
        animationPlayer.targetClipName.clear();
        animationPlayer.targetClipIndex = -1;
        animationPlayer.targetTimeSeconds = 0.0;
        animationPlayer.transitionDurationSeconds = 0.0f;
        animationPlayer.transitionElapsedSeconds = 0.0f;
        animationPlayer.transitionActive = false;
    }

    static void PromoteTransitionTarget(AnimationPlayer &animationPlayer)
    {
        animationPlayer.clipName = animationPlayer.targetClipName;
        animationPlayer.clipIndex = animationPlayer.targetClipIndex;
        animationPlayer.currentTimeSeconds = animationPlayer.targetTimeSeconds;
        ClearTransitionState(animationPlayer);
    }

    static void EvaluateSingleClip(const resources::Model &model,
                                   const resources::AnimationClip &clip,
                                   double timeSeconds,
                                   SkeletonPose &skeletonPose)
    {
        std::vector<animation::NodeTransform> localPose;
        animation::BuildLocalPose(model, clip, animation::SecondsToTicks(clip, timeSeconds), localPose);
        animation::BuildSkeletonPose(model, localPose, skeletonPose);
    }

    static void EvaluateTransition(const resources::Model &model,
                                   const resources::AnimationClip &sourceClip,
                                   double sourceTimeSeconds,
                                   const resources::AnimationClip &targetClip,
                                   double targetTimeSeconds,
                                   float blendAlpha,
                                   SkeletonPose &skeletonPose)
    {
        std::vector<animation::NodeTransform> sourcePose;
        std::vector<animation::NodeTransform> targetPose;
        std::vector<animation::NodeTransform> blendedPose;
        animation::BuildLocalPose(model, sourceClip, animation::SecondsToTicks(sourceClip, sourceTimeSeconds), sourcePose);
        animation::BuildLocalPose(model, targetClip, animation::SecondsToTicks(targetClip, targetTimeSeconds), targetPose);
        animation::BlendLocalPoses(sourcePose, targetPose, blendAlpha, blendedPose);
        animation::BuildSkeletonPose(model, blendedPose, skeletonPose);
    }

private:
    resources::ResourceManager *m_resourceManager = nullptr;
};