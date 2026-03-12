#pragma once

#include <utility>

#include <glm/vec2.hpp>
#include <GLFW/glfw3.h>

class MouseState
{
public:
    MouseState() = default;
    ~MouseState() = default;

    glm::vec2 GetPosition() const { return m_position; }
    glm::vec2 GetDelta() const { return m_position - m_positionLastFrame; }

    bool IsButtonPressed(int button) const;
    bool IsButtonJustPressed(int button) const;
    bool IsButtonReleased(int button) const;

    glm::vec2 GetScroll() const { return m_scroll; }

    void OnMouseMove(double x, double y);
    void OnMouseButton(int button, int action);
    void OnScroll(double xoffset, double yoffset);

    // Update state tracking (button previous states, position history)
    void Update();
    // Clear ephemeral state (scroll) - call after consuming the values
    void ClearEphemeralState();

private:
    glm::vec2 m_position = {0, 0};
    glm::vec2 m_positionLastFrame = {0, 0};
    glm::vec2 m_scroll = {0, 0};

    // Button state: {current_frame, previous_frame}
    // Index: 0 = left, 1 = right, 2 = middle
    std::pair<bool, bool> m_buttons[3] = {
        {false, false},
        {false, false},
        {false, false}};

    int getGLFWButtonIndex(int glfwButton) const;
};
