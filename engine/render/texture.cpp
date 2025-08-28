#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include <stb_image.h>

#include "texture.hpp"

Texture::Texture() : m_id(0), m_width(0), m_height(0),
                     m_internal_format(GL_RGB), m_image_format(GL_RGB),
                     m_wrap_s(GL_REPEAT), m_wrap_t(GL_REPEAT),
                     m_filter_min(GL_NEAREST), m_filter_mag(GL_LINEAR)
{
    glGenTextures(1, &m_id);
}

Texture::Texture(const std::string &iFilePath) : m_id(0), m_width(0), m_height(0),
                                                 m_internal_format(GL_RGB), m_image_format(GL_RGB),
                                                 m_wrap_s(GL_REPEAT), m_wrap_t(GL_REPEAT),
                                                 m_filter_min(GL_NEAREST), m_filter_mag(GL_LINEAR)
{
    glGenTextures(1, &m_id);

    stbi_set_flip_vertically_on_load(true);

    int aWidth, aHeight, _;
    unsigned char *aData = stbi_load(iFilePath.c_str(), &aWidth, &aHeight, &_, 0);
    if (aData == nullptr)
    {
        ERROR("Failed to load texture, param: " << iFilePath);
        return;
    }

    Bind();
    AddData(aData, aWidth, aHeight);
    Unbind();

    stbi_image_free(aData);
}

Texture::Texture(const unsigned char *iData, int iWidth, int iHeight)
{
    glGenTextures(1, &m_id);
    Bind();
    AddData(iData, iWidth, iHeight);
    Unbind();
}

Texture::~Texture()
{
    glDeleteTextures(1, &m_id);
}

void Texture::Bind() const
{
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::Unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::AddData(const unsigned char *iData, int iWidth, int iHeight)
{
    m_width = iWidth;
    m_height = iHeight;

    glTexImage2D(GL_TEXTURE_2D, 0, m_internal_format, m_width, m_height, 0, m_image_format, GL_UNSIGNED_BYTE, iData);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, m_wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, m_wrap_t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_filter_min);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_filter_mag);
}
