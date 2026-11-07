#pragma once

#include <string>

#include <audio/audio_engine.hpp>
#include <ecs/component.hpp>
#include <resources/handle.hpp>
#include <resources/units/audio_clip.hpp>

class AudioSource : public Component
{
public:
    void ResetRuntimeState()
    {
        clipHandle = {};
        playbackHandle = 0;
        loadedClipPath.clear();
        startedOnce = false;
    }

    std::string clipPath;
    resources::Handle<resources::AudioClip> clipHandle;
    AudioEngine::PlaybackHandle playbackHandle = 0;
    std::string loadedClipPath;
    float volume = 1.0f;
    bool loop = false;
    bool playOnAwake = true;
    bool enabled = true;
    bool spatialized = true;
    bool relativeToListener = false;
    float minDistance = 1.0f;
    float maxDistance = 20.0f;
    float attenuation = 1.0f;
    bool startedOnce = false;
};
