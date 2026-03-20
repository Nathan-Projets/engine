#include "application.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

Application::Application(int width, int height) : m_window(nullptr), m_width(width), m_height(height), m_bShouldExit(false), m_initialized(false)
{
}

Application::~Application()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

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
    float smoothedDeltaTime = 1.0f / 60.0f;
    double fpsAccumulatedTime = 0.0;
    int fpsAccumulatedFrames = 0;
    int fpsStable = 60;
    uint64_t totalFrameCount = 0;
    bool showDebugOverlay = true;
    const double startTime = glfwGetTime();

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

        if (InputManager::Get().IsKeyJustPressed(GLFW_KEY_F1))
        {
            showDebugOverlay = !showDebugOverlay;
        }

        if (m_scene)
        {
            m_scene->Render(deltaTime);
        }

        // TODO: create abstraction or idk because these are nasty
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

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

        if (showDebugOverlay)
        {
            int framebufferWidth = 0;
            int framebufferHeight = 0;
            glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);

            const glm::vec2 mousePosition = InputManager::Get().GetMousePosition();
            const double uptimeSeconds = glfwGetTime() - startTime;

            ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.85f);
            ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
            ImGui::Begin("Debug Stats", nullptr, overlayFlags);
            ImGui::Text("FPS: %d", fpsStable);
            ImGui::Text("Frame time: %.2f ms", smoothedDeltaTime * 1000.0f);
            ImGui::Separator();
            ImGui::Text("Uptime: %.1f s", uptimeSeconds);
            ImGui::Text("Frame: %llu", static_cast<unsigned long long>(totalFrameCount));
            ImGui::Text("Viewport: %d x %d", framebufferWidth, framebufferHeight);
            ImGui::Text("Mouse: (%.1f, %.1f)", mousePosition.x, mousePosition.y);
            ImGui::Text("Scene bound: %s", m_scene ? "yes" : "no");
            ImGui::Text("Toggle panel: F1");
            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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
