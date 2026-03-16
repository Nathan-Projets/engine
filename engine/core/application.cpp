#include "application.hpp"

Application::Application(int width, int height) : m_window(nullptr), m_width(width), m_height(height), m_bShouldExit(false), m_initialized(false)
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

    InputManager::Get().AttachToWindow(m_window);

    m_initialized = true;

    InputManager::Get().RegisterAction("move_forward", GLFW_KEY_W);
    InputManager::Get().RegisterAction("move_backward", GLFW_KEY_S);
    InputManager::Get().RegisterAction("move_left", GLFW_KEY_A);
    InputManager::Get().RegisterAction("move_right", GLFW_KEY_D);
    InputManager::Get().RegisterAction("jump", GLFW_KEY_SPACE);
    InputManager::Get().RegisterAction("reload_scene", GLFW_KEY_R);

    glEnable(GL_DEPTH_TEST);

    return true;
}

bool Application::Run()
{
    if (!m_initialized)
    {
        if (!Init())
        {
            return false;
        }
    }

    if (m_scene)
    {
        m_scene->Init();
    }

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(m_window))
    {
        InputManager::Get().Update();

        glfwPollEvents();

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (InputManager::Get().IsKeyPressed(GLFW_KEY_ESCAPE))
        {
            Stop();
        }

        if (m_scene)
        {
            m_scene->Render(deltaTime);
        }

        InputManager::Get().ClearFrameState();

        glfwSwapBuffers(m_window);
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
