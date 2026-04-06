#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../system.hpp"
#include "../world.hpp"
#include "../components/animation_player.hpp"
#include "../components/mesh_renderer.hpp"
#include "../components/skeleton_pose.hpp"
#include "../../helpers/log.hpp"
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
                continue;
            }

            const std::optional<uint32_t> clipIndex = ResolveClipIndex(*model, *animationPlayer);
            if (!clipIndex.has_value())
            {
                skeletonPose->valid = false;
                continue;
            }

            const resources::AnimationClip *clip = model->GetAnimationClip(clipIndex.value());
            if (!clip)
            {
                skeletonPose->valid = false;
                continue;
            }

            const double ticksPerSecond = clip->ticksPerSecond > 0.0 ? clip->ticksPerSecond : 25.0;
            const double durationSeconds = clip->durationTicks > 0.0 ? (clip->durationTicks / ticksPerSecond) : 0.0;

            if (!animationPlayer->paused && durationSeconds > 0.0)
            {
                animationPlayer->currentTimeSeconds += static_cast<double>(deltaTime) * static_cast<double>(animationPlayer->speed);

                if (animationPlayer->loop)
                {
                    animationPlayer->currentTimeSeconds = std::fmod(animationPlayer->currentTimeSeconds, durationSeconds);
                    if (animationPlayer->currentTimeSeconds < 0.0)
                        animationPlayer->currentTimeSeconds += durationSeconds;
                }
                else
                {
                    animationPlayer->currentTimeSeconds = std::clamp(animationPlayer->currentTimeSeconds, 0.0, durationSeconds);
                }
            }

            const double animationTimeTicks = durationSeconds > 0.0
                                                  ? animationPlayer->currentTimeSeconds * ticksPerSecond
                                                  : 0.0;

            EvaluatePose(*model, *clip, animationTimeTicks, *skeletonPose);
        }
    }

