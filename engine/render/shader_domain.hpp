#pragma once

#include <glm/glm.hpp>

/**
 * Uniform Buffer Bindings: Fixed layout points for data that transfers from CPU to GPU.
 * These are shared across all passes in a domain.
 */
enum UniformBufferBinding : unsigned int
{
    UBO_FRAME_DATA = 0,    // Frame constants (time, delta, frame index)
    UBO_CAMERA_DATA = 1,   // Camera view/proj matrices and position
    UBO_OBJECT_DATA = 2,   // Per-draw object transforms
    UBO_LIGHT_DATA = 3,    // Light array or cluster data
    UBO_MATERIAL_DATA = 4, // Per-material surface properties
};

/**
 * Frame-level constants: Uploaded once per BeginFrame.
 * All shaders in all domains may read from this.
 */
struct FrameData
{
    float time = 0.0f;      // Seconds since app start
    float deltaTime = 0.0f; // Time since last frame
    int frameIndex = 0;     // Sequential frame counter
    float _pad0 = 0.0f;
    glm::vec2 viewport = {1.0f, 1.0f}; // Framebuffer dimensions
    float _pad1 = 0.0f;
    float _pad2 = 0.0f;
};

/**
 * Camera-level constants: Uploaded once per camera per frame.
 * Used by LitOpaque, LitTransparent, ShadowCaster, Sky.
 */
struct CameraData
{
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 viewProjection = glm::mat4(1.0f);
    glm::mat4 invViewProjection = glm::mat4(1.0f);
    glm::vec3 cameraPosition = {0.0f, 0.0f, 0.0f};
    float near = 0.1f;
    glm::vec2 _pad0 = {0.0f, 0.0f};
    float far = 1000.0f;
};

/**
 * Object-level constants: Uploaded per draw call.
 * Includes transform and object identity.
 */
struct ObjectData
{
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 normalMatrix = glm::mat4(1.0f);
    unsigned int objectId = 0;      // Entity handle or custom ID
    unsigned int materialIndex = 0; // Index into material data array
    glm::vec2 _pad0 = {0.0f, 0.0f};
};

/**
 * Light source data: Uploaded once per frame.
 * Packed as array or used by compute shader for culling.
 */
struct LightData
{
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    float intensity = 0.0f;

    glm::vec3 color = {0.0f, 0.0f, 0.0f};
    float range = 0.0f;

    glm::vec3 ambient = {0.0f, 0.0f, 0.0f};
    float _padAmbient = 0.0f;

    glm::vec3 diffuse = {0.0f, 0.0f, 0.0f};
    float _padDiffuse = 0.0f;

    glm::vec3 specular = {0.0f, 0.0f, 0.0f};
    float _padSpecular = 0.0f;

    glm::vec3 direction = {0.0f, -1.0f, 0.0f};
    float spotFalloff = 1.0f;

    unsigned int type = 0;  // 0=point, 1=directional, 2=spot
    unsigned int flags = 0; // Cast shadow, etc.
    glm::vec2 _pad0 = {0.0f, 0.0f};
};

/**
 * Material surface properties: Uploaded once per material.
 * Does NOT include textures; those bind independently.
 */
struct MaterialData
{
    glm::vec3 baseColor = {0.8f, 0.8f, 0.8f};
    float metallic = 0.0f;

    glm::vec3 emissive = {0.0f, 0.0f, 0.0f};
    float roughness = 0.5f;

    float alphaCutoff = 0.5f;
    unsigned int features = 0;
    glm::vec2 _pad0 = {0.0f, 0.0f};
};
