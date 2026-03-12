#include "mouse_state.hpp"

bool MouseState::IsButtonPressed(int button) const
{
    int index = getGLFWButtonIndex(button);
    if (index == -1)
        return false;
    return m_buttons[index].first;
}

bool MouseState::IsButtonJustPressed(int button) const
{
    int index = getGLFWButtonIndex(button);
    if (index == -1)
        return false;
    return m_buttons[index].first && !m_buttons[index].second; // pressed now, not last frame
}

bool MouseState::IsButtonReleased(int button) const
{
    int index = getGLFWButtonIndex(button);
    if (index == -1)
        return false;
    return !m_buttons[index].first && m_buttons[index].second; // released now, was pressed last frame
}

void MouseState::OnMouseMove(double x, double y)
{
    m_positionLastFrame = m_position;
    m_position = {x, y};
}

void MouseState::OnMouseButton(int button, int action)
{
    int index = getGLFWButtonIndex(button);
    if (index == -1)
        return;

    bool isPressedCurrently = (action == GLFW_PRESS);
    bool isPressedPreviously = m_buttons[index].first;
    m_buttons[index] = {isPressedCurrently, m_buttons[index].first};
}

void MouseState::OnScroll(double xoffset, double yoffset)
{
    m_scroll = {xoffset, yoffset};
}

void MouseState::Update()
{
    m_positionLastFrame = m_position;
    for (int i = 0; i < 3; i++)
    {
        m_buttons[i].second = m_buttons[i].first; // update previous state
    }
}

void MouseState::ClearEphemeralState()
{
    m_scroll = {0, 0};
}

int MouseState::getGLFWButtonIndex(int glfwButton) const
{
    switch (glfwButton)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
        return 0;
    case GLFW_MOUSE_BUTTON_RIGHT:
        return 1;
    case GLFW_MOUSE_BUTTON_MIDDLE:
        return 2;
    default:
        return -1;
    }
}
