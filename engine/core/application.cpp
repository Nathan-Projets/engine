#include "application.hpp"

#include <imgui.h>

#include <audio/audio_engine.hpp>

Application::Application(int width, int height, Options options) : m_window(nullptr), m_width(width), m_height(height), m_bShouldExit(false), m_initialized(false), m_options(std::move(options))
{
}

Application::~Application()
{
    AudioEngine::Get().Shutdown();
    if (m_debugUIInitialized)
    {
        m_debugUI.Shutdown();
    }
    glfwTerminate();
}

void Application::SetScene(Scene *scene)
{
    m_scene = scene;
    if (m_options.enableGameUI)
    {
        m_gameHUDPanel.SetScene(m_scene);
    }
    if (m_options.enableEditorUI)
    {
        m_editorContext.BindScene(m_scene);
        m_loadingPanel.SetResourceManager(m_scene ? m_scene->GetResourceManager() : nullptr);
        m_animationPanel.SetScene(m_scene);
    }
}

bool Application::Init()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(m_width, m_height, m_options.windowTitle.c_str(), nullptr, nullptr);
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
    AudioEngine::Get().Init();

    if (m_options.enableEditorUI || m_options.enableDebugUI || m_options.enableGameUI)
    {
        m_debugUI.Init(m_window);
        m_debugUIInitialized = true;

        if (m_options.enableDebugUI)
        {
            m_debugUI.AddPanel(&m_statsPanel);
            m_debugUI.AddPanel(&m_animationPanel);
            m_debugUI.AddPanel(&m_loadingPanel);
            m_statsPanel.visible = m_debugPanelsVisible;
            m_animationPanel.visible = m_debugPanelsVisible;
            m_loadingPanel.visible = m_debugPanelsVisible;
        }

        if (m_options.enableEditorUI)
        {
            m_debugUI.AddPanel(&m_filesPanel);
            m_debugUI.AddPanel(&m_outlinerPanel);
            m_debugUI.AddPanel(&m_inspectorPanel);
            m_debugUI.AddPanel(&m_viewportPanel);

            m_filesPanel.SetContext(&m_editorContext);
            m_outlinerPanel.SetContext(&m_editorContext);
            m_inspectorPanel.SetContext(&m_editorContext);
            m_viewportPanel.SetContext(&m_editorContext);

            m_filesPanel.visible = true;
            m_outlinerPanel.visible = true;
            m_inspectorPanel.visible = true;
            m_viewportPanel.visible = true;
        }

        if (m_options.enableGameUI)
        {
            m_gameHUDPanel.SetScene(m_scene);
            m_gameHUDPanel.SetQuitCallback([this]() { Stop(); });
            m_gameHUDPanel.visible = true;
            m_debugUI.AddPanel(&m_gameHUDPanel);
        }
    }

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

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);
        m_scene->OnResize(framebufferWidth, framebufferHeight);
    }

    if (m_options.enableEditorUI)
    {
        m_loadingPanel.SetResourceManager(m_scene ? m_scene->GetResourceManager() : nullptr);
        m_animationPanel.SetScene(m_scene);
        m_editorContext.BindScene(m_scene);
    }

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
        AudioEngine::Get().Update();

        glfwPollEvents();

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        if (m_options.enableGameUI && InputManager::Get().IsKeyJustPressedGlobal(GLFW_KEY_ESCAPE) && m_scene)
        {
            if (!m_scene->SetGamePaused(!m_scene->IsGamePaused()))
            {
                Stop();
            }
        }
        else if (!m_options.enableGameUI && InputManager::Get().IsKeyPressedGlobal(GLFW_KEY_ESCAPE))
        {
            Stop();
        }

        if (m_options.enableDebugUI && InputManager::Get().IsKeyJustPressedGlobal(GLFW_KEY_F1))
        {
            m_debugPanelsVisible = !m_debugPanelsVisible;
            m_statsPanel.visible = m_debugPanelsVisible;
            m_animationPanel.visible = m_debugPanelsVisible;
            m_loadingPanel.visible = m_debugPanelsVisible;
        }

        const bool ctrlDown = InputManager::Get().IsKeyPressedGlobal(GLFW_KEY_LEFT_CONTROL) || InputManager::Get().IsKeyPressedGlobal(GLFW_KEY_RIGHT_CONTROL);
        const bool shiftDown = InputManager::Get().IsKeyPressedGlobal(GLFW_KEY_LEFT_SHIFT) || InputManager::Get().IsKeyPressedGlobal(GLFW_KEY_RIGHT_SHIFT);
        if (m_options.enableEditorUI && ctrlDown && InputManager::Get().IsKeyJustPressedGlobal(GLFW_KEY_S) && m_scene && !m_editorContext.IsPlaying())
        {
            if (m_scene->SaveEditorScene())
            {
                m_editorContext.ClearDirty();
            }
        }

        // ctrl + Z needs to be W for AZERTY
        if (m_options.enableEditorUI && ctrlDown && InputManager::Get().IsKeyJustPressedGlobal(GLFW_KEY_W) && !m_editorContext.IsPlaying())
        {
            if (shiftDown)
            {
                m_editorContext.Redo();
            }
            else
            {
                m_editorContext.Undo();
            }
        }

        if (m_options.enableEditorUI && ctrlDown && InputManager::Get().IsKeyJustPressedGlobal(GLFW_KEY_Y) && !m_editorContext.IsPlaying())
        {
            m_editorContext.Redo();
        }

        if (m_options.enableEditorUI && ctrlDown && InputManager::Get().IsKeyJustPressedGlobal(GLFW_KEY_P))
        {
            m_editorContext.TogglePlayMode();
        }

        if (m_options.enableEditorUI)
        {
            InputManager::Get().SetMouseInputBlocked(!m_editorContext.IsViewportHovered());
            InputManager::Get().SetKeyboardInputBlocked(!m_editorContext.IsViewportFocused());
        }
        else if (m_options.enableGameUI)
        {
            const bool paused = m_scene && m_scene->IsGamePaused();
            InputManager::Get().SetMouseInputBlocked(paused);
            InputManager::Get().SetKeyboardInputBlocked(paused);
        }
        else
        {
            InputManager::Get().SetMouseInputBlocked(false);
            InputManager::Get().SetKeyboardInputBlocked(false);
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
        if (m_options.enableDebugUI)
        {
            m_statsPanel.SetStats(stats);
            m_animationPanel.SetSnapshot(m_scene ? m_scene->GetAnimationDebugSnapshot() : AnimationDebugSnapshot{});
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, framebufferWidth, framebufferHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (m_scene && !m_options.enableEditorUI)
        {
            m_scene->PresentToScreen();
        }

        if (m_debugUIInitialized)
        {
            m_debugUI.BeginFrame();
            m_debugUI.DrawPanels();
            m_debugUI.EndFrame();
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

    Application *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app && app->m_scene)
    {
        app->m_width = width;
        app->m_height = height;
        app->m_scene->OnResize(width, height);
    }
}
