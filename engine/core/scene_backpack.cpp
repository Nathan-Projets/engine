#include "scene_backpack.hpp"

SceneBackpack::SceneBackpack(GLFWwindow *window, int width, int height, ResourceManager *resourceManager)
{
    m_window = window;
    m_width = width;
    m_height = height;
    m_resourceManager = resourceManager;

    Init();
}

SceneBackpack::~SceneBackpack()
{
}

void SceneBackpack::Init()
{
    m_camera = PerspectiveCamera({45.0f, (float)m_width, (float)m_height, 0.1f, 150.0f}, glm::vec3(0.0f, 0.0f, 9.0f), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    m_backpack = Model("assets/meshes/backpack/backpack.obj");
    m_light = Model("assets/meshes/cube/cube.obj");

    m_resourceManager->Load<Shader>("model", "assets/shaders/model.vert", "assets/shaders/model.frag");
    m_resourceManager->Load<Shader>("light", "assets/shaders/light.vert", "assets/shaders/light.frag");

    m_resourceManager->Get<Shader>("model")->Use();
    m_resourceManager->Get<Shader>("model")->Upload("projection", m_camera.GetProjectionMatrix());
    m_resourceManager->Get<Shader>("model")->Upload("model", glm::mat4(1.0f));
    m_resourceManager->Get<Shader>("model")->Upload("material.shininess", 32.0f);

    m_resourceManager->Get<Shader>("model")->Upload("light.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
    m_resourceManager->Get<Shader>("model")->Upload("light.diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
    m_resourceManager->Get<Shader>("model")->Upload("light.specular", glm::vec3(0.7f, 0.7f, 0.7f));

    m_resourceManager->Get<Shader>("light")->Use();
    m_resourceManager->Get<Shader>("light")->Upload("projection", m_camera.GetProjectionMatrix());
    m_resourceManager->Get<Shader>("light")->Upload("color", glm::vec3(1.0f));
}

void SceneBackpack::Update(float deltaTime)
{
    const float cameraSpeed = 12.5f;

    // camera controls
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
        m_camera.MoveForward(cameraSpeed * deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
        m_camera.MoveBackward(cameraSpeed * deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
        m_camera.MoveLeft(cameraSpeed * deltaTime * 3.0f);
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
        m_camera.MoveRight(cameraSpeed * deltaTime * 3.0f);
}

void SceneBackpack::Draw(float deltaTime)
{
    float currentFrame = static_cast<float>(glfwGetTime());

    const glm::mat4 &view = m_camera.GetViewMatrix();

    glm::mat4 lightModel = glm::mat4(1.0f);
    lightModel = glm::rotate(lightModel, currentFrame, glm::vec3(0.0f, 1.0f, 0.0f));
    lightModel = glm::translate(lightModel, glm::vec3(2.0f, 0.0f, 0.0f));
    lightModel = glm::scale(lightModel, glm::vec3(0.1f));
    glm::vec3 lightPos = glm::vec3(lightModel[3]);

    Shader &lightShader = *(m_resourceManager->Get<Shader>("light").get());
    lightShader.Use();
    lightShader.Upload("view", view);
    m_light.Draw(lightShader, lightModel);

    Shader &modelShader = *(m_resourceManager->Get<Shader>("model").get());
    modelShader.Use();
    modelShader.Upload("view", view);
    modelShader.Upload("light.position", lightPos);
    modelShader.Upload("viewPos", m_camera.GetPosition());
    m_backpack.Draw(modelShader, glm::mat4(1.0f));
}