#include "audio_engine.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#ifdef ERROR
#undef ERROR
#endif
#endif

#include <helpers/log.hpp>

namespace
{
    struct AudioEngineState
    {
        bool initialized = false;
        bool warnedUnsupportedPlatform = false;
        uint64_t nextAliasId = 1;
        bool musicLoop = false;
        std::wstring musicAlias;
        std::vector<std::wstring> activeOneShots;
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

#ifdef _WIN32
    std::string Narrow(const std::wstring &value)
    {
        if (value.empty())
        {
            return {};
        }

        const int requiredSize = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (requiredSize <= 0)
        {
            return {};
        }

        std::string result(static_cast<size_t>(requiredSize), '\0');
        WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), requiredSize, nullptr, nullptr);
        return result;
    }

    bool SendMciCommand(const std::wstring &command, std::wstring *output = nullptr)
    {
        wchar_t buffer[256] = {};
        const UINT error = mciSendStringW(command.c_str(), output ? buffer : nullptr, output ? static_cast<UINT>(std::size(buffer)) : 0, nullptr);
        if (error != 0)
        {
            wchar_t errorBuffer[256] = {};
            mciGetErrorStringW(error, errorBuffer, static_cast<UINT>(std::size(errorBuffer)));
            WARNING("Audio MCI command failed: " + Narrow(command) + " | " + Narrow(errorBuffer));
            return false;
        }

        if (output)
        {
            *output = buffer;
        }
        return true;
    }

    std::wstring BuildOpenCommand(const std::filesystem::path &path, const std::wstring &alias)
    {
        std::wstring command = L"open \"" + path.wstring() + L"\"";
        const std::wstring extension = path.extension().wstring();
        if (extension == L".wav")
        {
            command += L" type waveaudio";
        }
        else if (extension == L".mp3" || extension == L".mpeg" || extension == L".mpg")
        {
            command += L" type mpegvideo";
        }

        command += L" alias " + alias;
        return command;
    }

    bool OpenClip(const std::filesystem::path &path, const std::wstring &alias)
    {
        if (!std::filesystem::exists(path))
        {
            WARNING("Audio asset not found: " + path.string());
            return false;
        }

        if (!SendMciCommand(BuildOpenCommand(path, alias)))
        {
            return false;
        }
        return true;
    }

    void CloseAlias(const std::wstring &alias)
    {
        if (!alias.empty())
        {
            SendMciCommand(L"stop " + alias);
            SendMciCommand(L"close " + alias);
        }
    }
#endif
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
#ifndef _WIN32
    if (!state.warnedUnsupportedPlatform)
    {
        WARNING("AudioEngine is currently implemented only on Windows; audio playback is disabled.");
        state.warnedUnsupportedPlatform = true;
    }
#endif
    return true;
}

void AudioEngine::Shutdown()
{
    AudioEngineState &state = GetState();
#ifdef _WIN32
    for (const std::wstring &alias : state.activeOneShots)
    {
        CloseAlias(alias);
    }
    state.activeOneShots.clear();
    CloseAlias(state.musicAlias);
    state.musicAlias.clear();
#endif
    state.initialized = false;
}

void AudioEngine::Update()
{
    AudioEngineState &state = GetState();
    if (!state.initialized)
    {
        return;
    }

#ifdef _WIN32
    if (!state.musicAlias.empty())
    {
        std::wstring mode;
        if (SendMciCommand(L"status " + state.musicAlias + L" mode", &mode) && mode == L"stopped")
        {
            if (state.musicLoop)
            {
                SendMciCommand(L"play " + state.musicAlias + L" from 0");
            }
            else
            {
                CloseAlias(state.musicAlias);
                state.musicAlias.clear();
            }
        }
    }

    for (size_t index = 0; index < state.activeOneShots.size();)
    {
        std::wstring mode;
        if (!SendMciCommand(L"status " + state.activeOneShots[index] + L" mode", &mode) || mode == L"stopped")
        {
            CloseAlias(state.activeOneShots[index]);
            state.activeOneShots.erase(state.activeOneShots.begin() + static_cast<std::ptrdiff_t>(index));
            continue;
        }

        ++index;
    }
#endif
}

bool AudioEngine::PlayOneShot(std::string_view path, float volume)
{
    AudioEngineState &state = GetState();
    if (!Init())
    {
        return false;
    }

    (void)volume;

#ifdef _WIN32
    const std::filesystem::path resolvedPath = ResolveAudioPath(path);
    const std::wstring alias = L"sfx_" + std::to_wstring(state.nextAliasId++);
    if (!OpenClip(resolvedPath, alias))
    {
        return false;
    }

    if (!SendMciCommand(L"play " + alias + L" from 0"))
    {
        CloseAlias(alias);
        return false;
    }

    state.activeOneShots.push_back(alias);
    return true;
#else
    (void)path;
    (void)volume;
    return false;
#endif
}

bool AudioEngine::PlayMusic(std::string_view path, bool loop, float volume)
{
    AudioEngineState &state = GetState();
    if (!Init())
    {
        return false;
    }

    (void)volume;

#ifdef _WIN32
    StopMusic();

    const std::filesystem::path resolvedPath = ResolveAudioPath(path);
    const std::wstring alias = L"music_" + std::to_wstring(state.nextAliasId++);
    if (!OpenClip(resolvedPath, alias))
    {
        return false;
    }

    if (!SendMciCommand(L"play " + alias + L" from 0"))
    {
        CloseAlias(alias);
        return false;
    }

    state.musicLoop = loop;
    state.musicAlias = alias;
    return true;
#else
    (void)path;
    (void)loop;
    (void)volume;
    return false;
#endif
}

void AudioEngine::StopMusic()
{
    AudioEngineState &state = GetState();
#ifdef _WIN32
    state.musicLoop = false;
    CloseAlias(state.musicAlias);
    state.musicAlias.clear();
#endif
}

bool AudioEngine::PauseMusic()
{
    AudioEngineState &state = GetState();
#ifdef _WIN32
    if (state.musicAlias.empty())
    {
        return false;
    }

    return SendMciCommand(L"pause " + state.musicAlias);
#else
    return false;
#endif
}

bool AudioEngine::ResumeMusic()
{
    AudioEngineState &state = GetState();
#ifdef _WIN32
    if (state.musicAlias.empty())
    {
        return false;
    }

    return SendMciCommand(L"resume " + state.musicAlias);
#else
    return false;
#endif
}