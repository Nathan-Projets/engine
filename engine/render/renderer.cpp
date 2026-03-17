#include "renderer.hpp"

#include <algorithm>

void RenderFrameSnapshot::Clear()
{
    lights.clear();
    opaqueUnits.clear();
    transparentUnits.clear();
    unlitUnits.clear();
    debugUnits.clear();
}

void Renderer::Initialize(uint32_t width, uint32_t height, const RendererOptions &options)
{
    m_viewportWidth = width;
    m_viewportHeight = height;
    m_options = options;
    m_initialized = true;
    m_frameOpen = false;
    m_buildingSnapshot.Clear();

    // Create uniform buffer manager for standard layouts
    m_uniformBufferManager = new UniformBufferManager();
}

void Renderer::Shutdown()
{
    m_initialized = false;
    m_frameOpen = false;
    m_buildingSnapshot.Clear();

    if (m_uniformBufferManager)
    {
        delete m_uniformBufferManager;
        m_uniformBufferManager = nullptr;
    }
}

void Renderer::Resize(uint32_t width, uint32_t height)
{
    m_viewportWidth = width;
    m_viewportHeight = height;
}

void Renderer::BeginFrame(const CameraRenderData &camera)
{
    if (!m_initialized)
        return;

    m_buildingSnapshot.Clear();
    m_buildingSnapshot.camera = camera;
    m_frameOpen = true;
}

void Renderer::Submit(const RenderUnit &unit, RenderQueue queue)
{
    if (!m_initialized || !m_frameOpen)
        return;

    switch (queue)
    {
    case RenderQueue::Opaque:
        m_buildingSnapshot.opaqueUnits.push_back(unit);
        break;
    case RenderQueue::Transparent:
        m_buildingSnapshot.transparentUnits.push_back(unit);
        break;
    case RenderQueue::Unlit:
        m_buildingSnapshot.unlitUnits.push_back(unit);
        break;
    case RenderQueue::Debug:
        m_buildingSnapshot.debugUnits.push_back(unit);
        break;
    }
}

void Renderer::SubmitLight(const LightRenderData &light)
{
    if (!m_initialized || !m_frameOpen)
        return;

    m_buildingSnapshot.lights.push_back(light);
}

void Renderer::EndFrame()
{
    if (!m_initialized || !m_frameOpen)
        return;

    SortQueues(m_buildingSnapshot);
    ExecuteFrame(m_buildingSnapshot);
    m_frameOpen = false;
}

void Renderer::Render(const RenderFrameSnapshot &snapshot)
{
    if (!m_initialized)
        return;

    RenderFrameSnapshot sortedSnapshot = snapshot;
    SortQueues(sortedSnapshot);
    ExecuteFrame(sortedSnapshot);
}

const RendererOptions &Renderer::GetOptions() const
{
    return m_options;
}

void Renderer::SetOptions(const RendererOptions &options)
{
    m_options = options;
}

void Renderer::SortQueues(RenderFrameSnapshot &snapshot) const
{
    if (!m_options.sortTransparentBackToFront)
        return;

    const glm::vec3 cameraPosition = snapshot.camera.position;
    std::sort(snapshot.transparentUnits.begin(), snapshot.transparentUnits.end(), [&cameraPosition](const RenderUnit &a, const RenderUnit &b)
              {
                  const glm::vec3 pa = glm::vec3(a.model[3]);
                  const glm::vec3 pb = glm::vec3(b.model[3]);
                  const glm::vec3 deltaA = pa - cameraPosition;
                  const glm::vec3 deltaB = pb - cameraPosition;
                  const float da = glm::dot(deltaA, deltaA);
                  const float db = glm::dot(deltaB, deltaB);
                  return da > db; });
}

void Renderer::ExecuteFrame(const RenderFrameSnapshot &snapshot)
{
    if (m_options.depthPrepass)
    {
        ExecuteDepthPrepass(snapshot);
    }

    ExecuteOpaquePass(snapshot);
    ExecuteTransparentPass(snapshot);
    ExecuteUnlitPass(snapshot);
    ExecuteDebugPass(snapshot);
}

void Renderer::ExecuteDepthPrepass(const RenderFrameSnapshot &snapshot)
{
    (void)snapshot;
}

void Renderer::ExecuteOpaquePass(const RenderFrameSnapshot &snapshot)
{
    if (!m_uniformBufferManager)
        return;

    // Convert camera data to UBO format
    CameraData cameraData;
    cameraData.view = snapshot.camera.view;
    cameraData.projection = snapshot.camera.projection;
    cameraData.viewProjection = snapshot.camera.viewProjection;
    cameraData.invViewProjection = snapshot.camera.invViewProjection;
    cameraData.cameraPosition = snapshot.camera.position;
    cameraData.near = snapshot.camera.near;
    cameraData.far = snapshot.camera.far;

    m_uniformBufferManager->UpdateCameraData(cameraData);

    if (!snapshot.lights.empty())
    {
        std::vector<LightData> lightDataArray;
        for (const LightRenderData &renderLight : snapshot.lights)
        {
            LightData lightData;
            lightData.position = renderLight.position;
            lightData.color = renderLight.color;
            lightDataArray.push_back(lightData);
        }
        if (!lightDataArray.empty())
        {
            m_uniformBufferManager->UpdateLightData(lightDataArray.data(), static_cast<unsigned int>(lightDataArray.size()));
        }
    }

    m_uniformBufferManager->BindAllBuffers();

    const LightRenderData *mainLight = snapshot.lights.empty() ? nullptr : &snapshot.lights[0];

    for (const RenderUnit &unit : snapshot.opaqueUnits)
    {
        if (!unit.mesh || !unit.material || !unit.material->shader)
            continue;

        Shader &shader = *unit.material->shader;
        unit.material->Use();

        // Convert object data to UBO format
        ObjectData objectData;
        objectData.model = unit.model;
        objectData.normalMatrix = glm::mat4(glm::mat3(glm::transpose(glm::inverse(glm::mat3(unit.model)))));
        objectData.objectId = 0; // TODO: Set from entity handle
        objectData.materialIndex = 0;
        m_uniformBufferManager->UpdateObjectData(objectData);

        // Legacy fallback: still upload via shader for compatibility (until shaders are migrated to UBO layout)
        shader.Upload("view", snapshot.camera.view);
        shader.Upload("projection", snapshot.camera.projection);
        shader.Upload("viewPos", snapshot.camera.position);
        shader.Upload("model", unit.model);

        if (mainLight)
        {
            shader.Upload("light.position", mainLight->position);
            shader.Upload("light.ambient", mainLight->ambient);
            shader.Upload("light.diffuse", mainLight->diffuse);
            shader.Upload("light.specular", mainLight->specular);
            shader.Upload("color", mainLight->color);
            shader.Upload("material.shininess", 32.0f);
        }

        unit.mesh->Draw(shader);
    }
}

void Renderer::ExecuteTransparentPass(const RenderFrameSnapshot &snapshot)
{
    (void)snapshot;
}

void Renderer::ExecuteUnlitPass(const RenderFrameSnapshot &snapshot)
{
    (void)snapshot;
}

void Renderer::ExecuteDebugPass(const RenderFrameSnapshot &snapshot)
{
    (void)snapshot;
}
