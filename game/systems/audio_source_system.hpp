#pragma once

#include <unordered_map>
#include <unordered_set>

#include <audio/audio_engine.hpp>
#include <ecs/components/camera.hpp>
#include <ecs/components/transform.hpp>
#include <ecs/world.hpp>
#include <resources/resource_manager.hpp>

#include "../components/audio_source.hpp"

class AudioSourceSystem : public System
{
public:
    explicit AudioSourceSystem(resources::ResourceManager *resourceManager = nullptr) : m_resourceManager(resourceManager) {}

    void Update(World &world, float deltaTime) override
    {
        (void)deltaTime;

        SyncListener(world);

        std::unordered_set<Entity::ID> activeEntities;
        for (Entity entity : world.GetEntitiesWith<AudioSource>())
        {
            activeEntities.insert(entity.GetID());
            AudioSource *source = world.GetComponent<AudioSource>(entity);
            if (!source)
            {
                continue;
            }

            SyncClip(*source);
            SyncPlayback(world, entity, *source);

            if (source->playbackHandle != 0)
            {
                m_entityPlaybacks[entity.GetID()] = source->playbackHandle;
            }
            else
            {
                m_entityPlaybacks.erase(entity.GetID());
            }
        }

        for (auto it = m_entityPlaybacks.begin(); it != m_entityPlaybacks.end();)
        {
            if (activeEntities.contains(it->first))
            {
                ++it;
                continue;
            }

            AudioEngine::Get().StopPlayback(it->second);
            it = m_entityPlaybacks.erase(it);
        }
    }

private:
    void SyncListener(World &world)
    {
        for (Entity entity : world.GetEntitiesWith<Camera, Transform>())
        {
            const Camera *camera = world.GetComponent<Camera>(entity);
            const Transform *transform = world.GetComponent<Transform>(entity);
            if (!camera || !transform || !camera->main)
            {
                continue;
            }

            glm::vec3 forward = camera->GetProjection().GetLookAt() - transform->position;
            if (glm::length(forward) < 0.0001f)
            {
                forward = glm::vec3(0.0f, 0.0f, -1.0f);
            }
            else
            {
                forward = glm::normalize(forward);
            }

            AudioEngine::Get().UpdateListener(transform->position, forward, camera->GetProjection().GetUpVector());
            return;
        }
    }

    void SyncClip(AudioSource &source)
    {
        if (source.clipPath.empty())
        {
            if (source.playbackHandle != 0)
            {
                AudioEngine::Get().StopPlayback(source.playbackHandle);
            }
            source.ResetRuntimeState();
            return;
        }

        if (source.loadedClipPath == source.clipPath && source.clipHandle.IsValid())
        {
            return;
        }

        if (source.playbackHandle != 0)
        {
            AudioEngine::Get().StopPlayback(source.playbackHandle);
        }

        source.ResetRuntimeState();
        if (!m_resourceManager)
        {
            return;
        }

        source.clipHandle = AudioEngine::Get().Preload(m_resourceManager, source.clipPath);
        if (source.clipHandle.IsValid())
        {
            source.loadedClipPath = source.clipPath;
        }
    }

    void SyncPlayback(World &world, Entity entity, AudioSource &source)
    {
        const bool active = source.playbackHandle != 0 && AudioEngine::Get().IsPlaybackActive(source.playbackHandle);
        if (source.playbackHandle != 0 && !active)
        {
            source.playbackHandle = 0;
        }

        if (!source.enabled || !source.clipHandle.IsValid())
        {
            if (source.playbackHandle != 0)
            {
                AudioEngine::Get().StopPlayback(source.playbackHandle);
                source.playbackHandle = 0;
            }
            return;
        }

        AudioSpatialParams spatial;
        const AudioSpatialParams *spatialPtr = nullptr;
        if (source.spatialized)
        {
            if (const Transform *transform = world.GetComponent<Transform>(entity))
            {
                spatial.position = transform->position;
                spatial.relativeToListener = source.relativeToListener;
                spatial.minDistance = source.minDistance;
                spatial.maxDistance = source.maxDistance;
                spatial.attenuation = source.attenuation;
                spatialPtr = &spatial;
            }
        }

        if (source.loop)
        {
            if (source.playOnAwake && source.playbackHandle == 0)
            {
                source.playbackHandle = AudioEngine::Get().PlayClip(m_resourceManager, source.clipHandle, true, source.volume, spatialPtr);
            }

            if (source.playbackHandle != 0)
            {
                AudioEngine::Get().SetPlaybackVolume(source.playbackHandle, source.volume);
                if (spatialPtr)
                {
                    AudioEngine::Get().SetPlaybackSpatial(source.playbackHandle, *spatialPtr);
                }
            }
            return;
        }

        if (source.playOnAwake && !source.startedOnce)
        {
            source.playbackHandle = AudioEngine::Get().PlayClip(m_resourceManager, source.clipHandle, false, source.volume, spatialPtr);
            source.startedOnce = source.playbackHandle != 0;
        }

        if (source.playbackHandle != 0)
        {
            AudioEngine::Get().SetPlaybackVolume(source.playbackHandle, source.volume);
            if (spatialPtr)
            {
                AudioEngine::Get().SetPlaybackSpatial(source.playbackHandle, *spatialPtr);
            }
        }
    }

    resources::ResourceManager *m_resourceManager = nullptr;
    std::unordered_map<Entity::ID, AudioEngine::PlaybackHandle> m_entityPlaybacks;
};
