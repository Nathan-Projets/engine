#include "keyboard_state.hpp"

bool KeyboardState::IsKeyPressed(int key) const
{
    auto it = m_keyState.find(key);
    if (it == m_keyState.end())
        return false;
    return it->second.first;
}

bool KeyboardState::IsKeyJustPressed(int key) const
{
    auto it = m_keyState.find(key);
    if (it == m_keyState.end())
        return false;
    return it->second.first && !it->second.second;  // pressed now, not last frame
}

bool KeyboardState::IsKeyReleased(int key) const
{
    auto it = m_keyState.find(key);
    if (it == m_keyState.end())
        return false;
    return !it->second.first && it->second.second;  // released now, was pressed last frame
}

void KeyboardState::RegisterAction(const std::string &name, int key)
{
    m_actionMap[name].push_back(key);
}

bool KeyboardState::IsActionActive(const std::string &name) const
{
    auto it = m_actionMap.find(name);
    if (it == m_actionMap.end())
        return false;

    for (int key : it->second)
    {
        if (IsKeyPressed(key))
            return true;
    }
    return false;
}

void KeyboardState::OnKeyEvent(int key, int action)
{
    bool isPressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
    bool previousState = m_keyState[key].first;
    m_keyState[key] = {isPressed, previousState};
}

void KeyboardState::Update()
{
    for (auto &[key, state] : m_keyState)
    {
        state.second = state.first;  // previous = current
    }
}
