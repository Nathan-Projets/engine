#include "shader.hpp"

Shader::Shader(const std::string &iVertexFilePath, const std::string &iFragmentFilePath)
{
    GLuint aVertexShader = compile(ShaderType::Vertex, tools::LoadFile(iVertexFilePath));
    GLuint aFragmentShader = compile(ShaderType::Fragment, tools::LoadFile(iFragmentFilePath));
    if (aVertexShader == 0 || aFragmentShader == 0)
    {
        if (aVertexShader != 0)
            glDeleteShader(aVertexShader);
        if (aFragmentShader != 0)
            glDeleteShader(aFragmentShader);
        return;
    }

    createProgramShader(aVertexShader, aFragmentShader);
}

Shader::~Shader()
{
    if (m_id != 0)
        glDeleteProgram(m_id);
}

void Shader::Use() const
{
    glUseProgram(m_id);
}

GLint Shader::GetLocation(const std::string &name) const
{
    auto it = m_locationCache.find(name);
    if (it != m_locationCache.end())
        return it->second;
    GLint loc = glGetUniformLocation(m_id, name.c_str());
    m_locationCache[name] = loc;
    return loc;
}

GLuint Shader::compile(ShaderType iType, const std::string &iShaderData)
{
    if (iShaderData.size() == 0)
    {
        ERROR("Shader not compiled, empty shader.");
        return 0;
    }

    const char *aData = iShaderData.data();
    unsigned int aShader = glCreateShader(ShaderType::Fragment == iType ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER);
    glShaderSource(aShader, 1, &aData, NULL);
    glCompileShader(aShader);

    if (!checkError(iType, aShader))
    {
        glDeleteShader(aShader);
        return 0;
    }

    return aShader;
}

void Shader::createProgramShader(GLuint iVertexShader, GLuint iFragmentShader)
{
    m_id = glCreateProgram();

    glAttachShader(m_id, iVertexShader);
    glAttachShader(m_id, iFragmentShader);
    glLinkProgram(m_id);

    if (!checkError(ShaderType::Program, m_id))
    {
        glDeleteProgram(m_id);
        m_id = 0;
    }

    glDeleteShader(iVertexShader);
    glDeleteShader(iFragmentShader);
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
    glUniform1i(GetLocation(iName), iValue);
}

void Shader::UploadUniformInt(const std::string &iName, const glm::uint &iValue)
{
    glUniform1i(GetLocation(iName), iValue);
}

void Shader::UploadUniformFloat1(const std::string &iName, const float &iVector)
{
    glUniform1f(GetLocation(iName), iVector);
}

void Shader::UploadUniformFloat2(const std::string &iName, const glm::vec2 &iVector)
{
    glUniform2f(GetLocation(iName), iVector[0], iVector[1]);
}

void Shader::UploadUniformFloat3(const std::string &iName, const glm::vec3 &iVector)
{
    glUniform3f(GetLocation(iName), iVector[0], iVector[1], iVector[2]);
}

void Shader::UploadUniformFloat4(const std::string &iName, const glm::vec4 &iVector)
{
    glUniform4f(GetLocation(iName), iVector[0], iVector[1], iVector[2], iVector[3]);
}

void Shader::UploadUniformMatrixFloat4(const std::string &iName, const glm::mat4 &iVector)
{
    glUniformMatrix4fv(GetLocation(iName), 1, GL_FALSE, value_ptr(iVector));
}