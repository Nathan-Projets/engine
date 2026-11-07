#pragma once

#include <cstdint>
#include <string>

#include "../resource.hpp"

namespace resources
{
    enum class AudioLoadMode
    {
        Auto,
        Preload,
        Stream
    };

    enum class AudioPlaybackMode
    {
        OneShot,
        Streamed
    };

    enum class AudioBus
    {
        Sfx = 0,
        Music,
        Ui,
        Count
    };

    class AudioClip : public Resource
    {
    public:
        static constexpr uintmax_t AutoPreloadThresholdBytes = 512 * 1024;

        AudioClip(uint32_t id, const std::string &path = "") : Resource(id, path) {}

        void SetSourcePath(std::string sourcePath)
        {
            m_sourcePath = std::move(sourcePath);
        }

        const std::string &GetSourcePath() const noexcept
        {
            return m_sourcePath;
        }

        void SetByteSize(uintmax_t byteSize) noexcept
        {
            m_byteSize = byteSize;
        }

        uintmax_t GetByteSize() const noexcept
        {
            return m_byteSize;
        }

        void SetLoadMode(AudioLoadMode loadMode) noexcept
        {
            m_loadMode = loadMode;
        }

        AudioLoadMode GetLoadMode() const noexcept
        {
            return m_loadMode;
        }

        AudioLoadMode GetResolvedLoadMode() const noexcept
        {
            if (m_loadMode != AudioLoadMode::Auto)
            {
                return m_loadMode;
            }

            if (m_playbackMode == AudioPlaybackMode::Streamed)
            {
                return AudioLoadMode::Stream;
            }

            return m_byteSize <= AutoPreloadThresholdBytes ? AudioLoadMode::Preload : AudioLoadMode::Stream;
        }

        void SetPlaybackMode(AudioPlaybackMode playbackMode) noexcept
        {
            m_playbackMode = playbackMode;
        }

        AudioPlaybackMode GetPlaybackMode() const noexcept
        {
            return m_playbackMode;
        }

        void SetBus(AudioBus bus) noexcept
        {
            m_bus = bus;
        }

        AudioBus GetBus() const noexcept
        {
            return m_bus;
        }

    private:
        std::string m_sourcePath;
        uintmax_t m_byteSize = 0;
        AudioLoadMode m_loadMode = AudioLoadMode::Auto;
        AudioPlaybackMode m_playbackMode = AudioPlaybackMode::OneShot;
        AudioBus m_bus = AudioBus::Sfx;
    };
}