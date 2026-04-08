#pragma once

#include <string_view>

class AudioEngine
{
public:
    static AudioEngine &Get();

    bool Init();
    void Shutdown();
    void Update();

    bool PlayOneShot(std::string_view path, float volume = 1.0f);
    bool PlayMusic(std::string_view path, bool loop = true, float volume = 0.65f);
    void StopMusic();
    bool PauseMusic();
    bool ResumeMusic();

private:
    AudioEngine() = default;
};