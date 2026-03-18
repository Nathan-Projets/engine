#pragma once

#include <cstdint>

#include "resources/resource.hpp"
#include "resources/units/mesh.hpp"
#include "resources/units/shader.hpp"
#include "resources/units/texture.hpp"

class ResourceGpuUploader
{
public:
    void Upload(resources::Resource *resource);
    void Release(resources::Resource *resource);

private:
    void UploadMesh(resources::Mesh &mesh);
    void UploadShader(resources::Shader &shader);
    void UploadTexture(resources::Texture &texture);

    void ReleaseMesh(resources::Mesh &mesh);
    void ReleaseShader(resources::Shader &shader);
    void ReleaseTexture(resources::Texture &texture);

    static uint32_t CompileShaderStage(resources::ShaderStage stage, const std::string &source);
};
