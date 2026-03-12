#pragma once

#include <unordered_map>
#include <vector>
#include <utility>
#include <string>

#include <GLFW/glfw3.h>

class KeyboardState
{
public:
    KeyboardState() = default;
    ~KeyboardState() = default;

    bool IsKeyPressed(int key) const;
    bool IsKeyJustPressed(int key) const;
    bool IsKeyReleased(int key) const;

    void RegisterAction(const std::string &name, int key);
    bool IsActionActive(const std::string &name) const;

    void OnKeyEvent(int key, int action);
    void Update();

private:
    // key state value: {current_frame, previous_frame}
    std::unordered_map<int, std::pair<bool, bool>> m_keyState;
    std::unordered_map<std::string, std::vector<int>> m_actionMap;
};
