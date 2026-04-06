#include "resource_gpu_uploader.hpp"

#include <stdexcept>
#include <vector>

#include <glad/glad.h>
#include <stb_image.h>

namespace
{
    GLenum ToGlShaderType(resources::ShaderStage stage)
    {
        switch (stage)
        {
        case resources::ShaderStage::Vertex:
            return GL_VERTEX_SHADER;
        case resources::ShaderStage::Fragment:
            return GL_FRAGMENT_SHADER;
        case resources::ShaderStage::Geometry:
            return GL_GEOMETRY_SHADER;
        case resources::ShaderStage::Compute:
            return GL_COMPUTE_SHADER;
        case resources::ShaderStage::TessellationControl:
            return GL_TESS_CONTROL_SHADER;
        case resources::ShaderStage::TessellationEvaluation:
            return GL_TESS_EVALUATION_SHADER;
        default:
            throw std::runtime_error("Unsupported shader stage");
        }
    }

    void ThrowIfShaderCompileFailed(GLuint shaderId)
    {
        GLint success = 0;
        glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
        if (success == GL_TRUE)
        {
            return;
        }

        GLint logLength = 0;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logLength);

        std::string log;
        log.resize(static_cast<size_t>(logLength));
        glGetShaderInfoLog(shaderId, logLength, nullptr, log.data());

        throw std::runtime_error("Shader stage compilation failed: " + log);
    }

    void ThrowIfProgramLinkFailed(GLuint programId)
    {
        GLint success = 0;
        glGetProgramiv(programId, GL_LINK_STATUS, &success);
        if (success == GL_TRUE)
        {
            return;
        }

        GLint logLength = 0;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);

        std::string log = "Unknown link error (no info log available)";
        if (logLength > 1)
        {
            log.resize(static_cast<size_t>(logLength));
            glGetProgramInfoLog(programId, logLength, nullptr, log.data());
        }

        throw std::runtime_error("Shader program link failed: " + log);
    }

    void ResolveTextureFormat(resources::TextureFormat format, GLenum &internalFormat, GLenum &externalFormat, GLenum &dataType)
    {
        switch (format)
        {
        case resources::TextureFormat::R8:
            internalFormat = GL_R8;
            externalFormat = GL_RED;
            dataType = GL_UNSIGNED_BYTE;
            return;
        case resources::TextureFormat::RG8:
            internalFormat = GL_RG8;
            externalFormat = GL_RG;
            dataType = GL_UNSIGNED_BYTE;
            return;
        case resources::TextureFormat::RGB8:
            internalFormat = GL_RGB8;
            externalFormat = GL_RGB;
            dataType = GL_UNSIGNED_BYTE;
            return;
        case resources::TextureFormat::RGBA8:
            internalFormat = GL_RGBA8;
            externalFormat = GL_RGBA;
            dataType = GL_UNSIGNED_BYTE;
            return;
        case resources::TextureFormat::R32F:
            internalFormat = GL_R32F;
            externalFormat = GL_RED;
            dataType = GL_FLOAT;
            return;
        case resources::TextureFormat::RGB32F:
            internalFormat = GL_RGB32F;
            externalFormat = GL_RGB;
            dataType = GL_FLOAT;
            return;
        case resources::TextureFormat::RGBA32F:
            internalFormat = GL_RGBA32F;
            externalFormat = GL_RGBA;
            dataType = GL_FLOAT;
            return;
        default:
            break;
        }

        throw std::runtime_error("Unsupported texture format");
    }
}

void ResourceGpuUploader::Upload(resources::Resource *resource)
{
    if (!resource)
    {
        return;
    }

    if (auto *model = dynamic_cast<resources::Model *>(resource))
    {
        UploadModel(*model);
        return;
    }

    if (auto *shader = dynamic_cast<resources::Shader *>(resource))
    {
        UploadShader(*shader);
        return;
    }

    if (auto *texture = dynamic_cast<resources::Texture *>(resource))
    {
        UploadTexture(*texture);
        return;
    }
}

