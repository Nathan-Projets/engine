#include "audio_engine.hpp"

#include <SFML/Audio/Listener.hpp>
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#include <helpers/log.hpp>
#include <resources/resource_manager.hpp>
#include <resources/units/audio_clip.hpp>

namespace
{
    constexpr size_t kAudioBusCount = static_cast<size_t>(resources::AudioBus::Count);

    struct ActiveSoundInstance
    {
        AudioEngine::PlaybackHandle id = 0;
        std::unique_ptr<sf::Sound> sound;
        float baseVolume = 1.0f;
        resources::AudioBus bus = resources::AudioBus::Sfx;
    };

    struct ActiveStreamInstance
    {
        AudioEngine::PlaybackHandle id = 0;
        std::unique_ptr<sf::Music> stream;
        float baseVolume = 1.0f;
        resources::AudioBus bus = resources::AudioBus::Music;
    };

    struct MusicPlayback
    {
        std::unique_ptr<sf::Music> stream;
        float baseVolume = 0.65f;
        resources::AudioBus bus = resources::AudioBus::Music;
    };

    struct AudioEngineState
    {
        bool initialized = false;
        AudioEngine::PlaybackHandle nextPlaybackId = 1;
        MusicPlayback music;
        std::unordered_map<std::string, std::shared_ptr<sf::SoundBuffer>> soundBuffers;
        std::vector<ActiveSoundInstance> activeSounds;
        std::vector<ActiveStreamInstance> activeStreams;
        std::array<float, kAudioBusCount> busVolumes = {1.0f, 1.0f, 1.0f};
    };

    AudioEngineState &GetState()
    {
        static AudioEngineState state;
        return state;
    }

    std::filesystem::path ResolveAudioPath(std::string_view path)
    {
        const std::filesystem::path candidate(path);
        if (candidate.is_absolute())
        {
            return candidate;
        }

        const std::filesystem::path current = std::filesystem::current_path();
        const std::filesystem::path direct = current / candidate;
        if (std::filesystem::exists(direct))
        {
            return direct;
        }

        return candidate;
    }

    float ToSfmlVolume(float volume)
    {
        return std::clamp(volume, 0.0f, 1.0f) * 100.0f;
    }

    size_t ToBusIndex(resources::AudioBus bus)
    {
        return static_cast<size_t>(bus);
    }

    float ApplyBusGain(const AudioEngineState &state, float volume, resources::AudioBus bus)
    {
        return std::clamp(volume, 0.0f, 1.0f) * std::clamp(state.busVolumes[ToBusIndex(bus)], 0.0f, 1.0f);
    }

    sf::Vector3f ToSfVector(const glm::vec3 &value)
    {
        return {value.x, value.y, value.z};
    }

    template <typename TSound>
    void ApplySpatialSettings(TSound &sound, const AudioSpatialParams &spatial)
    {
        sound.setSpatializationEnabled(true);
        sound.setRelativeToListener(spatial.relativeToListener);
        sound.setPosition(ToSfVector(spatial.position));
        sound.setMinDistance(std::max(spatial.minDistance, 0.001f));
        sound.setMaxDistance(std::max(spatial.maxDistance, spatial.minDistance));
        sound.setAttenuation(std::max(spatial.attenuation, 0.0f));
    }

    template <typename TSound>
    void DisableSpatialization(TSound &sound)
    {
        sound.setSpatializationEnabled(false);
        sound.setRelativeToListener(false);
    }

    std::shared_ptr<sf::SoundBuffer> LoadSoundBuffer(AudioEngineState &state, const std::filesystem::path &path)
    {
        const std::string cacheKey = path.lexically_normal().generic_string();
        if (const auto it = state.soundBuffers.find(cacheKey); it != state.soundBuffers.end())
        {
            return it->second;
        }

        if (!std::filesystem::exists(path))
        {
            WARNING("Audio asset not found: " + path.string());
            return nullptr;
        }

        auto buffer = std::make_shared<sf::SoundBuffer>();
        if (!buffer->loadFromFile(path))
        {
            WARNING("Failed to load audio asset: " + path.string());
            return nullptr;
        }

        state.soundBuffers.emplace(cacheKey, buffer);
        return buffer;
    }