private:
    static glm::mat4 ComposeTransform(const glm::vec3 &translation, const glm::quat &rotation, const glm::vec3 &scale)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation);
        transform *= glm::mat4_cast(glm::normalize(rotation));
        transform = glm::scale(transform, scale);
        return transform;
    }

    static glm::vec3 SamplePosition(const resources::AnimationChannel &channel,
                                    double timeTicks,
                                    const resources::SkeletonNode &node)
    {
        if (channel.positionKeys.empty())
            return node.localTranslation;
        if (channel.positionKeys.size() == 1 || timeTicks <= channel.positionKeys.front().timeTicks)
            return channel.positionKeys.front().value;
        if (timeTicks >= channel.positionKeys.back().timeTicks)
            return channel.positionKeys.back().value;

        for (size_t keyIndex = 0; keyIndex + 1 < channel.positionKeys.size(); ++keyIndex)
        {
            const resources::PositionKey &current = channel.positionKeys[keyIndex];
            const resources::PositionKey &next = channel.positionKeys[keyIndex + 1];
            if (timeTicks <= next.timeTicks)
            {
                const double span = std::max(next.timeTicks - current.timeTicks, 1e-8);
                const float alpha = static_cast<float>((timeTicks - current.timeTicks) / span);
                return glm::mix(current.value, next.value, alpha);
            }
        }

        return channel.positionKeys.back().value;
    }

    static glm::quat SampleRotation(const resources::AnimationChannel &channel,
                                    double timeTicks,
                                    const resources::SkeletonNode &node)
    {
        if (channel.rotationKeys.empty())
            return node.localRotation;
        if (channel.rotationKeys.size() == 1 || timeTicks <= channel.rotationKeys.front().timeTicks)
            return glm::normalize(channel.rotationKeys.front().value);
        if (timeTicks >= channel.rotationKeys.back().timeTicks)
            return glm::normalize(channel.rotationKeys.back().value);

        for (size_t keyIndex = 0; keyIndex + 1 < channel.rotationKeys.size(); ++keyIndex)
        {
            const resources::RotationKey &current = channel.rotationKeys[keyIndex];
            const resources::RotationKey &next = channel.rotationKeys[keyIndex + 1];
            if (timeTicks <= next.timeTicks)
            {
                const double span = std::max(next.timeTicks - current.timeTicks, 1e-8);
                const float alpha = static_cast<float>((timeTicks - current.timeTicks) / span);
                return glm::normalize(glm::slerp(current.value, next.value, alpha));
            }
        }

        return glm::normalize(channel.rotationKeys.back().value);
    }

    static glm::vec3 SampleScale(const resources::AnimationChannel &channel,
                                 double timeTicks,
                                 const resources::SkeletonNode &node)
    {
        if (channel.scaleKeys.empty())
            return node.localScale;
        if (channel.scaleKeys.size() == 1 || timeTicks <= channel.scaleKeys.front().timeTicks)
            return channel.scaleKeys.front().value;
        if (timeTicks >= channel.scaleKeys.back().timeTicks)
            return channel.scaleKeys.back().value;

        for (size_t keyIndex = 0; keyIndex + 1 < channel.scaleKeys.size(); ++keyIndex)
        {
            const resources::ScaleKey &current = channel.scaleKeys[keyIndex];
            const resources::ScaleKey &next = channel.scaleKeys[keyIndex + 1];
            if (timeTicks <= next.timeTicks)
            {
                const double span = std::max(next.timeTicks - current.timeTicks, 1e-8);
                const float alpha = static_cast<float>((timeTicks - current.timeTicks) / span);
                return glm::mix(current.value, next.value, alpha);
            }
        }

        return channel.scaleKeys.back().value;
    }

    static void EvaluateNodeRecursive(uint32_t nodeIndex,
                                      const std::vector<resources::SkeletonNode> &nodes,
                                      const std::vector<glm::mat4> &localTransforms,
                                      std::vector<glm::mat4> &globalTransforms)
    {
        const resources::SkeletonNode &node = nodes[nodeIndex];
        if (node.parentIndex != std::numeric_limits<uint32_t>::max())
        {
            globalTransforms[nodeIndex] = globalTransforms[node.parentIndex] * localTransforms[nodeIndex];
        }
        else
        {
            globalTransforms[nodeIndex] = localTransforms[nodeIndex];
        }

        for (uint32_t childIndex : node.children)
        {
            EvaluateNodeRecursive(childIndex, nodes, localTransforms, globalTransforms);
        }
    }

    static std::optional<uint32_t> ResolveClipIndex(const resources::Model &model, AnimationPlayer &animationPlayer)
    {
        if (!animationPlayer.clipName.empty())
        {
            if (std::optional<uint32_t> clipIndex = model.FindAnimationClipIndex(animationPlayer.clipName))
            {
                animationPlayer.clipIndex = static_cast<int>(clipIndex.value());
                return clipIndex;
            }

            const std::string suffix = "|" + animationPlayer.clipName;
            const auto &clips = model.GetAnimationClips();
            for (uint32_t clipIndex = 0; clipIndex < static_cast<uint32_t>(clips.size()); ++clipIndex)
            {
                if (clips[clipIndex].name == animationPlayer.clipName)
                {
                    animationPlayer.clipIndex = static_cast<int>(clipIndex);
                    return clipIndex;
                }

                if (clips[clipIndex].name.size() > suffix.size() &&
                    clips[clipIndex].name.ends_with(suffix))
                {
                    animationPlayer.clipIndex = static_cast<int>(clipIndex);
                    return clipIndex;
                }
            }
        }

        if (animationPlayer.clipIndex >= 0 && animationPlayer.clipIndex < static_cast<int>(model.GetAnimationClipCount()))
        {
            return static_cast<uint32_t>(animationPlayer.clipIndex);
        }

        if (model.GetAnimationClipCount() == 0)
            return std::nullopt;

        animationPlayer.clipIndex = 0;
        return 0u;
    }

    static void EvaluatePose(const resources::Model &model,
                             const resources::AnimationClip &clip,
                             double animationTimeTicks,
                             SkeletonPose &skeletonPose)
    {
        const std::vector<resources::SkeletonNode> &nodes = model.GetSkeletonNodes();
        const std::vector<resources::BoneInfo> &bones = model.GetBones();
        const std::vector<resources::MeshPrimitiveInstance> &primitiveInstances = model.GetPrimitiveInstances();

        skeletonPose.nodeGlobalTransforms.assign(nodes.size(), glm::mat4(1.0f));
        std::vector<glm::mat4> localTransforms(nodes.size(), glm::mat4(1.0f));

        for (uint32_t nodeIndex = 0; nodeIndex < static_cast<uint32_t>(nodes.size()); ++nodeIndex)
        {
            const resources::SkeletonNode &node = nodes[nodeIndex];
            localTransforms[nodeIndex] = ComposeTransform(node.localTranslation, node.localRotation, node.localScale);
        }

        for (const resources::AnimationChannel &channel : clip.channels)
        {
            if (channel.nodeIndex >= nodes.size())
                continue;

            const resources::SkeletonNode &node = nodes[channel.nodeIndex];
            localTransforms[channel.nodeIndex] = ComposeTransform(
                SamplePosition(channel, animationTimeTicks, node),
                SampleRotation(channel, animationTimeTicks, node),
                SampleScale(channel, animationTimeTicks, node));
        }

        if (!nodes.empty())
        {
            const uint32_t rootNodeIndex = model.GetRootNodeIndex() < nodes.size() ? model.GetRootNodeIndex() : 0u;
            EvaluateNodeRecursive(rootNodeIndex, nodes, localTransforms, skeletonPose.nodeGlobalTransforms);
        }

        skeletonPose.primitiveBonePalettes.clear();
        skeletonPose.primitiveBonePalettes.resize(primitiveInstances.size());

        for (size_t instanceIndex = 0; instanceIndex < primitiveInstances.size(); ++instanceIndex)
        {
            const resources::MeshPrimitiveInstance &primitiveInstance = primitiveInstances[instanceIndex];
            std::vector<glm::mat4> &palette = skeletonPose.primitiveBonePalettes[instanceIndex];

            if (!primitiveInstance.usesSkinning || primitiveInstance.nodeIndex >= nodes.size())
            {
                palette.clear();
                continue;
            }

            palette.resize(bones.size(), glm::mat4(1.0f));
            for (size_t boneIndex = 0; boneIndex < bones.size(); ++boneIndex)
            {
                const resources::BoneInfo &bone = bones[boneIndex];
                if (bone.nodeIndex >= skeletonPose.nodeGlobalTransforms.size())
                {
                    palette[boneIndex] = glm::mat4(1.0f);
                    continue;
                }

                palette[boneIndex] = skeletonPose.nodeGlobalTransforms[bone.nodeIndex] * bone.inverseBindMatrix;
            }
        }

        skeletonPose.valid = true;
    }

private:
    resources::ResourceManager *m_resourceManager = nullptr;
};