void ResourceGpuUploader::Release(resources::Resource *resource)
{
    if (!resource)
    {
        return;
    }

    if (auto *model = dynamic_cast<resources::Model *>(resource))
    {
        ReleaseModel(*model);
        return;
    }

    if (auto *shader = dynamic_cast<resources::Shader *>(resource))
    {
        ReleaseShader(*shader);
        return;
    }

    if (auto *texture = dynamic_cast<resources::Texture *>(resource))
    {
        ReleaseTexture(*texture);
        return;
    }
}

void ResourceGpuUploader::UploadModel(resources::Model &model)
{
    UploadMesh(model.GetMesh());
}

void ResourceGpuUploader::UploadMesh(resources::Mesh &mesh)
{
    if (mesh.IsGpuReady() || mesh.IsEmpty())
    {
        return;
    }

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(mesh.GetVertices().size() * sizeof(resources::MeshVertex)),
        mesh.GetVertices().data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(mesh.GetIndices().size() * sizeof(uint32_t)),
        mesh.GetIndices().data(),
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(resources::MeshVertex), reinterpret_cast<const void *>(offsetof(resources::MeshVertex, position)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(resources::MeshVertex), reinterpret_cast<const void *>(offsetof(resources::MeshVertex, normal)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(resources::MeshVertex), reinterpret_cast<const void *>(offsetof(resources::MeshVertex, uv0)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(resources::MeshVertex), reinterpret_cast<const void *>(offsetof(resources::MeshVertex, tangent)));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(resources::MeshVertex), reinterpret_cast<const void *>(offsetof(resources::MeshVertex, bitangent)));

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, sizeof(resources::MeshVertex), reinterpret_cast<const void *>(offsetof(resources::MeshVertex, uv1)));

    glEnableVertexAttribArray(6);
    glVertexAttribIPointer(6, 4, GL_UNSIGNED_INT, sizeof(resources::MeshVertex), reinterpret_cast<const void *>(offsetof(resources::MeshVertex, boneIndices)));

    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(resources::MeshVertex), reinterpret_cast<const void *>(offsetof(resources::MeshVertex, boneWeights)));

    glBindVertexArray(0);

    mesh.SetGpuHandles(vao, vbo, ebo);
}

void ResourceGpuUploader::UploadShader(resources::Shader &shader)
{
    if (shader.IsGpuReady() || shader.GetSources().empty())
    {
        return;
    }

    const GLuint programId = glCreateProgram();
    std::vector<GLuint> stageIds;
    stageIds.reserve(shader.GetSources().size());

    try
    {
        for (const resources::ShaderSource &source : shader.GetSources())
        {
            const GLuint stageId = CompileShaderStage(source.stage, source.code);
            glAttachShader(programId, stageId);
            stageIds.push_back(stageId);
        }

        glLinkProgram(programId);
        ThrowIfProgramLinkFailed(programId);

        for (GLuint stageId : stageIds)
        {
            glDetachShader(programId, stageId);
            glDeleteShader(stageId);
        }

        shader.SetProgramId(programId);
    }
    catch (const std::exception &)
    {
        for (GLuint stageId : stageIds)
        {
            glDeleteShader(stageId);
        }
        glDeleteProgram(programId);
        throw;
    }
}

