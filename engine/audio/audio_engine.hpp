#pragma once

#include <chrono>
#include <cstdint>

#include <glm/glm.hpp>

#include <string_view>

#include <resources/units/audio_clip.hpp>

namespace resources
{
    class ResourceManager;

    template <typename T>
    struct Handle;
}

struct AudioSpatialParams
{
    glm::vec3 position = glm::vec3(0.0f);
    bool relativeToListener = false;
    float minDistance = 1.0f;
    float maxDistance = 10000.0f;
    float attenuation = 1.0f;
};

class AudioEngine
{
public:
    using PlaybackHandle = uint64_t;

    static AudioEngine &Get();

    bool Init();
    void Shutdown();
    void Update();
    void UpdateListener(const glm::vec3 &position, const glm::vec3 &forward, const glm::vec3 &up = glm::vec3(0.0f, 1.0f, 0.0f));
    void StopAll();
    void PauseAll();
    void ResumeAll();
    void SetBusVolume(resources::AudioBus bus, float volume);
    float GetBusVolume(resources::AudioBus bus) const;

    resources::Handle<resources::AudioClip> Preload(resources::ResourceManager *resourceManager,
                                                    std::string_view path,
                                                    bool cacheBuffer = true,
                                                    std::chrono::milliseconds timeout = std::chrono::milliseconds(750));

    PlaybackHandle PlayClip(resources::ResourceManager *resourceManager,
                            resources::Handle<resources::AudioClip> clip,
                            bool loop = false,
                            float volume = 1.0f,
                            const AudioSpatialParams *spatial = nullptr);
    bool IsPlaybackActive(PlaybackHandle handle) const;
    void StopPlayback(PlaybackHandle handle);
    bool PausePlayback(PlaybackHandle handle);
    bool ResumePlayback(PlaybackHandle handle);
    bool SetPlaybackVolume(PlaybackHandle handle, float volume);
    bool SetPlaybackSpatial(PlaybackHandle handle, const AudioSpatialParams &spatial);

    bool PlayOneShot(std::string_view path, float volume = 1.0f);
    bool PlayOneShot(resources::ResourceManager *resourceManager, resources::Handle<resources::AudioClip> clip, float volume = 1.0f);
    bool PlayOneShotAt(resources::ResourceManager *resourceManager,
                       resources::Handle<resources::AudioClip> clip,
                       const AudioSpatialParams &spatial,
                       float volume = 1.0f);
    bool PlayMusic(std::string_view path, bool loop = true, float volume = 0.65f);
    bool PlayMusic(resources::ResourceManager *resourceManager,
                   resources::Handle<resources::AudioClip> clip,
                   bool loop = true,
                   float volume = 0.65f);
    void StopMusic();
    bool PauseMusic();
    bool ResumeMusic();

private:
    AudioEngine() = default;
};