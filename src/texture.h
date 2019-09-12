#pragma once

#include "math.h"
#include "gl.h"

struct Texture
{
    enum struct Format
    {
        SRGBA8,
        RGBA8,
        R8,
    };
    Format format = Format::SRGBA8;
    u32 width = 0;
    u32 height = 0;

    GLuint handle = 0;

    Texture() {}
    Texture(u32 width, u32 height, Format format, u8* data, size_t size)
        : width(width), height(height), format(format)
    {
        GLuint internalFormat, baseFormat;
        switch (format)
        {
            case Texture::Format::SRGBA8:
                internalFormat = GL_SRGB8_ALPHA8;
                baseFormat = GL_RGBA;
                glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
                break;
            case Texture::Format::RGBA8:
                internalFormat = GL_RGBA8;
                baseFormat = GL_RGBA;
                glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
                break;
            case Texture::Format::R8:
                //internalFormat = GL_R8;
                internalFormat = GL_SR8_EXT;
                baseFormat = GL_RED;
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                break;
        }

        u32 mipLevels = 1 + glm::floor(glm::log2((f32)glm::max(width, height)));

        glCreateTextures(GL_TEXTURE_2D, 1, &handle);
        glTextureStorage2D(handle, mipLevels, internalFormat, width, height);
        glTextureSubImage2D(handle, 0, 0, 0, width, height, baseFormat, GL_UNSIGNED_BYTE, data);
        glTextureParameteri(handle, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(handle, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTextureParameteri(handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(handle, GL_TEXTURE_MAX_ANISOTROPY, 8);
        glTextureParameterf(handle, GL_TEXTURE_LOD_BIAS, -0.2f);
        glGenerateTextureMipmap(handle);
    }

    ~Texture()
    {
        // TODO: cleanup
    }
};