    bool WaitForAudioClip(resources::ResourceManager *resourceManager,
                          resources::Handle<resources::AudioClip> clip,
                          std::chrono::milliseconds timeout)
    {
        if (!resourceManager || !clip.IsValid())
        {
            return false;
        }

        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
            const resources::AudioClip *audioClip = resourceManager->Get(clip);
            if (audioClip && audioClip->IsLoaded())
            {
                return true;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        return false;
    }

    const resources::AudioClip *ResolveLoadedAudioClip(resources::ResourceManager *resourceManager, resources::Handle<resources::AudioClip> clip)
    {
        if (!resourceManager || !clip.IsValid())
        {
            return nullptr;
        }

        const resources::AudioClip *audioClip = resourceManager->Get(clip);
        if (!audioClip || !audioClip->IsLoaded())
        {
            return nullptr;
        }

        return audioClip;
    }

    ActiveSoundInstance *FindActiveSound(AudioEngineState &state, AudioEngine::PlaybackHandle handle)
    {
        for (ActiveSoundInstance &instance : state.activeSounds)
        {
            if (instance.id == handle)
            {
                return &instance;
            }
        }

        return nullptr;
    }

    ActiveStreamInstance *FindActiveStream(AudioEngineState &state, AudioEngine::PlaybackHandle handle)
    {
        for (ActiveStreamInstance &instance : state.activeStreams)
        {
            if (instance.id == handle)
            {
                return &instance;
            }
        }

        return nullptr;
    }

    const ActiveSoundInstance *FindActiveSound(const AudioEngineState &state, AudioEngine::PlaybackHandle handle)
    {
        for (const ActiveSoundInstance &instance : state.activeSounds)
        {
            if (instance.id == handle)
            {
                return &instance;
            }
        }

        return nullptr;
    }

    const ActiveStreamInstance *FindActiveStream(const AudioEngineState &state, AudioEngine::PlaybackHandle handle)
    {
        for (const ActiveStreamInstance &instance : state.activeStreams)
        {
            if (instance.id == handle)
            {
                return &instance;
            }
        }

        return nullptr;
    }

    void ApplyActiveSoundVolume(const AudioEngineState &state, ActiveSoundInstance &instance)
    {
        if (!instance.sound)
        {
            return;
        }

        instance.sound->setVolume(ToSfmlVolume(ApplyBusGain(state, instance.baseVolume, instance.bus)));
    }

    void ApplyActiveStreamVolume(const AudioEngineState &state, ActiveStreamInstance &instance)
    {
        if (!instance.stream)
        {
            return;
        }

        instance.stream->setVolume(ToSfmlVolume(ApplyBusGain(state, instance.baseVolume, instance.bus)));
    }

    void ApplyMusicVolume(const AudioEngineState &state)
    {
        if (!state.music.stream)
        {
            return;
        }

        state.music.stream->setVolume(ToSfmlVolume(ApplyBusGain(state, state.music.baseVolume, state.music.bus)));
    }
}

AudioEngine &AudioEngine::Get()
{
    static AudioEngine instance;
    return instance;
}

bool AudioEngine::Init()
{
    AudioEngineState &state = GetState();
    if (state.initialized)
    {
        return true;
    }

    state.initialized = true;
    return true;
}

void AudioEngine::Shutdown()
{
    AudioEngineState &state = GetState();
    StopAll();
    state.soundBuffers.clear();
    state.initialized = false;
}

void AudioEngine::Update()
{
    AudioEngineState &state = GetState();
    if (!state.initialized)
    {
        return;
    }

    state.activeSounds.erase(
        std::remove_if(
            state.activeSounds.begin(),
            state.activeSounds.end(),
            [](const ActiveSoundInstance &instance)
            {
                return !instance.sound || instance.sound->getStatus() == sf::SoundSource::Status::Stopped;
            }),
        state.activeSounds.end());

    state.activeStreams.erase(
        std::remove_if(
            state.activeStreams.begin(),
            state.activeStreams.end(),
            [](const ActiveStreamInstance &instance)
            {
                return !instance.stream || instance.stream->getStatus() == sf::SoundSource::Status::Stopped;
            }),
        state.activeStreams.end());

    if (state.music.stream && state.music.stream->getStatus() == sf::SoundSource::Status::Stopped && !state.music.stream->isLooping())
    {
        state.music.stream.reset();
    }
}

void AudioEngine::UpdateListener(const glm::vec3 &position, const glm::vec3 &forward, const glm::vec3 &up)
{
    if (!Init())
    {
        return;
    }

    sf::Listener::setPosition(ToSfVector(position));
    sf::Listener::setDirection(ToSfVector(forward));
    sf::Listener::setUpVector(ToSfVector(up));
}

void AudioEngine::StopAll()
{
    AudioEngineState &state = GetState();
    for (ActiveSoundInstance &instance : state.activeSounds)
    {
        if (instance.sound)
        {
            instance.sound->stop();
        }
    }
    state.activeSounds.clear();

    for (ActiveStreamInstance &instance : state.activeStreams)
    {
        if (instance.stream)
        {
            instance.stream->stop();
        }
    }
    state.activeStreams.clear();
    StopMusic();
}

void AudioEngine::PauseAll()
{
    AudioEngineState &state = GetState();
    for (ActiveSoundInstance &instance : state.activeSounds)
    {
        if (instance.sound && instance.sound->getStatus() == sf::SoundSource::Status::Playing)
        {
            instance.sound->pause();
        }
    }

    for (ActiveStreamInstance &instance : state.activeStreams)
    {
        if (instance.stream && instance.stream->getStatus() == sf::SoundSource::Status::Playing)
        {
            instance.stream->pause();
        }
    }

    if (state.music.stream && state.music.stream->getStatus() == sf::SoundSource::Status::Playing)
    {
        state.music.stream->pause();
    }
}

void AudioEngine::ResumeAll()
{
    AudioEngineState &state = GetState();
    for (ActiveSoundInstance &instance : state.activeSounds)
    {
        if (instance.sound && instance.sound->getStatus() == sf::SoundSource::Status::Paused)
        {
            instance.sound->play();
        }
    }

    for (ActiveStreamInstance &instance : state.activeStreams)
    {
        if (instance.stream && instance.stream->getStatus() == sf::SoundSource::Status::Paused)
        {
            instance.stream->play();
        }
    }

    if (state.music.stream && state.music.stream->getStatus() == sf::SoundSource::Status::Paused)
    {
        state.music.stream->play();
    }
}

void AudioEngine::SetBusVolume(resources::AudioBus bus, float volume)
{
    AudioEngineState &state = GetState();
    if (!Init())
    {
        return;
    }

    state.busVolumes[ToBusIndex(bus)] = std::clamp(volume, 0.0f, 1.0f);
    for (ActiveSoundInstance &instance : state.activeSounds)
    {
        ApplyActiveSoundVolume(state, instance);
    }
    for (ActiveStreamInstance &instance : state.activeStreams)
    {
        ApplyActiveStreamVolume(state, instance);
    }
    ApplyMusicVolume(state);
}

float AudioEngine::GetBusVolume(resources::AudioBus bus) const
{
    const AudioEngineState &state = GetState();
    return state.busVolumes[ToBusIndex(bus)];
}

resources::Handle<resources::AudioClip> AudioEngine::Preload(resources::ResourceManager *resourceManager,
                                                             std::string_view path,
                                                             bool cacheBuffer,
                                                             std::chrono::milliseconds timeout)
{
    AudioEngineState &state = GetState();
    if (!Init())
    {
        return {};
    }

    if (!resourceManager)
    {
        return {};
    }

    resources::Handle<resources::AudioClip> clip = resourceManager->Load<resources::AudioClip>(std::string(path));
    if (!WaitForAudioClip(resourceManager, clip, timeout))
    {
        return {};
    }

    const resources::AudioClip *audioClip = ResolveLoadedAudioClip(resourceManager, clip);
    if (!audioClip)
    {
        return {};
    }

    if (!cacheBuffer || audioClip->GetResolvedLoadMode() != resources::AudioLoadMode::Preload)
    {
        return clip;
    }

    if (!LoadSoundBuffer(state, std::filesystem::path(audioClip->GetSourcePath())))
    {
        return {};
    }

    return clip;
}

AudioEngine::PlaybackHandle AudioEngine::PlayClip(resources::ResourceManager *resourceManager,
                                                  resources::Handle<resources::AudioClip> clip,
                                                  bool loop,
                                                  float volume,
                                                  const AudioSpatialParams *spatial)
{
    AudioEngineState &state = GetState();
    if (!Init())
    {
        return 0;
    }

    const resources::AudioClip *audioClip = ResolveLoadedAudioClip(resourceManager, clip);
    if (!audioClip)
    {
        return 0;
    }

    const PlaybackHandle handle = state.nextPlaybackId++;
    if (audioClip->GetPlaybackMode() == resources::AudioPlaybackMode::Streamed)
    {
        auto stream = std::make_unique<sf::Music>();
        if (!stream->openFromFile(std::filesystem::path(audioClip->GetSourcePath())))
        {
            WARNING("Failed to open streamed audio clip: " + audioClip->GetSourcePath());
            return 0;
        }

        stream->setLooping(loop);
        if (spatial)
        {
            ApplySpatialSettings(*stream, *spatial);
        }
        else
        {
            DisableSpatialization(*stream);
        }

        ActiveStreamInstance instance;
        instance.id = handle;
        instance.baseVolume = volume;
        instance.bus = audioClip->GetBus();
        instance.stream = std::move(stream);
        ApplyActiveStreamVolume(state, instance);
        instance.stream->play();
        state.activeStreams.push_back(std::move(instance));
        return handle;
    }

    std::shared_ptr<sf::SoundBuffer> buffer = LoadSoundBuffer(state, std::filesystem::path(audioClip->GetSourcePath()));
    if (!buffer)
    {
        return 0;
    }

    ActiveSoundInstance instance;
    instance.id = handle;
    instance.baseVolume = volume;
    instance.bus = audioClip->GetBus();
    instance.sound = std::make_unique<sf::Sound>(*buffer);
    instance.sound->setLooping(loop);
    if (spatial)
    {
        ApplySpatialSettings(*instance.sound, *spatial);
    }
    else
    {
        DisableSpatialization(*instance.sound);
    }
    ApplyActiveSoundVolume(state, instance);
    instance.sound->play();
    state.activeSounds.push_back(std::move(instance));
    return handle;
}

bool AudioEngine::IsPlaybackActive(PlaybackHandle handle) const
{
    if (handle == 0)
    {
        return false;
    }

    const AudioEngineState &state = GetState();
    if (const ActiveSoundInstance *sound = FindActiveSound(state, handle))
    {
        return sound->sound && sound->sound->getStatus() != sf::SoundSource::Status::Stopped;
    }

    if (const ActiveStreamInstance *stream = FindActiveStream(state, handle))
    {
        return stream->stream && stream->stream->getStatus() != sf::SoundSource::Status::Stopped;
    }

    return false;
}

void AudioEngine::StopPlayback(PlaybackHandle handle)
{
    if (handle == 0)
    {
        return;
    }

    AudioEngineState &state = GetState();
    state.activeSounds.erase(
        std::remove_if(
            state.activeSounds.begin(),
            state.activeSounds.end(),
            [handle](ActiveSoundInstance &instance)
            {
                if (instance.id != handle)
                {
                    return false;
                }

                if (instance.sound)
                {
                    instance.sound->stop();
                }
                return true;
            }),
        state.activeSounds.end());

    state.activeStreams.erase(
        std::remove_if(
            state.activeStreams.begin(),
            state.activeStreams.end(),
            [handle](ActiveStreamInstance &instance)
            {
                if (instance.id != handle)
                {
                    return false;
                }

                if (instance.stream)
                {
                    instance.stream->stop();
                }
                return true;
            }),
        state.activeStreams.end());
}

bool AudioEngine::PausePlayback(PlaybackHandle handle)
{
    AudioEngineState &state = GetState();
    if (ActiveSoundInstance *sound = FindActiveSound(state, handle))
    {
        if (sound->sound)
        {
            sound->sound->pause();
            return true;
        }
    }

    if (ActiveStreamInstance *stream = FindActiveStream(state, handle))
    {
        if (stream->stream)
        {
            stream->stream->pause();
            return true;
        }
    }

    return false;
}

bool AudioEngine::ResumePlayback(PlaybackHandle handle)
{
    AudioEngineState &state = GetState();
    if (ActiveSoundInstance *sound = FindActiveSound(state, handle))
    {
        if (sound->sound)
        {
            sound->sound->play();
            return true;
        }
    }

    if (ActiveStreamInstance *stream = FindActiveStream(state, handle))
    {
        if (stream->stream)
        {
            stream->stream->play();
            return true;
        }
    }

    return false;
}

bool AudioEngine::SetPlaybackVolume(PlaybackHandle handle, float volume)
{
    AudioEngineState &state = GetState();
    if (ActiveSoundInstance *sound = FindActiveSound(state, handle))
    {
        sound->baseVolume = volume;
        ApplyActiveSoundVolume(state, *sound);
        return true;
    }

    if (ActiveStreamInstance *stream = FindActiveStream(state, handle))
    {
        stream->baseVolume = volume;
        ApplyActiveStreamVolume(state, *stream);
        return true;
    }

    return false;
}

bool AudioEngine::SetPlaybackSpatial(PlaybackHandle handle, const AudioSpatialParams &spatial)
{
    AudioEngineState &state = GetState();
    if (ActiveSoundInstance *sound = FindActiveSound(state, handle))
    {
        if (sound->sound)
        {
            ApplySpatialSettings(*sound->sound, spatial);
            return true;
        }
    }

    if (ActiveStreamInstance *stream = FindActiveStream(state, handle))
    {
        if (stream->stream)
        {
            ApplySpatialSettings(*stream->stream, spatial);
            return true;
        }
    }

    return false;
}

bool AudioEngine::PlayOneShot(std::string_view path, float volume)
{
    AudioEngineState &state = GetState();
    if (!Init())
    {
        return false;
    }

    std::shared_ptr<sf::SoundBuffer> buffer = LoadSoundBuffer(state, ResolveAudioPath(path));
    if (!buffer)
    {
        return false;
    }

    ActiveSoundInstance instance;
    instance.id = state.nextPlaybackId++;
    instance.baseVolume = volume;
    instance.bus = resources::AudioBus::Sfx;
    instance.sound = std::make_unique<sf::Sound>(*buffer);
    DisableSpatialization(*instance.sound);
    ApplyActiveSoundVolume(state, instance);
    instance.sound->play();
    state.activeSounds.push_back(std::move(instance));
    return true;
}

bool AudioEngine::PlayOneShot(resources::ResourceManager *resourceManager, resources::Handle<resources::AudioClip> clip, float volume)
{
    return PlayClip(resourceManager, clip, false, volume, nullptr) != 0;
}

bool AudioEngine::PlayOneShotAt(resources::ResourceManager *resourceManager,
                                resources::Handle<resources::AudioClip> clip,
                                const AudioSpatialParams &spatial,
                                float volume)
{
    return PlayClip(resourceManager, clip, false, volume, &spatial) != 0;
}

bool AudioEngine::PlayMusic(std::string_view path, bool loop, float volume)
{
    AudioEngineState &state = GetState();
    if (!Init())
    {
        return false;
    }

    StopMusic();

    const std::filesystem::path resolvedPath = ResolveAudioPath(path);
    if (!std::filesystem::exists(resolvedPath))
    {
        WARNING("Audio asset not found: " + resolvedPath.string());
        return false;
    }

    auto music = std::make_unique<sf::Music>();
    if (!music->openFromFile(resolvedPath))
    {
        WARNING("Failed to open music asset: " + resolvedPath.string());
        return false;
    }

    music->setLooping(loop);
    DisableSpatialization(*music);
    state.music.baseVolume = volume;
    state.music.bus = resources::AudioBus::Music;
    state.music.stream = std::move(music);
    ApplyMusicVolume(state);
    state.music.stream->play();
    return true;
}

bool AudioEngine::PlayMusic(resources::ResourceManager *resourceManager,
                            resources::Handle<resources::AudioClip> clip,
                            bool loop,
                            float volume)
{
    const resources::AudioClip *audioClip = ResolveLoadedAudioClip(resourceManager, clip);
    if (!audioClip)
    {
        return false;
    }

    return PlayMusic(audioClip->GetSourcePath(), loop, volume);
}

void AudioEngine::StopMusic()
{
    AudioEngineState &state = GetState();
    if (state.music.stream)
    {
        state.music.stream->stop();
        state.music.stream.reset();
    }
}

bool AudioEngine::PauseMusic()
{
    AudioEngineState &state = GetState();
    if (!state.music.stream || state.music.stream->getStatus() != sf::SoundSource::Status::Playing)
    {
        return false;
    }

    state.music.stream->pause();
    return true;
}

bool AudioEngine::ResumeMusic()
{
    AudioEngineState &state = GetState();
    if (!state.music.stream || state.music.stream->getStatus() != sf::SoundSource::Status::Paused)
    {
        return false;
    }

    state.music.stream->play();
    return true;
}