void ResourceGpuUploader::UploadTexture(resources::Texture &texture)
{
    if (texture.IsGpuReady())
    {
        return;
    }

    if (texture.IsCubemap())
    {
        const std::array<std::string, 6> &facePaths = texture.GetCubemapFacePaths();
        for (const std::string &facePath : facePaths)
        {
            if (facePath.empty())
            {
                throw std::runtime_error("Cubemap texture is missing one or more face paths");
            }
        }

        GLuint textureId = 0;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);

        int width = 0;
        int height = 0;
        int channels = 0;

        for (size_t faceIndex = 0; faceIndex < 6; ++faceIndex)
        {
            const std::string &facePath = facePaths[faceIndex];
            stbi_uc *pixelData = stbi_load(facePath.c_str(), &width, &height, &channels, 0);
            if (!pixelData)
            {
                glDeleteTextures(1, &textureId);
                throw std::runtime_error("Failed to load cubemap face texture at path: " + facePath);
            }

            resources::TextureFormat faceFormat = resources::TextureFormat::Unknown;
            switch (channels)
            {
            case 1:
                faceFormat = resources::TextureFormat::R8;
                break;
            case 2:
                faceFormat = resources::TextureFormat::RG8;
                break;
            case 3:
                faceFormat = resources::TextureFormat::RGB8;
                break;
            case 4:
                faceFormat = resources::TextureFormat::RGBA8;
                break;
            default:
                stbi_image_free(pixelData);
                glDeleteTextures(1, &textureId);
                throw std::runtime_error("Unsupported cubemap face channel layout at path: " + facePath);
            }

            GLenum internalFormat = GL_RGBA8;
            GLenum externalFormat = GL_RGBA;
            GLenum dataType = GL_UNSIGNED_BYTE;
            ResolveTextureFormat(faceFormat, internalFormat, externalFormat, dataType);

            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(faceIndex),
                0,
                static_cast<GLint>(internalFormat),
                width,
                height,
                0,
                externalFormat,
                dataType,
                pixelData);

            stbi_image_free(pixelData);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        texture.SetTextureId(textureId);
        return;
    }

    if (texture.IsEmpty())
    {
        return;
    }

    GLenum internalFormat = GL_RGBA8;
    GLenum externalFormat = GL_RGBA;
    GLenum dataType = GL_UNSIGNED_BYTE;
    ResolveTextureFormat(texture.GetFormat(), internalFormat, externalFormat, dataType);

    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        static_cast<GLint>(internalFormat),
        texture.GetWidth(),
        texture.GetHeight(),
        0,
        externalFormat,
        dataType,
        texture.GetPixels().data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    texture.SetTextureId(textureId);
}

uint32_t ResourceGpuUploader::CompileShaderStage(resources::ShaderStage stage, const std::string &source)
{
    const GLenum shaderType = ToGlShaderType(stage);
    const GLuint shaderId = glCreateShader(shaderType);

    const GLchar *sourceCode = source.c_str();
    glShaderSource(shaderId, 1, &sourceCode, nullptr);
    glCompileShader(shaderId);
    ThrowIfShaderCompileFailed(shaderId);

    return shaderId;
}

void ResourceGpuUploader::ReleaseMesh(resources::Mesh &mesh)
{
    GLuint vao = static_cast<GLuint>(mesh.GetVao());
    GLuint vbo = static_cast<GLuint>(mesh.GetVbo());
    GLuint ebo = static_cast<GLuint>(mesh.GetEbo());

    if (vao != 0)
    {
        glDeleteVertexArrays(1, &vao);
    }
    if (vbo != 0)
    {
        glDeleteBuffers(1, &vbo);
    }
    if (ebo != 0)
    {
        glDeleteBuffers(1, &ebo);
    }

    mesh.SetGpuHandles(0, 0, 0);
}

void ResourceGpuUploader::ReleaseModel(resources::Model &model)
{
    ReleaseMesh(model.GetMesh());
}

void ResourceGpuUploader::ReleaseShader(resources::Shader &shader)
{
    GLuint programId = static_cast<GLuint>(shader.GetProgramId());
    if (programId != 0)
    {
        glDeleteProgram(programId);
    }

    shader.SetProgramId(0);
}

void ResourceGpuUploader::ReleaseTexture(resources::Texture &texture)
{
    GLuint textureId = static_cast<GLuint>(texture.GetTextureId());
    if (textureId != 0)
    {
        glDeleteTextures(1, &textureId);
    }

    texture.SetTextureId(0);
}
