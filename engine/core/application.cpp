#include "application.hpp"

Application::Application(int width, int height) : m_window(nullptr), m_width(width), m_height(height), m_bShouldExit(false), m_initialized(false)
{
}

Application::~Application()
{
    m_debugUI.Shutdown();
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
    glfwSwapInterval(0); // 0 = vsync off, 1 = vsync on (capped to monitor refresh rate)
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, Application::ResizeCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        ERROR("GLAD not initialized.");
        return false;
    }

    InputManager::Get().AttachToWindow(m_window);

    m_debugUI.Init(m_window);
    m_debugUI.AddPanel(&m_statsPanel);
    m_debugUI.AddPanel(&m_loadingPanel);

    m_initialized = true;

    InputManager::Get().RegisterAction("move_forward", GLFW_KEY_W);
    InputManager::Get().RegisterAction("move_backward", GLFW_KEY_S);
    InputManager::Get().RegisterAction("move_left", GLFW_KEY_A);
    InputManager::Get().RegisterAction("move_right", GLFW_KEY_D);
    InputManager::Get().RegisterAction("jump", GLFW_KEY_SPACE);
    InputManager::Get().RegisterAction("reload_scene", GLFW_KEY_R);
    InputManager::Get().RegisterAction("reload_scene_clean", GLFW_KEY_F5);

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

    m_loadingPanel.SetResourceManager(m_scene ? m_scene->GetResourceManager() : nullptr);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    float smoothedDeltaTime = 1.0f / 60.0f;
    double fpsAccumulatedTime = 0.0;
    int fpsAccumulatedFrames = 0;
    int fpsStable = 60;
    uint64_t totalFrameCount = 0;
    const double startTime = glfwGetTime();

    while (!glfwWindowShouldClose(m_window))
    {
        InputManager::Get().Update();

        glfwPollEvents();

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (InputManager::Get().IsKeyPressed(GLFW_KEY_ESCAPE))
        {
            Stop();
        }

        if (InputManager::Get().IsKeyJustPressed(GLFW_KEY_F1))
        {
            m_statsPanel.visible = !m_statsPanel.visible;
            m_loadingPanel.visible = !m_loadingPanel.visible;
        }

        if (m_scene)
        {
            m_scene->Render(deltaTime);
        }

        // TODO: create abstraction or idk because these are nasty
        smoothedDeltaTime += (deltaTime - smoothedDeltaTime) * 0.10f;
        fpsAccumulatedTime += static_cast<double>(deltaTime);
        ++fpsAccumulatedFrames;
        ++totalFrameCount;
        if (fpsAccumulatedTime >= 0.25)
        {
            fpsStable = static_cast<int>(static_cast<double>(fpsAccumulatedFrames) / fpsAccumulatedTime + 0.5);
            fpsAccumulatedTime = 0.0;
            fpsAccumulatedFrames = 0;
        }

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);

        FrameStats stats;
        stats.fps = fpsStable;
        stats.frameTimeMs = smoothedDeltaTime * 1000.0f;
        stats.totalFrames = totalFrameCount;
        stats.uptimeSeconds = glfwGetTime() - startTime;
        stats.viewportWidth = framebufferWidth;
        stats.viewportHeight = framebufferHeight;
        stats.mousePosition = InputManager::Get().GetMousePosition();
        stats.sceneLoaded = m_scene != nullptr;
        m_statsPanel.SetStats(stats);

        m_debugUI.BeginFrame();
        m_debugUI.DrawPanels();
        m_debugUI.EndFrame();

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

    Application *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app && app->m_scene)
    {
        app->m_width = width;
        app->m_height = height;
        app->m_scene->OnResize(width, height);
    }
}
