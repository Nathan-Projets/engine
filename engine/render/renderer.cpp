#include "renderer.hpp"

#include <algorithm>

void Renderer::UploadCameraUbo(const CameraRenderData &camera)
{
    if (!m_uniformBufferManager)
        return;

    CameraData cameraData;
    cameraData.view = camera.view;
    cameraData.projection = camera.projection;
    cameraData.viewProjection = camera.viewProjection;
    cameraData.invViewProjection = camera.invViewProjection;
    cameraData.cameraPosition = camera.position;
    cameraData.near = camera.near;
    cameraData.far = camera.far;

    m_uniformBufferManager->UpdateCameraData(cameraData);
}

void Renderer::UploadLightsUbo(const std::vector<LightRenderData> &lights)
{
    if (!m_uniformBufferManager)
        return;

    std::vector<LightData> lightDataArray;
    lightDataArray.reserve(lights.size());
    for (const LightRenderData &renderLight : lights)
    {
        LightData lightData;
        lightData.position = renderLight.position;
        lightData.color = renderLight.color;
        lightData.ambient = renderLight.ambient;
        lightData.diffuse = renderLight.diffuse;
        lightData.specular = renderLight.specular;
        lightData.intensity = 1.0f;
        lightDataArray.push_back(lightData);
    }

    m_uniformBufferManager->UpdateLightData(lightDataArray.data(), static_cast<unsigned int>(lightDataArray.size()));
}

void Renderer::UploadObjectUbo(const RenderUnit &unit)
{
    if (!m_uniformBufferManager)
        return;

    ObjectData objectData;
    objectData.model = unit.model;
    objectData.normalMatrix = glm::mat4(glm::mat3(glm::transpose(glm::inverse(glm::mat3(unit.model)))));
    objectData.objectId = 0;
    objectData.materialIndex = 0;
    m_uniformBufferManager->UpdateObjectData(objectData);
}

void Renderer::UploadMaterialUbo(const RenderUnit &unit) const
{
    if (!m_uniformBufferManager)
        return;

    MaterialData materialData = unit.material;
    materialData.roughness = glm::clamp(materialData.roughness, 0.02f, 1.0f);
    m_uniformBufferManager->UpdateMaterialData(materialData);
}

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
    m_uniformBufferManager = std::make_unique<UniformBufferManager>();
}

void Renderer::Shutdown()
{
    m_initialized = false;
    m_frameOpen = false;
    m_buildingSnapshot.Clear();
    m_uniformBufferManager.reset(nullptr);
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
}

void Renderer::ExecuteOpaquePass(const RenderFrameSnapshot &snapshot)
{
    if (!m_uniformBufferManager)
        return;

    UploadCameraUbo(snapshot.camera);
    UploadLightsUbo(snapshot.lights);

    m_uniformBufferManager->BindAllBuffers();
    for (const RenderUnit &unit : snapshot.opaqueUnits)
    {
        if (!unit.mesh || !unit.shader)
            continue;

        Shader &shader = *unit.shader;
        shader.Use();

        UploadObjectUbo(unit);
        UploadMaterialUbo(unit);

        unit.mesh->Draw(shader);
    }
}

void Renderer::ExecuteTransparentPass(const RenderFrameSnapshot &snapshot)
{
}

void Renderer::ExecuteUnlitPass(const RenderFrameSnapshot &snapshot)
{
    if (!m_uniformBufferManager)
        return;

    UploadCameraUbo(snapshot.camera);
    m_uniformBufferManager->BindAllBuffers();

    for (const RenderUnit &unit : snapshot.unlitUnits)
    {
        if (!unit.mesh || !unit.shader)
            continue;

        Shader &shader = *unit.shader;
        shader.Use();

        UploadObjectUbo(unit);
        UploadMaterialUbo(unit);

        unit.mesh->Draw(shader);
    }
}

void Renderer::ExecuteDebugPass(const RenderFrameSnapshot &snapshot)
{
}
