#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "render_unit.hpp"
#include "uniform_buffer.hpp"
#include "shader_domain.hpp"

enum class RenderQueue : uint8_t
{
    Opaque,
    Transparent,
    Unlit,
    Debug
};

struct CameraRenderData
{
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 viewProjection = glm::mat4(1.0f);
    glm::mat4 invViewProjection = glm::mat4(1.0f);
    glm::vec3 position = glm::vec3(0.0f);
    float near = 0.1f;
    float far = 1000.0f;
};

struct LightRenderData
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 ambient = glm::vec3(0.2f);
    glm::vec3 diffuse = glm::vec3(0.5f);
    glm::vec3 specular = glm::vec3(0.7f);
    glm::vec3 color = glm::vec3(1.0f);
};

struct RendererOptions
{
    bool depthPrepass = false;
    bool frustumCulling = false;
    bool sortTransparentBackToFront = true;
};

struct RenderFrameSnapshot
{
    CameraRenderData camera;
    std::vector<LightRenderData> lights;

    std::vector<RenderUnit> opaqueUnits;
    std::vector<RenderUnit> transparentUnits;
    std::vector<RenderUnit> unlitUnits;
    std::vector<RenderUnit> debugUnits;

    void Clear();
};

class Renderer
{
public:
    Renderer() = default;
    ~Renderer() = default;

    void Initialize(uint32_t width, uint32_t height, const RendererOptions &options = {});
    void Shutdown();
    void Resize(uint32_t width, uint32_t height);

    // Immediate frame API: build the snapshot incrementally then render it.
    void BeginFrame(const CameraRenderData &camera);
    void Submit(const RenderUnit &unit, RenderQueue queue = RenderQueue::Opaque);
    void SubmitLight(const LightRenderData &light);
    void EndFrame();

    // Snapshot API: provide the fully built frame data directly.
    void Render(const RenderFrameSnapshot &snapshot);

    const RendererOptions &GetOptions() const;
    void SetOptions(const RendererOptions &options);

private:
    void SortQueues(RenderFrameSnapshot &snapshot) const;

    // Pipeline entry points.
    void ExecuteFrame(const RenderFrameSnapshot &snapshot);
    void ExecuteDepthPrepass(const RenderFrameSnapshot &snapshot);
    void ExecuteOpaquePass(const RenderFrameSnapshot &snapshot);
    void ExecuteTransparentPass(const RenderFrameSnapshot &snapshot);
    void ExecuteUnlitPass(const RenderFrameSnapshot &snapshot);
    void ExecuteDebugPass(const RenderFrameSnapshot &snapshot);

private:
    RendererOptions m_options = {};
    uint32_t m_viewportWidth = 0;
    uint32_t m_viewportHeight = 0;
    bool m_initialized = false;
    bool m_frameOpen = false;

    RenderFrameSnapshot m_buildingSnapshot = {};
    UniformBufferManager *m_uniformBufferManager = nullptr;
};