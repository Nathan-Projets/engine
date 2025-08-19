#include "application.hpp"

Application::Application(int width, int height) : m_window(nullptr), m_width(width), m_height(height), m_bShouldExit(false)
{
}

Application::~Application()
{
    glfwTerminate();
}

bool Application::Init()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(m_width, m_height, "Engine", nullptr, nullptr);
    if (m_window == nullptr)
    {
        std::println("Error: GLFW window not created.");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSetFramebufferSizeCallback(m_window, Application::ResizeCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::println("Error: GLAD not initialized.");
        return false;
    }

    glEnable(GL_DEPTH_TEST);

    return true;
}

bool Application::Run()
{
    if (!Init())
    {
        return false;
    }

    ResourceManager resourceManager;
    resourceManager.Load<Shader>("backpack", "assets/shaders/backpack.vert", "assets/shaders/backpack.frag");
    resourceManager.Load<Shader>("model", "assets/shaders/model.vert", "assets/shaders/model.frag");
    resourceManager.Load<Shader>("light", "assets/shaders/light.vert", "assets/shaders/light.frag");

    // camera
    glm::vec3 positionCamera = glm::vec3(0.0f, 0.0f, 7.0f);
    PerspectiveCamera::Frustrum frustum = {45.0f, (float)m_width, (float)m_height, 0.1f, 150.0f};
    PerspectiveCamera camera{frustum, positionCamera, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)};

    // timing
    float deltaTime = 0.0f; // time between current frame and last frame
    float lastFrame = 0.0f;

    resourceManager.Get<Shader>("model")->Use();
    resourceManager.Get<Shader>("model")->Upload("projection", camera.GetProjectionMatrix());
    resourceManager.Get<Shader>("model")->Upload("model", glm::mat4(1.0f));

    resourceManager.Get<Shader>("model")->Upload("material.ambient", glm::vec3(1.0f, 0.5f, 0.31f));
    resourceManager.Get<Shader>("model")->Upload("material.diffuse", glm::vec3(1.0f, 0.5f, 0.31f));
    resourceManager.Get<Shader>("model")->Upload("material.specular", glm::vec3(0.5f, 0.5f, 0.5f));
    resourceManager.Get<Shader>("model")->Upload("material.shininess", 32.0f);

    resourceManager.Get<Shader>("model")->Upload("light.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
    resourceManager.Get<Shader>("model")->Upload("light.diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
    resourceManager.Get<Shader>("model")->Upload("light.specular", glm::vec3(1.0f, 1.0f, 1.0f));

    resourceManager.Get<Shader>("light")->Use();
    resourceManager.Get<Shader>("light")->Upload("projection", camera.GetProjectionMatrix());
    resourceManager.Get<Shader>("light")->Upload("color", glm::vec3(1.0f));

    Model backpack("assets/meshes/backpack/backpack.obj");
    Model light("assets/meshes/cube/cube.obj");

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(1.2f, 1.0f, 2.0f));
    model = glm::scale(model, glm::vec3(0.1f));

    const float cameraSpeed = 12.5f;
    bool shouldLightOrbit = false;

    while (!glfwWindowShouldClose(m_window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // application controls
        if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            Stop();
        if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(m_window, true);

        // camera controls
        if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
            camera.MoveForward(cameraSpeed * deltaTime);
        if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
            camera.MoveBackward(cameraSpeed * deltaTime);
        if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
            camera.MoveLeft(cameraSpeed * deltaTime * 3.0f);
        if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
            camera.MoveRight(cameraSpeed * deltaTime * 3.0f);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();

        // light
        Shader &lightShader = *(resourceManager.Get<Shader>("light").get());
        glm::mat4 lightModel = glm::mat4(1.0f);
        lightModel = glm::rotate(lightModel, currentFrame, glm::vec3(0.0f, 1.0f, 0.0f));
        lightModel = glm::translate(lightModel, glm::vec3(5.0f, 0.0f, 0.0f));
        lightModel = glm::scale(lightModel, glm::vec3(0.1f));
        glm::vec3 lightPos = glm::vec3(lightModel[3]);
        lightShader.Use();
        lightShader.Upload("view", view);
        lightShader.Upload("model", lightModel);
        light.Draw(lightShader);

        // backpack
        Shader &modelShader = *(resourceManager.Get<Shader>("model").get());
        modelShader.Use();
        modelShader.Upload("view", view);
        modelShader.Upload("model", glm::mat4(1.0f));
        modelShader.Upload("light.position", lightPos);
        modelShader.Upload("viewPos", camera.GetPosition());
        backpack.Draw(modelShader);

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }

    return true;
}

void Application::Stop()
{
    glfwSetWindowShouldClose(m_window, true);
}

void Application::ResizeCallback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}
