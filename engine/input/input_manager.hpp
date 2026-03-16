#pragma once

#include <string>
#include <vector>
#include <utility>
#include <functional>
#include <unordered_map>

#include <glm/vec2.hpp>
#include <GLFW/glfw3.h>

#include "mouse_state.hpp"
#include "keyboard_state.hpp"

class InputManager
{
public:
    static InputManager &Get()
    {
        static InputManager instance;
        return instance;
    }

    void AttachToWindow(GLFWwindow *window);
    
    // Update state tracking (previous frame states for keys/buttons)
    void Update();
    
    // Clear ephemeral state (scroll, etc) - call after frame rendering
    void ClearFrameState();

    // === Keyboard ===

    bool IsKeyPressed(int key) const { return m_keyboardState.IsKeyPressed(key); }
    bool IsKeyJustPressed(int key) const { return m_keyboardState.IsKeyJustPressed(key); }
    bool IsKeyReleased(int key) const { return m_keyboardState.IsKeyReleased(key); }

    void RegisterAction(const std::string &name, int key) { m_keyboardState.RegisterAction(name, key); }
    bool IsActionActive(const std::string &name) const { return m_keyboardState.IsActionActive(name); }
    bool IsActionJustPressed(const std::string &name) const { return m_keyboardState.IsActionJustPressed(name); }

    // === Mouse ===

    glm::vec2 GetMousePosition() const { return m_mouseState.GetPosition(); }
    glm::vec2 GetMouseDelta() const { return m_mouseState.GetDelta(); }
    bool IsMouseButtonPressed(int button) const { return m_mouseState.IsButtonPressed(button); }
    bool IsMouseButtonJustPressed(int button) const { return m_mouseState.IsButtonJustPressed(button); }
    bool IsMouseButtonReleased(int button) const { return m_mouseState.IsButtonReleased(button); }
    glm::vec2 GetMouseScroll() const { return m_mouseState.GetScroll(); }

private:
    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void mouseMoveCallback(GLFWwindow *window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);

    void onKeyEvent(int key, int action);
    void onMouseMove(double x, double y);
    void onMouseButton(int button, int action);
    void onScroll(double xoffset, double yoffset);

    InputManager() = default;
    InputManager(const InputManager &) = delete;
    InputManager &operator=(const InputManager &) = delete;

    GLFWwindow *m_window;

    KeyboardState m_keyboardState;
    MouseState m_mouseState;
};