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
        ERROR("GLFW window not created.");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSetFramebufferSizeCallback(m_window, Application::ResizeCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        ERROR("GLAD not initialized.");
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

    // timing
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    // resources/shaders
    ResourceManager resourceManager;
    // scene with one backpack and one light
    // m_scenes.push_back(std::make_unique<SceneBackpack>(m_window, m_width, m_height, &resourceManager));
    // scene with one cube to test custom object loader
    m_scenes.push_back(std::make_unique<SceneLoadingTest>(m_window, m_width, m_height, &resourceManager));

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(1.2f, 1.0f, 2.0f));
    model = glm::scale(model, glm::vec3(0.1f));

    const float cameraSpeed = 12.5f;

    while (!glfwWindowShouldClose(m_window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // application controls
        if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            Stop();
        if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(m_window, true);

        // There should only ever be one scene in the vector but it's convenient to use for debug (un/comment scenes)
        for (auto &scene : m_scenes)
        {
            scene->Render(deltaTime);
        }

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
