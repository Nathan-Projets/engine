#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../components/animation_player.hpp"
#include "../components/skeleton_pose.hpp"
#include "../../resources/units/model.hpp"

namespace animation
{
    struct NodeTransform
    {
        glm::vec3 translation{0.0f, 0.0f, 0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f, 1.0f, 1.0f};
    };

    inline glm::mat4 ComposeTransform(const NodeTransform &transform)
    {
        glm::mat4 matrix = glm::translate(glm::mat4(1.0f), transform.translation);
        matrix *= glm::mat4_cast(glm::normalize(transform.rotation));
        matrix = glm::scale(matrix, transform.scale);
        return matrix;
    }

    inline glm::vec3 SamplePosition(const resources::AnimationChannel &channel,
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

    inline glm::quat SampleRotation(const resources::AnimationChannel &channel,
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

    inline glm::vec3 SampleScale(const resources::AnimationChannel &channel,
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

    inline std::optional<uint32_t> ResolveClipIndex(const resources::Model &model,
                                                    const std::string &clipName,
                                                    int clipIndex)
    {
        if (!clipName.empty())
        {
            if (std::optional<uint32_t> resolvedIndex = model.FindAnimationClipIndex(clipName))
                return resolvedIndex;

            const std::string suffix = "|" + clipName;
            const auto &clips = model.GetAnimationClips();
            for (uint32_t currentClipIndex = 0; currentClipIndex < static_cast<uint32_t>(clips.size()); ++currentClipIndex)
            {
                if (clips[currentClipIndex].name == clipName)
                    return currentClipIndex;

                if (clips[currentClipIndex].name.size() > suffix.size() &&
                    clips[currentClipIndex].name.ends_with(suffix))
                {
                    return currentClipIndex;
                }
            }
        }

        if (clipIndex >= 0 && clipIndex < static_cast<int>(model.GetAnimationClipCount()))
            return static_cast<uint32_t>(clipIndex);

        if (model.GetAnimationClipCount() == 0)
            return std::nullopt;

        return 0u;
    }

    inline std::optional<uint32_t> ResolveAndCacheClipIndex(const resources::Model &model, AnimationPlayer &animationPlayer)
    {
        const std::optional<uint32_t> clipIndex = ResolveClipIndex(model, animationPlayer.clipName, animationPlayer.clipIndex);
        if (!clipIndex.has_value())
            return std::nullopt;

        animationPlayer.clipIndex = static_cast<int>(clipIndex.value());
        const resources::AnimationClip *clip = model.GetAnimationClip(clipIndex.value());
        if (clip)
            animationPlayer.clipName = clip->name;

        return clipIndex;
    }

    inline std::optional<uint32_t> ResolveAndCacheTargetClipIndex(const resources::Model &model, AnimationPlayer &animationPlayer)
    {
        const std::optional<uint32_t> clipIndex = ResolveClipIndex(model, animationPlayer.targetClipName, animationPlayer.targetClipIndex);
        if (!clipIndex.has_value())
            return std::nullopt;

        animationPlayer.targetClipIndex = static_cast<int>(clipIndex.value());
        const resources::AnimationClip *clip = model.GetAnimationClip(clipIndex.value());
        if (clip)
            animationPlayer.targetClipName = clip->name;

        return clipIndex;
    }

    inline double GetTicksPerSecond(const resources::AnimationClip &clip)
    {
        return clip.ticksPerSecond > 0.0 ? clip.ticksPerSecond : 25.0;
    }

    inline double GetDurationSeconds(const resources::AnimationClip &clip)
    {
        const double ticksPerSecond = GetTicksPerSecond(clip);
        return clip.durationTicks > 0.0 ? (clip.durationTicks / ticksPerSecond) : 0.0;
    }

    inline double SecondsToTicks(const resources::AnimationClip &clip, double timeSeconds)
    {
        return timeSeconds * GetTicksPerSecond(clip);
    }

    inline void AdvanceClipTime(double &timeSeconds,
                                const resources::AnimationClip &clip,
                                float speed,
                                bool loop,
                                bool paused,
                                float deltaTime)
    {
        const double durationSeconds = GetDurationSeconds(clip);
        if (paused || durationSeconds <= 0.0)
            return;

        timeSeconds += static_cast<double>(deltaTime) * static_cast<double>(speed);
        if (loop)
        {
            timeSeconds = std::fmod(timeSeconds, durationSeconds);
            if (timeSeconds < 0.0)
                timeSeconds += durationSeconds;
            return;
        }

        timeSeconds = std::clamp(timeSeconds, 0.0, durationSeconds);
    }

    inline void BuildLocalPose(const resources::Model &model,
                               const resources::AnimationClip &clip,
                               double animationTimeTicks,
                               std::vector<NodeTransform> &localPose)
    {
        const std::vector<resources::SkeletonNode> &nodes = model.GetSkeletonNodes();
        localPose.resize(nodes.size());

        for (size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex)
        {
            const resources::SkeletonNode &node = nodes[nodeIndex];
            localPose[nodeIndex].translation = node.localTranslation;
            localPose[nodeIndex].rotation = node.localRotation;
            localPose[nodeIndex].scale = node.localScale;
        }

        for (const resources::AnimationChannel &channel : clip.channels)
        {
            if (channel.nodeIndex >= nodes.size())
                continue;

            const resources::SkeletonNode &node = nodes[channel.nodeIndex];
            localPose[channel.nodeIndex].translation = SamplePosition(channel, animationTimeTicks, node);
            localPose[channel.nodeIndex].rotation = SampleRotation(channel, animationTimeTicks, node);
            localPose[channel.nodeIndex].scale = SampleScale(channel, animationTimeTicks, node);
        }
    }

    inline void BlendLocalPoses(const std::vector<NodeTransform> &sourcePose,
                                const std::vector<NodeTransform> &targetPose,
                                float alpha,
                                std::vector<NodeTransform> &blendedPose)
    {
        blendedPose.resize(sourcePose.size());
        for (size_t nodeIndex = 0; nodeIndex < sourcePose.size(); ++nodeIndex)
        {
            blendedPose[nodeIndex].translation = glm::mix(sourcePose[nodeIndex].translation, targetPose[nodeIndex].translation, alpha);
            blendedPose[nodeIndex].rotation = glm::normalize(glm::slerp(sourcePose[nodeIndex].rotation, targetPose[nodeIndex].rotation, alpha));
            blendedPose[nodeIndex].scale = glm::mix(sourcePose[nodeIndex].scale, targetPose[nodeIndex].scale, alpha);
        }
    }

    inline void EvaluateNodeRecursive(uint32_t nodeIndex,
                                      const std::vector<resources::SkeletonNode> &nodes,
                                      const std::vector<NodeTransform> &localPose,
                                      std::vector<glm::mat4> &globalTransforms)
    {
        const resources::SkeletonNode &node = nodes[nodeIndex];
        const glm::mat4 localTransform = ComposeTransform(localPose[nodeIndex]);
        if (node.parentIndex != std::numeric_limits<uint32_t>::max())
        {
            globalTransforms[nodeIndex] = globalTransforms[node.parentIndex] * localTransform;
        }
        else
        {
            globalTransforms[nodeIndex] = localTransform;
        }

        for (uint32_t childIndex : node.children)
        {
            EvaluateNodeRecursive(childIndex, nodes, localPose, globalTransforms);
        }
    }

    inline void BuildSkeletonPose(const resources::Model &model,
                                  const std::vector<NodeTransform> &localPose,
                                  SkeletonPose &skeletonPose)
    {
        const std::vector<resources::SkeletonNode> &nodes = model.GetSkeletonNodes();
        const std::vector<resources::BoneInfo> &bones = model.GetBones();
        const std::vector<resources::MeshPrimitiveInstance> &primitiveInstances = model.GetPrimitiveInstances();

        skeletonPose.nodeGlobalTransforms.assign(nodes.size(), glm::mat4(1.0f));
        if (!nodes.empty())
        {
            const uint32_t rootNodeIndex = model.GetRootNodeIndex() < nodes.size() ? model.GetRootNodeIndex() : 0u;
            EvaluateNodeRecursive(rootNodeIndex, nodes, localPose, skeletonPose.nodeGlobalTransforms);
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
}