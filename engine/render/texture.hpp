#pragma once

#include <string>
#include <print>

#include <glad/glad.h>

class Texture
{
public:
    Texture();
    Texture(const std::string &iFilePath);
    Texture(const unsigned char *iData, int iWidth, int iHeight);

    ~Texture();

    void Bind() const;
    void Unbind() const;
    void AddData(const unsigned char *iData, int iWidth, int iHeight);

private:
    GLuint m_id;
    int m_width, m_height;
    unsigned int m_internal_format, m_image_format;
    unsigned int m_wrap_s, m_wrap_t;
    unsigned int m_filter_min, m_filter_mag;
};