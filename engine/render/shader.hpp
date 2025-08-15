#pragma once

#include <print>
#include <string>
#include <format>
#include <type_traits>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "resources/fileloader.hpp"

enum class ShaderType
{
    Fragment,
    Vertex,
    Program
};

class Shader
{
public:
    Shader() = delete;
    Shader(const std::string &vertexFilePath, const std::string &fragmentFilePath);
    ~Shader();

    void UploadUniformBool(const std::string &name, const glm::uint &value);
    void UploadUniformInt(const std::string &name, const glm::uint &value);
    void UploadUniformFloat1(const std::string &name, const float &vector);
    void UploadUniformFloat2(const std::string &name, const glm::vec2 &vector);
    void UploadUniformFloat3(const std::string &name, const glm::vec3 &vector);
    void UploadUniformFloat4(const std::string &name, const glm::vec4 &vector);
    void UploadUniformMatrixFloat4(const std::string &name, const glm::mat4 &vector);

    void Use() const;

    template <typename TYPE>
    inline void Upload(const std::string &name, const TYPE &value)
    {
        if constexpr (std::is_same_v<TYPE, glm::uint> || std::is_same_v<TYPE, int>)
        {
            UploadUniformInt(name, value);
        }
        else if constexpr (std::is_same_v<TYPE, float>)
        {
            UploadUniformFloat1(name, value);
        }
        else if constexpr (std::is_same_v<TYPE, glm::vec2>)
        {
            UploadUniformFloat2(name, value);
        }
        else if constexpr (std::is_same_v<TYPE, glm::vec3>)
        {
            UploadUniformFloat3(name, value);
        }
        else if constexpr (std::is_same_v<TYPE, glm::vec4>)
        {
            UploadUniformFloat4(name, value);
        }
        else if constexpr (std::is_same_v<TYPE, glm::mat4>)
        {
            UploadUniformMatrixFloat4(name, value);
        }
        else
        {
            throw std::invalid_argument("Unsupported type for shader upload");
        }
    }

private:
    GLuint compile(ShaderType iType, const std::string &shaderSource);
    void createProgramShader(GLuint iVertexShader, GLuint iFragmentShader);
    bool checkError(ShaderType iType, GLuint iShader);

    GLuint m_id;
};