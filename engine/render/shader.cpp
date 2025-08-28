#include "shader.hpp"

Shader::Shader(const std::string &iVertexFilePath, const std::string &iFragmentFilePath)
{
    GLuint aVertexShader = compile(ShaderType::Vertex, tools::LoadFile(iVertexFilePath));
    GLuint aFragmentShader = compile(ShaderType::Fragment, tools::LoadFile(iFragmentFilePath));
    if (aVertexShader == -1 || aFragmentShader == -1)
    {
        return;
    }

    createProgramShader(aVertexShader, aFragmentShader);
}

Shader::~Shader()
{
    glDeleteProgram(m_id);
}

void Shader::Use() const
{
    glUseProgram(m_id);
}

GLuint Shader::compile(ShaderType iType, const std::string &iShaderData)
{
    if (iShaderData.size() == 0)
    {
        ERROR("Shader not compiled, empty shader.");
        return -1;
    }

    const char *aData = iShaderData.data();
    unsigned int aShader = glCreateShader(ShaderType::Fragment == iType ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER);
    glShaderSource(aShader, 1, &aData, NULL);
    glCompileShader(aShader);

    return checkError(iType, aShader) ? aShader : -1;
}

void Shader::createProgramShader(GLuint iVertexShader, GLuint iFragmentShader)
{
    m_id = glCreateProgram();

    glAttachShader(m_id, iVertexShader);
    glAttachShader(m_id, iFragmentShader);
    glLinkProgram(m_id);

    if (checkError(ShaderType::Program, m_id))
    {
        glDeleteShader(iVertexShader);
        glDeleteShader(iFragmentShader);
    }
}

bool Shader::checkError(ShaderType iType, GLuint iShader)
{
    int aStatus;
    char aInfoLog[512];

    if (iType == ShaderType::Vertex || iType == ShaderType::Fragment)
    {
        glGetShaderiv(iShader, GL_COMPILE_STATUS, &aStatus);
    }
    else if (iType == ShaderType::Program)
    {
        glGetProgramiv(iShader, GL_LINK_STATUS, &aStatus);
    }

    if (!aStatus && iType == ShaderType::Vertex)
    {
        glGetShaderInfoLog(iShader, 512, NULL, aInfoLog);
        ERROR("Vertex shader not compiled, reason: " << aInfoLog);
    }
    else if (!aStatus && iType == ShaderType::Fragment)
    {
        glGetShaderInfoLog(iShader, 512, NULL, aInfoLog);
        ERROR("Fragment shader not compiled, reason: " << aInfoLog);
    }
    else if (!aStatus && iType == ShaderType::Program)
    {
        glGetProgramInfoLog(iShader, 512, NULL, aInfoLog);
        ERROR("Vertex and Fragment shaders not linked, reason: " << aInfoLog);
    }

    return aStatus;
}

void Shader::UploadUniformBool(const std::string &iName, const glm::uint &iValue)
{
    glUseProgram(m_id);
    glUniform1i(glGetUniformLocation(m_id, iName.c_str()), iValue);
}

void Shader::UploadUniformInt(const std::string &iName, const glm::uint &iValue)
{
    glUseProgram(m_id);
    glUniform1i(glGetUniformLocation(m_id, iName.c_str()), iValue);
}

void Shader::UploadUniformFloat1(const std::string &iName, const float &iVector)
{
    glUseProgram(m_id);
    glUniform1f(glGetUniformLocation(m_id, iName.c_str()), iVector);
}

void Shader::UploadUniformFloat2(const std::string &iName, const glm::vec2 &iVector)
{
    glUseProgram(m_id);
    glUniform2f(glGetUniformLocation(m_id, iName.c_str()), iVector[0], iVector[1]);
}

void Shader::UploadUniformFloat3(const std::string &iName, const glm::vec3 &iVector)
{
    glUseProgram(m_id);
    glUniform3f(glGetUniformLocation(m_id, iName.c_str()), iVector[0], iVector[1], iVector[2]);
}

void Shader::UploadUniformFloat4(const std::string &iName, const glm::vec4 &iVector)
{
    glUseProgram(m_id);
    glUniform4f(glGetUniformLocation(m_id, iName.c_str()), iVector[0], iVector[1], iVector[2], iVector[3]);
}

void Shader::UploadUniformMatrixFloat4(const std::string &iName, const glm::mat4 &iVector)
{
    glUseProgram(m_id);
    glUniformMatrix4fv(glGetUniformLocation(m_id, iName.c_str()), 1, GL_FALSE, value_ptr(iVector));
}