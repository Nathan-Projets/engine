#pragma once

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include "../resource_loader.hpp"
#include "../units/audio_clip.hpp"

namespace resources
{
    namespace detail
    {
        inline bool AudioPathContains(std::string_view haystack, std::string_view needle)
        {
            return haystack.find(needle) != std::string_view::npos;
        }

        inline std::string NormalizeAudioKey(std::string_view value)
        {
            std::string normalized(value);
            for (char &character : normalized)
            {
                if (character == '\\')
                {
                    character = '/';
                    continue;
                }

                character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
            }
            return normalized;
        }

        inline AudioBus InferAudioBus(const std::string &normalizedPath)
        {
            if (AudioPathContains(normalizedPath, "/ui/") || AudioPathContains(normalizedPath, "ui_"))
            {
                return AudioBus::Ui;
            }

            return AudioBus::Sfx;
        }

        inline AudioPlaybackMode InferPlaybackMode(const std::string &normalizedPath)
        {
            if (AudioPathContains(normalizedPath, "/music/") ||
                AudioPathContains(normalizedPath, "music") ||
                AudioPathContains(normalizedPath, "bgm") ||
                AudioPathContains(normalizedPath, "loop") ||
                AudioPathContains(normalizedPath, "ambient"))
            {
                return AudioPlaybackMode::Streamed;
            }

            return AudioPlaybackMode::OneShot;
        }

        inline std::filesystem::path ResolveAudioClipPath(const std::string &path)
        {
            const std::filesystem::path candidate(path);
            if (candidate.is_absolute())
            {
                return candidate;
            }

            const std::filesystem::path direct = std::filesystem::current_path() / candidate;
            if (std::filesystem::exists(direct))
            {
                return direct;
            }

            return candidate;
        }
    }

    template <>
    struct ResourceLoader<AudioClip>
    {
        static std::unique_ptr<AudioClip> Load(const std::string &path)
        {
            LoadDebugStageScope stageScope("Resolve audio clip", 0.5f);

            const std::filesystem::path resolvedPath = detail::ResolveAudioClipPath(path);
            if (!std::filesystem::exists(resolvedPath))
            {
                throw std::runtime_error("Audio clip not found: " + resolvedPath.string());
            }

            auto clip = std::make_unique<AudioClip>(0, path);
            clip->SetSourcePath(resolvedPath.lexically_normal().generic_string());
            clip->SetByteSize(std::filesystem::file_size(resolvedPath));

            const std::string normalizedPath = detail::NormalizeAudioKey(clip->GetSourcePath());
            const AudioPlaybackMode playbackMode = detail::InferPlaybackMode(normalizedPath);
            clip->SetPlaybackMode(playbackMode);
            clip->SetBus(playbackMode == AudioPlaybackMode::Streamed ? AudioBus::Music : detail::InferAudioBus(normalizedPath));
            clip->SetLoadMode(playbackMode == AudioPlaybackMode::Streamed ? AudioLoadMode::Stream : AudioLoadMode::Auto);
            return clip;
        }
    };
}