#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "scene.hpp"

#include "render/shader.hpp"
#include "render/camera/camera_perspective.hpp"
#include "render/model.hpp"
#include "resources/manager.hpp"

class SceneBackpack : public Scene
{
public:
    SceneBackpack(GLFWwindow *window, int width, int height, ResourceManager *resourceManager);
    ~SceneBackpack() override;

    void Init() override;
    void Update(float deltaTime) override;
    void Draw(float deltaTime) override;

private:
    GLFWwindow *m_window;
    int m_width, m_height;
    ResourceManager *m_resourceManager;

    PerspectiveCamera m_camera;
    Model m_backpack;
    Model m_light;
};