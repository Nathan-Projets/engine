#include "renderer.hpp"

namespace
{
    GLint GetUniformLocation(GLuint programId, const char *name)
    {
        return glGetUniformLocation(programId, name);
    }

    void SetUniformInt(GLuint programId, const char *name, int value)
    {
        const GLint location = GetUniformLocation(programId, name);
        if (location >= 0)
        {
            glUniform1i(location, value);
        }
    }

    int BindTextureForSlot(
        const RenderUnit &unit,
        resources::ResourceManager *resourceManager,
        resources::MaterialTextureSlot slot,
        int textureUnit)
    {
        if (!unit.resourceMaterial || !resourceManager)
        {
            return 0;
        }

        const std::optional<resources::Handle<resources::Texture>> textureHandle = unit.resourceMaterial->GetTextureHandle(slot);
        if (!textureHandle.has_value() || !textureHandle.value().IsValid())
        {
            return 0;
        }

        resources::Texture *texture = resourceManager->Get(textureHandle.value());
        if (!texture || !texture->IsGpuReady())
        {
            return 0;
        }

        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, texture->GetTextureId());
        return 1;
    }

    void BindResourceMaterial(const RenderUnit &unit, resources::ResourceManager *resourceManager)
    {
        if (!unit.resourceShader)
        {
            return;
        }

        const GLuint programId = unit.resourceShader->GetProgramId();

        int nextTextureUnit = 0;

        int diffuseCount = BindTextureForSlot(unit, resourceManager, resources::MaterialTextureSlot::BaseColor, nextTextureUnit);
        if (diffuseCount > 0)
        {
            SetUniformInt(programId, "uDiffuse[0]", nextTextureUnit);
            ++nextTextureUnit;
        }
        SetUniformInt(programId, "uDiffuseCount", diffuseCount);

        int normalCount = BindTextureForSlot(unit, resourceManager, resources::MaterialTextureSlot::Normal, nextTextureUnit);
        if (normalCount > 0)
        {
            SetUniformInt(programId, "uNormal[0]", nextTextureUnit);
            ++nextTextureUnit;
        }
        SetUniformInt(programId, "uNormalCount", normalCount);

        int ambientCount = BindTextureForSlot(unit, resourceManager, resources::MaterialTextureSlot::Occlusion, nextTextureUnit);
        if (ambientCount > 0)
        {
            SetUniformInt(programId, "uAmbient[0]", nextTextureUnit);
            ++nextTextureUnit;
        }
        SetUniformInt(programId, "uAmbientCount", ambientCount);

        int specularCount = BindTextureForSlot(unit, resourceManager, resources::MaterialTextureSlot::MetallicRoughness, nextTextureUnit);
        if (specularCount > 0)
        {
            SetUniformInt(programId, "uSpecular[0]", nextTextureUnit);
            ++nextTextureUnit;
        }
        SetUniformInt(programId, "uSpecularCount", specularCount);

        (void)nextTextureUnit;
    }

    void DrawResourceUnit(const RenderUnit &unit, resources::ResourceManager *resourceManager)
    {
        if (!unit.resourceMesh || !unit.resourceShader)
        {
            return;
        }

        if (!unit.resourceMesh->IsGpuReady() || !unit.resourceShader->IsGpuReady())
        {
            return;
        }

        glUseProgram(unit.resourceShader->GetProgramId());
        BindResourceMaterial(unit, resourceManager);
        glBindVertexArray(unit.resourceMesh->GetVao());

        const std::vector<resources::MeshPrimitive> &primitives = unit.resourceMesh->GetPrimitives();
        if (!primitives.empty())
        {
            for (const resources::MeshPrimitive &primitive : primitives)
            {
                glDrawElements(
                    GL_TRIANGLES,
                    static_cast<GLsizei>(primitive.indexCount),
                    GL_UNSIGNED_INT,
                    reinterpret_cast<const void *>(static_cast<size_t>(primitive.indexOffset) * sizeof(uint32_t)));
            }
        }
        else
        {
            glDrawElements(
                GL_TRIANGLES,
                static_cast<GLsizei>(unit.resourceMesh->GetIndices().size()),
                GL_UNSIGNED_INT,
                nullptr);
        }

        glBindVertexArray(0);
    }
}

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
    m_resourceGpuUploader = std::make_unique<ResourceGpuUploader>();

    if (m_resourceManager)
    {
        m_resourceManager->SetGPUUploadCallback(
            [this](resources::Resource *resource)
            {
                if (m_resourceGpuUploader)
                {
                    m_resourceGpuUploader->Upload(resource);
                }
            });

        m_resourceManager->SetGPUReleaseCallback(
            [this](resources::Resource *resource)
            {
                if (m_resourceGpuUploader)
                {
                    m_resourceGpuUploader->Release(resource);
                }
            });
    }
}

void Renderer::Shutdown()
{
    if (m_resourceManager)
    {
        m_resourceManager->SetGPUUploadCallback(nullptr);
        m_resourceManager->SetGPUReleaseCallback(nullptr);
    }

    m_initialized = false;
    m_frameOpen = false;
    m_buildingSnapshot.Clear();
    m_uniformBufferManager.reset(nullptr);
    m_resourceGpuUploader.reset(nullptr);
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

    if (m_resourceManager)
    {
        m_resourceManager->ProcessGPUUploads();
    }

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

    if (m_resourceManager)
    {
        m_resourceManager->ProcessGPUUploads();
    }

    RenderFrameSnapshot sortedSnapshot = snapshot;
    SortQueues(sortedSnapshot);
    ExecuteFrame(sortedSnapshot);
}

void Renderer::SetResourceManager(resources::ResourceManager *resourceManager)
{
    m_resourceManager = resourceManager;

    if (!m_resourceManager)
    {
        return;
    }

    m_resourceManager->SetGPUUploadCallback(
        [this](resources::Resource *resource)
        {
            if (m_resourceGpuUploader)
            {
                m_resourceGpuUploader->Upload(resource);
            }
        });

    m_resourceManager->SetGPUReleaseCallback(
        [this](resources::Resource *resource)
        {
            if (m_resourceGpuUploader)
            {
                m_resourceGpuUploader->Release(resource);
            }
        });
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
        if (unit.resourceMesh && unit.resourceShader)
        {
            UploadObjectUbo(unit);
            UploadMaterialUbo(unit);
            DrawResourceUnit(unit, m_resourceManager);
        }
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
        if (unit.resourceMesh && unit.resourceShader)
        {
            UploadObjectUbo(unit);
            UploadMaterialUbo(unit);
            DrawResourceUnit(unit, m_resourceManager);
        }
    }
}

void Renderer::ExecuteDebugPass(const RenderFrameSnapshot &snapshot)
{
}
