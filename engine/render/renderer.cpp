#include "renderer.hpp"

#include "../helpers/log.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace
{
    constexpr int kPointShadowTextureUnitBase = 10;
    constexpr int kShadowTextureUnit = 15;

    size_t TextureSlotToIndex(resources::MaterialTextureSlot slot)
    {
        return static_cast<size_t>(slot);
    }

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

    void SetUniformFloat(GLuint programId, const char *name, float value)
    {
        const GLint location = GetUniformLocation(programId, name);
        if (location >= 0)
        {
            glUniform1f(location, value);
        }
    }

    void SetUniformVec3(GLuint programId, const char *name, const glm::vec3 &value)
    {
        const GLint location = GetUniformLocation(programId, name);
        if (location >= 0)
        {
            glUniform3f(location, value.x, value.y, value.z);
        }
    }

    void SetUniformFloatArray(GLuint programId, const char *name, const float *values, int count)
    {
        const GLint location = GetUniformLocation(programId, name);
        if (location >= 0 && values != nullptr && count > 0)
        {
            glUniform1fv(location, count, values);
        }
    }

    void SetUniformVec3Array(GLuint programId, const char *name, const glm::vec3 *values, int count)
    {
        const GLint location = GetUniformLocation(programId, name);
        if (location >= 0 && values != nullptr && count > 0)
        {
            glUniform3fv(location, count, glm::value_ptr(values[0]));
        }
    }

    void SetUniformMat4(GLuint programId, const char *name, const glm::mat4 &value)
    {
        const GLint location = GetUniformLocation(programId, name);
        if (location >= 0)
        {
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
        }
    }

    void SetUniformMat4Array(GLuint programId, const char *name, const glm::mat4 *values, int count)
    {
        const GLint location = GetUniformLocation(programId, name);
        if (location >= 0 && values != nullptr && count > 0)
        {
            glUniformMatrix4fv(location, count, GL_FALSE, glm::value_ptr(values[0]));
        }
    }

    void DrawUnitGeometryOnly(const RenderUnit &unit)
    {
        if (!unit.resourceModel || !unit.resourceModel->IsGpuReady())
        {
            return;
        }

        glBindVertexArray(unit.resourceModel->GetVao());
        const std::vector<resources::MeshPrimitive> &primitives = unit.resourceModel->GetPrimitives();
        if (unit.primitiveIndex < primitives.size())
        {
            const resources::MeshPrimitive &primitive = primitives[unit.primitiveIndex];
            glDrawElements(
                GL_TRIANGLES,
                static_cast<GLsizei>(primitive.indexCount),
                GL_UNSIGNED_INT,
                reinterpret_cast<const void *>(static_cast<size_t>(primitive.indexOffset) * sizeof(uint32_t)));
        }
        else if (!primitives.empty())
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
                static_cast<GLsizei>(unit.resourceModel->GetIndices().size()),
                GL_UNSIGNED_INT,
                nullptr);
        }

        glBindVertexArray(0);
    }

    void SetTextureUvUniforms(GLuint programId, const RenderUnit &unit)
    {
        SetUniformInt(programId, "uDiffuseUV", static_cast<int>(unit.textureUvIndices[TextureSlotToIndex(resources::MaterialTextureSlot::BaseColor)]));
        SetUniformInt(programId, "uNormalUV", static_cast<int>(unit.textureUvIndices[TextureSlotToIndex(resources::MaterialTextureSlot::Normal)]));
        SetUniformInt(programId, "uSpecularUV", static_cast<int>(unit.textureUvIndices[TextureSlotToIndex(resources::MaterialTextureSlot::MetallicRoughness)]));
        SetUniformInt(programId, "uAmbientUV", static_cast<int>(unit.textureUvIndices[TextureSlotToIndex(resources::MaterialTextureSlot::Occlusion)]));
        SetUniformInt(programId, "uDisplacementUV", static_cast<int>(unit.textureUvIndices[TextureSlotToIndex(resources::MaterialTextureSlot::Displacement)]));
    }

    int BindTextureForSlot(const RenderUnit &unit, resources::ResourceManager *resourceManager, resources::MaterialTextureSlot slot, int textureUnit)
    {
        if (!unit.resourceMaterial || !resourceManager)
        {
            const resources::Handle<resources::Texture> overrideHandle = unit.textureOverrides[TextureSlotToIndex(slot)];
            if (!overrideHandle.IsValid())
            {
                return 0;
            }

            resources::Texture *overrideTexture = resourceManager ? resourceManager->Get(overrideHandle) : nullptr;
            if (!overrideTexture || !overrideTexture->IsGpuReady())
            {
                return 0;
            }

            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, overrideTexture->GetTextureId());
            return 1;
        }

        const resources::Handle<resources::Texture> overrideHandle = unit.textureOverrides[TextureSlotToIndex(slot)];
        if (overrideHandle.IsValid())
        {
            resources::Texture *overrideTexture = resourceManager->Get(overrideHandle);
            if (overrideTexture && overrideTexture->IsGpuReady())
            {
                glActiveTexture(GL_TEXTURE0 + textureUnit);
                glBindTexture(GL_TEXTURE_2D, overrideTexture->GetTextureId());
                return 1;
            }
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
        SetTextureUvUniforms(programId, unit);

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

        int displacementCount = BindTextureForSlot(unit, resourceManager, resources::MaterialTextureSlot::Displacement, nextTextureUnit);
        if (displacementCount > 0)
        {
            SetUniformInt(programId, "uDisplacement", nextTextureUnit);
            ++nextTextureUnit;
        }
        SetUniformInt(programId, "uDisplacementCount", displacementCount);

        (void)nextTextureUnit;
    }

    void DrawResourceUnit(const RenderUnit &unit, resources::ResourceManager *resourceManager, UniformBufferManager *uniformBufferManager)
    {
        if (!unit.resourceModel || !unit.resourceShader)
        {
            return;
        }

        if (!unit.resourceModel->IsGpuReady() || !unit.resourceShader->IsGpuReady())
        {
            return;
        }

        auto uploadObjectUbo = [](UniformBufferManager *uniformBufferManager, const glm::mat4 &modelMatrix, uint32_t materialIndex)
        {
            if (!uniformBufferManager)
            {
                return;
            }

            ObjectData objectData;
            objectData.model = modelMatrix;
            objectData.normalMatrix = glm::mat4(glm::mat3(glm::transpose(glm::inverse(glm::mat3(modelMatrix)))));
            objectData.objectId = 0;
            objectData.materialIndex = materialIndex;
            uniformBufferManager->UpdateObjectData(objectData);
        };

        glUseProgram(unit.resourceShader->GetProgramId());
        BindResourceMaterial(unit, resourceManager);
        glBindVertexArray(unit.resourceModel->GetVao());

        const std::vector<resources::MeshPrimitive> &primitives = unit.resourceModel->GetPrimitives();
        if (unit.primitiveIndex < primitives.size())
        {
            const resources::MeshPrimitive &primitive = primitives[unit.primitiveIndex];
            uploadObjectUbo(uniformBufferManager, unit.model * unit.localTransform, unit.materialIndex);
            glDrawElements(
                GL_TRIANGLES,
                static_cast<GLsizei>(primitive.indexCount),
                GL_UNSIGNED_INT,
                reinterpret_cast<const void *>(static_cast<size_t>(primitive.indexOffset) * sizeof(uint32_t)));
        }
        else if (!primitives.empty())
        {
            for (const resources::MeshPrimitive &primitive : primitives)
            {
                uploadObjectUbo(uniformBufferManager, unit.model, unit.materialIndex);
                glDrawElements(
                    GL_TRIANGLES,
                    static_cast<GLsizei>(primitive.indexCount),
                    GL_UNSIGNED_INT,
                    reinterpret_cast<const void *>(static_cast<size_t>(primitive.indexOffset) * sizeof(uint32_t)));
            }
        }
        else
        {
            uploadObjectUbo(uniformBufferManager, unit.model * unit.localTransform, unit.materialIndex);
            glDrawElements(
                GL_TRIANGLES,
                static_cast<GLsizei>(unit.resourceModel->GetIndices().size()),
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
        lightData.intensity = renderLight.intensity;
        lightData.constant = renderLight.constant;
        lightData.linear = renderLight.linear;
        lightData.quadratic = renderLight.quadratic;
        lightData.direction = renderLight.direction;
        lightData.spotInnerCutoff = glm::cos(glm::radians(renderLight.innerCutoff));
        lightData.spotOuterCutoff = glm::cos(glm::radians(renderLight.outerCutoff));
        lightData.type = renderLight.type;
        lightDataArray.push_back(lightData);
    }

    m_uniformBufferManager->UpdateLightData(lightDataArray.data(), static_cast<unsigned int>(lightDataArray.size()));
}

void Renderer::UploadObjectUbo(const RenderUnit &unit)
{
    if (!m_uniformBufferManager)
        return;

    ObjectData objectData;
    const glm::mat4 objectModel = unit.model * unit.localTransform;
    objectData.model = objectModel;
    objectData.normalMatrix = glm::mat4(glm::mat3(glm::transpose(glm::inverse(glm::mat3(objectModel)))));
    objectData.objectId = 0;
    objectData.materialIndex = unit.materialIndex;
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
    litUnits.clear();
    transparentUnits.clear();
    unlitUnits.clear();
    debugUnits.clear();
}

void Renderer::InitializeShadowResources()
{
    InitializeDirectionalShadowResources();
    InitializePointShadowResources();
}

void Renderer::InitializeDirectionalShadowResources()
{
    glGenFramebuffers(1, &m_directionalShadow.framebuffer);
    glGenTextures(1, &m_directionalShadow.depthTexture);

    glBindTexture(GL_TEXTURE_2D, m_directionalShadow.depthTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT,
        m_directionalShadow.mapSize,
        m_directionalShadow.mapSize,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, m_directionalShadow.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_directionalShadow.depthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        glDeleteFramebuffers(1, &m_directionalShadow.framebuffer);
        glDeleteTextures(1, &m_directionalShadow.depthTexture);
        m_directionalShadow.framebuffer = 0;
        m_directionalShadow.depthTexture = 0;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::InitializePointShadowResources()
{
    glGenFramebuffers(1, &m_pointShadow.framebuffer);
    bool pointShadowFramebufferValid = true;
    glBindFramebuffer(GL_FRAMEBUFFER, m_pointShadow.framebuffer);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    for (unsigned int textureIndex = 0; textureIndex < m_pointShadow.maxLights; ++textureIndex)
    {
        glGenTextures(1, &m_pointShadow.depthCubemaps[textureIndex]);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_pointShadow.depthCubemaps[textureIndex]);
        for (unsigned int face = 0; face < 6; ++face)
        {
            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                0,
                GL_DEPTH_COMPONENT,
                m_pointShadow.mapSize,
                m_pointShadow.mapSize,
                0,
                GL_DEPTH_COMPONENT,
                GL_FLOAT,
                nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_pointShadow.depthCubemaps[textureIndex], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            pointShadowFramebufferValid = false;
            break;
        }
    }

    if (!pointShadowFramebufferValid)
    {
        glDeleteFramebuffers(1, &m_pointShadow.framebuffer);
        m_pointShadow.framebuffer = 0;
        for (unsigned int textureIndex = 0; textureIndex < m_pointShadow.maxLights; ++textureIndex)
        {
            if (m_pointShadow.depthCubemaps[textureIndex] != 0)
            {
                glDeleteTextures(1, &m_pointShadow.depthCubemaps[textureIndex]);
                m_pointShadow.depthCubemaps[textureIndex] = 0;
            }
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::ReleaseShadowResources()
{
    ReleaseDirectionalShadowResources();
    ReleasePointShadowResources();
}

void Renderer::ReleaseDirectionalShadowResources()
{
    if (m_directionalShadow.framebuffer != 0)
    {
        glDeleteFramebuffers(1, &m_directionalShadow.framebuffer);
        m_directionalShadow.framebuffer = 0;
    }
    if (m_directionalShadow.depthTexture != 0)
    {
        glDeleteTextures(1, &m_directionalShadow.depthTexture);
        m_directionalShadow.depthTexture = 0;
    }
}

void Renderer::ReleasePointShadowResources()
{
    if (m_pointShadow.framebuffer != 0)
    {
        glDeleteFramebuffers(1, &m_pointShadow.framebuffer);
        m_pointShadow.framebuffer = 0;
    }
    for (unsigned int textureIndex = 0; textureIndex < Renderer::kMaxPointShadowLights; ++textureIndex)
    {
        if (m_pointShadow.depthCubemaps[textureIndex] != 0)
        {
            glDeleteTextures(1, &m_pointShadow.depthCubemaps[textureIndex]);
            m_pointShadow.depthCubemaps[textureIndex] = 0;
        }
    }
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
    m_directionalShadow.dataValid = false;
    m_pointShadow.dataValid = false;
    m_directionalShadow.invalidDirectionWarningLogged = false;
    m_directionalShadow.materialHandle = {};
    m_directionalShadow.shaderHandle = {};
    m_pointShadow.materialHandle = {};
    m_pointShadow.shaderHandle = {};
    m_pointShadow.depthCubemaps.fill(0);
    m_pointShadow.lightPositions.fill(glm::vec3(0.0f));
    m_pointShadow.farPlanes.fill(0.0f);
    m_pointShadow.casterCount = 0;
    m_pointShadow.maxLights = std::clamp(m_options.maxPointShadowLights, 1u, Renderer::kMaxPointShadowLights);
    m_pointShadow.farPlane = std::max(1.0f, m_options.pointShadowFarPlane);

    if (m_resourceManager != nullptr)
    {
        m_directionalShadow.materialHandle = m_resourceManager->Load<resources::Material>(m_options.shadowMaterialPath);
        m_pointShadow.materialHandle = m_resourceManager->Load<resources::Material>(m_options.pointShadowMaterialPath);
    }

    InitializeShadowResources();

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
    m_directionalShadow.dataValid = false;
    m_pointShadow.dataValid = false;
    m_directionalShadow.invalidDirectionWarningLogged = false;
    m_buildingSnapshot.Clear();
    m_uniformBufferManager.reset(nullptr);
    m_resourceGpuUploader.reset(nullptr);

    ReleaseShadowResources();

    m_directionalShadow.materialHandle = {};
    m_directionalShadow.shaderHandle = {};
    m_pointShadow.materialHandle = {};
    m_pointShadow.shaderHandle = {};
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
    case RenderQueue::Lit:
        m_buildingSnapshot.litUnits.push_back(unit);
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
        m_directionalShadow.materialHandle = {};
        m_directionalShadow.shaderHandle = {};
        m_pointShadow.materialHandle = {};
        m_pointShadow.shaderHandle = {};
        return;
    }

    m_directionalShadow.materialHandle = m_resourceManager->Load<resources::Material>(m_options.shadowMaterialPath);
    m_directionalShadow.shaderHandle = {};
    m_pointShadow.materialHandle = m_resourceManager->Load<resources::Material>(m_options.pointShadowMaterialPath);
    m_pointShadow.shaderHandle = {};

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
    const bool shadowMaterialChanged = (m_options.shadowMaterialPath != options.shadowMaterialPath);
    const bool pointShadowMaterialChanged = (m_options.pointShadowMaterialPath != options.pointShadowMaterialPath);
    m_options = options;
    m_pointShadow.maxLights = std::clamp(m_options.maxPointShadowLights, 1u, Renderer::kMaxPointShadowLights);
    m_pointShadow.farPlane = std::max(1.0f, m_options.pointShadowFarPlane);

    if (shadowMaterialChanged)
    {
        m_directionalShadow.materialHandle = {};
        m_directionalShadow.shaderHandle = {};

        if (m_resourceManager != nullptr)
        {
            m_directionalShadow.materialHandle = m_resourceManager->Load<resources::Material>(m_options.shadowMaterialPath);
        }
    }

    if (pointShadowMaterialChanged)
    {
        m_pointShadow.materialHandle = {};
        m_pointShadow.shaderHandle = {};

        if (m_resourceManager != nullptr)
        {
            m_pointShadow.materialHandle = m_resourceManager->Load<resources::Material>(m_options.pointShadowMaterialPath);
        }
    }
}

void Renderer::SortQueues(RenderFrameSnapshot &snapshot) const
{
    if (!m_options.sortTransparentBackToFront)
        return;

    const glm::vec3 cameraPosition = snapshot.camera.position;
    std::sort(snapshot.transparentUnits.begin(), snapshot.transparentUnits.end(), [&cameraPosition](const RenderUnit &a, const RenderUnit &b)
              {
                  const glm::vec3 pa = glm::vec3((a.model * a.localTransform)[3]);
                  const glm::vec3 pb = glm::vec3((b.model * b.localTransform)[3]);
                  const glm::vec3 deltaA = pa - cameraPosition;
                  const glm::vec3 deltaB = pb - cameraPosition;
                  const float da = glm::dot(deltaA, deltaA);
                  const float db = glm::dot(deltaB, deltaB);
                  return da > db; });
}

void Renderer::ExecuteFrame(const RenderFrameSnapshot &snapshot)
{
    if (m_options.shadowPass)
    {
        ExecuteShadowPass(snapshot);
        ExecutePointShadowPass(snapshot);
    }

    ExecuteLitPass(snapshot);
    ExecuteTransparentPass(snapshot);
    ExecuteUnlitPass(snapshot);
    ExecuteDebugPass(snapshot);
}

void Renderer::ExecuteShadowPass(const RenderFrameSnapshot &snapshot)
{
    m_directionalShadow.dataValid = false;

    if (m_directionalShadow.framebuffer == 0 || m_directionalShadow.depthTexture == 0 || m_resourceManager == nullptr)
    {
        return;
    }

    if (!m_directionalShadow.materialHandle.IsValid())
    {
        m_directionalShadow.materialHandle = m_resourceManager->Load<resources::Material>(m_options.shadowMaterialPath);
    }

    resources::Material *shadowMaterial = m_resourceManager->Get(m_directionalShadow.materialHandle);
    if (shadowMaterial == nullptr || !shadowMaterial->IsLoaded())
    {
        return;
    }

    if (!shadowMaterial->GetShaderHandle().IsValid() && !shadowMaterial->GetShaderPath().empty())
    {
        shadowMaterial->SetShaderHandle(m_resourceManager->Load<resources::Shader>(shadowMaterial->GetShaderPath()));
    }

    if (!m_directionalShadow.shaderHandle.IsValid())
    {
        m_directionalShadow.shaderHandle = shadowMaterial->GetShaderHandle();
    }

    resources::Shader *shadowShader = m_resourceManager->Get(m_directionalShadow.shaderHandle);
    if (shadowShader == nullptr || !shadowShader->IsGpuReady())
    {
        return;
    }

    const GLuint shadowProgram = shadowShader->GetProgramId();
    if (shadowProgram == 0)
    {
        return;
    }

    auto directionalLightIt = std::find_if(
        snapshot.lights.begin(),
        snapshot.lights.end(),
        [](const LightRenderData &light)
        {
            return light.type == 1u && light.intensity > 0.0f;
        });

    if (directionalLightIt == snapshot.lights.end())
    {
        return;
    }

    const glm::vec3 lightDirection = glm::length(directionalLightIt->direction) > 0.0001f ? glm::normalize(directionalLightIt->direction) : glm::vec3(0.0f);

    if (glm::length(lightDirection) <= 0.0001f)
    {
        if (!m_directionalShadow.invalidDirectionWarningLogged)
        {
            WARNING("Shadow pass skipped: directional light direction is near zero. Configure a non-zero direction in the scene.");
            m_directionalShadow.invalidDirectionWarningLogged = true;
        }
        return;
    }

    m_directionalShadow.invalidDirectionWarningLogged = false;

    const glm::vec3 focusPoint = snapshot.camera.position;
    const glm::vec3 lightPosition = focusPoint - lightDirection * 35.0f;
    const glm::mat4 lightView = glm::lookAt(lightPosition, focusPoint, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 lightProjection = glm::ortho(-25.0f, 25.0f, -25.0f, 25.0f, 1.0f, 90.0f);
    m_directionalShadow.lightSpaceMatrix = lightProjection * lightView;

    GLint previousViewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    glViewport(0, 0, m_directionalShadow.mapSize, m_directionalShadow.mapSize);
    glBindFramebuffer(GL_FRAMEBUFFER, m_directionalShadow.framebuffer);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);

    glUseProgram(shadowProgram);
    SetUniformMat4(shadowProgram, "uLightSpaceMatrix", m_directionalShadow.lightSpaceMatrix);

    for (const RenderUnit &unit : snapshot.litUnits)
    {
        if (!unit.resourceModel || !unit.resourceModel->IsGpuReady())
        {
            continue;
        }

        const glm::mat4 modelMatrix = unit.model * unit.localTransform;
        SetUniformMat4(shadowProgram, "uModel", modelMatrix);
        DrawUnitGeometryOnly(unit);
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
    m_directionalShadow.dataValid = true;
}

void Renderer::ExecutePointShadowPass(const RenderFrameSnapshot &snapshot)
{
    m_pointShadow.dataValid = false;
    m_pointShadow.casterCount = 0;

    if (m_pointShadow.framebuffer == 0 || m_resourceManager == nullptr)
    {
        return;
    }

    if (!m_pointShadow.materialHandle.IsValid())
    {
        m_pointShadow.materialHandle = m_resourceManager->Load<resources::Material>(m_options.pointShadowMaterialPath);
    }

    resources::Material *pointShadowMaterial = m_resourceManager->Get(m_pointShadow.materialHandle);
    if (pointShadowMaterial == nullptr || !pointShadowMaterial->IsLoaded())
    {
        return;
    }

    if (!pointShadowMaterial->GetShaderHandle().IsValid() && !pointShadowMaterial->GetShaderPath().empty())
    {
        pointShadowMaterial->SetShaderHandle(m_resourceManager->Load<resources::Shader>(pointShadowMaterial->GetShaderPath()));
    }

    if (!m_pointShadow.shaderHandle.IsValid())
    {
        m_pointShadow.shaderHandle = pointShadowMaterial->GetShaderHandle();
    }

    resources::Shader *pointShadowShader = m_resourceManager->Get(m_pointShadow.shaderHandle);
    if (pointShadowShader == nullptr || !pointShadowShader->IsGpuReady())
    {
        return;
    }

    const GLuint pointShadowProgram = pointShadowShader->GetProgramId();
    if (pointShadowProgram == 0)
    {
        return;
    }

    std::array<const LightRenderData *, Renderer::kMaxPointShadowLights> pointShadowCasters = {};
    unsigned int casterCount = 0;
    for (const LightRenderData &light : snapshot.lights)
    {
        if (light.type == 0u && light.intensity > 0.0f)
        {
            pointShadowCasters[casterCount] = &light;
            ++casterCount;
            if (casterCount >= m_pointShadow.maxLights)
            {
                break;
            }
        }
    }

    if (casterCount == 0)
    {
        return;
    }

    m_pointShadow.farPlane = std::max(1.0f, m_options.pointShadowFarPlane);

    const glm::mat4 shadowProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, m_pointShadow.farPlane);

    GLint previousViewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    glViewport(0, 0, m_pointShadow.mapSize, m_pointShadow.mapSize);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.5f, 3.0f);

    for (unsigned int casterIndex = 0; casterIndex < casterCount; ++casterIndex)
    {
        const glm::vec3 lightPosition = pointShadowCasters[casterIndex]->position;
        m_pointShadow.lightPositions[casterIndex] = lightPosition;
        m_pointShadow.farPlanes[casterIndex] = m_pointShadow.farPlane;

        const glm::mat4 shadowTransforms[6] = {
            shadowProjection * glm::lookAt(lightPosition, lightPosition + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            shadowProjection * glm::lookAt(lightPosition, lightPosition + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            shadowProjection * glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            shadowProjection * glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
            shadowProjection * glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            shadowProjection * glm::lookAt(lightPosition, lightPosition + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

        glBindFramebuffer(GL_FRAMEBUFFER, m_pointShadow.framebuffer);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_pointShadow.depthCubemaps[casterIndex], 0);
        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(pointShadowProgram);
        SetUniformMat4Array(pointShadowProgram, "uShadowMatrices[0]", shadowTransforms, 6);
        SetUniformVec3(pointShadowProgram, "uLightPos", lightPosition);
        SetUniformFloat(pointShadowProgram, "uFarPlane", m_pointShadow.farPlane);

        for (const RenderUnit &unit : snapshot.litUnits)
        {
            if (!unit.resourceModel || !unit.resourceModel->IsGpuReady())
            {
                continue;
            }

            const glm::mat4 modelMatrix = unit.model * unit.localTransform;
            SetUniformMat4(pointShadowProgram, "uModel", modelMatrix);
            DrawUnitGeometryOnly(unit);
        }
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
    m_pointShadow.casterCount = casterCount;
    m_pointShadow.dataValid = true;
}

void Renderer::ExecuteLitPass(const RenderFrameSnapshot &snapshot)
{
    if (!m_uniformBufferManager)
        return;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    UploadCameraUbo(snapshot.camera);
    UploadLightsUbo(snapshot.lights);

    m_uniformBufferManager->BindAllBuffers();
    for (const RenderUnit &unit : snapshot.litUnits)
    {
        if (unit.resourceModel && unit.resourceShader)
        {
            const GLuint programId = unit.resourceShader->GetProgramId();
            glUseProgram(programId);
            SetUniformInt(programId, "uShadowEnabled", m_directionalShadow.dataValid ? 1 : 0);
            SetUniformInt(programId, "uShadowMap", kShadowTextureUnit);
            SetUniformMat4(programId, "uLightSpaceMatrix", m_directionalShadow.lightSpaceMatrix);
            SetUniformInt(programId, "uPointShadowEnabled", m_pointShadow.dataValid ? 1 : 0);
            SetUniformInt(programId, "uPointShadowCount", m_pointShadow.dataValid ? static_cast<int>(m_pointShadow.casterCount) : 0);
            SetUniformVec3Array(
                programId,
                "uPointShadowLightPositions[0]",
                m_pointShadow.lightPositions.data(),
                static_cast<int>(m_pointShadow.casterCount));
            SetUniformFloatArray(
                programId,
                "uPointShadowFarPlanes[0]",
                m_pointShadow.farPlanes.data(),
                static_cast<int>(m_pointShadow.casterCount));

            if (m_directionalShadow.dataValid)
            {
                glActiveTexture(GL_TEXTURE0 + kShadowTextureUnit);
                glBindTexture(GL_TEXTURE_2D, m_directionalShadow.depthTexture);
            }

            for (unsigned int shadowIndex = 0; shadowIndex < Renderer::kMaxPointShadowLights; ++shadowIndex)
            {
                const int textureUnit = kPointShadowTextureUnitBase + static_cast<int>(shadowIndex);
                glActiveTexture(GL_TEXTURE0 + textureUnit);
                if (m_pointShadow.dataValid && shadowIndex < m_pointShadow.casterCount)
                {
                    glBindTexture(GL_TEXTURE_CUBE_MAP, m_pointShadow.depthCubemaps[shadowIndex]);
                }
                else
                {
                    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
                }

                const std::string uniformName = "uPointShadowMaps[" + std::to_string(shadowIndex) + "]";
                SetUniformInt(programId, uniformName.c_str(), textureUnit);
            }

            UploadMaterialUbo(unit);
            DrawResourceUnit(unit, m_resourceManager, m_uniformBufferManager.get());
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
        if (unit.resourceModel && unit.resourceShader)
        {
            UploadMaterialUbo(unit);
            DrawResourceUnit(unit, m_resourceManager, m_uniformBufferManager.get());
        }
    }
}

void Renderer::ExecuteDebugPass(const RenderFrameSnapshot &snapshot)
{
}
