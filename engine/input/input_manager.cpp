#include "input_manager.hpp"

void InputManager::AttachToWindow(GLFWwindow *window)
{
    if (!window)
        return;

    m_window = window;
    glfwSetKeyCallback(window, InputManager::keyCallback);
    glfwSetCursorPosCallback(window, InputManager::mouseMoveCallback);
    glfwSetMouseButtonCallback(window, InputManager::mouseButtonCallback);
    glfwSetScrollCallback(window, InputManager::scrollCallback);
}

void InputManager::Update()
{
    m_keyboardState.Update();
    m_mouseState.Update();
}

void InputManager::ClearFrameState()
{
    m_mouseState.ClearEphemeralState();
}

void InputManager::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    InputManager::Get().onKeyEvent(key, action);
}

void InputManager::mouseMoveCallback(GLFWwindow *window, double xpos, double ypos)
{
    InputManager::Get().onMouseMove(xpos, ypos);
}

void InputManager::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    InputManager::Get().onMouseButton(button, action);
}

void InputManager::scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    InputManager::Get().onScroll(xoffset, yoffset);
}

void InputManager::onKeyEvent(int key, int action)
{
    m_keyboardState.OnKeyEvent(key, action);
}

void InputManager::onMouseMove(double x, double y)
{
    m_mouseState.OnMouseMove(x, y);
}

void InputManager::onMouseButton(int button, int action)
{
    m_mouseState.OnMouseButton(button, action);
}

void InputManager::onScroll(double xoffset, double yoffset)
{
    m_mouseState.OnScroll(xoffset, yoffset);
}
