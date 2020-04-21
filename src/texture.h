#pragma once

#include "math.h"
#include "gl.h"
#include "smallvec.h"
#include "datafile.h"
#include <stb_image.h>

namespace TextureFilter
{
    enum
    {
        NEAREST = 0,
        BILINEAR = 1,
        TRILINEAR = 2,
        MAX
    };
}

struct Texture
{
    // serialized
    i64 guid = 0;
    std::string name;
    std::string sourceFile;
    bool usedForDecal = false;
    bool usedForBillboard = false;
    bool srgb = true;
    bool repeat = true;
    bool generateMipMaps = true;
    f32 lodBias = -0.2f;
    i32 anisotropy = 8;

    i32 filter = TextureFilter::TRILINEAR;

    void serialize(Serializer& s)
    {
        s.field(guid);
        s.field(name);
        s.field(sourceFile);
        s.field(usedForBillboard);
        s.field(usedForBillboard);
        s.field(srgb);
        s.field(repeat);
        s.field(generateMipMaps);
        s.field(lodBias);
        s.field(anisotropy);
        s.field(filter);
    }

    // not serialized
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
    Texture(const char* name, u32 width, u32 height, u8* data, Format format = Format::SRGBA8)
        : name(name), format(format), width(width), height(height)
    {
        initGLTexture(data);
    }

    Texture(const char* filename, bool repeat=true, Format format = Format::SRGBA8)
        : name(filename), format(format)
    {
        i32 width, height, channels;
        u8* data = (u8*)stbi_load(filename, &width, &height, &channels, 4);
        if (!data)
        {
            error("Failed to load image: ", filename, " (", stbi_failure_reason(), ")\n");
        }
        this->width = (u32)width;
        this->height = (u32)height;
        initGLTexture(data, repeat);
        stbi_image_free(data);
    }

    void reload()
    {
    }

    void initGLTexture(u8* data, bool repeat=true)
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
                internalFormat = GL_R8;
                //internalFormat = GL_SR8_EXT;
                baseFormat = GL_RED;
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                break;
        }

        u32 mipLevels = 1 + (u32)(glm::log2((f32)glm::max(width, height)));

        glCreateTextures(GL_TEXTURE_2D, 1, &handle);
        glTextureStorage2D(handle, mipLevels, internalFormat, width, height);
        glTextureSubImage2D(handle, 0, 0, 0, width, height, baseFormat, GL_UNSIGNED_BYTE, data);
        if (repeat)
        {
            glTextureParameteri(handle, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTextureParameteri(handle, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        else
        {
            glTextureParameteri(handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        glTextureParameteri(handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(handle, GL_TEXTURE_MAX_ANISOTROPY, 8);
        glTextureParameterf(handle, GL_TEXTURE_LOD_BIAS, -0.2f);
        glGenerateTextureMipmap(handle);

#ifndef NDEBUG
        glObjectLabel(GL_TEXTURE, handle, name.length(), name.c_str());
#endif
    }

    Texture(SmallVec<const char*> faces)
    {
        assert(faces.size() == 6);

        u8* faceData[6];
        for (u32 i=0; i<6; ++i)
        {
            i32 width, height, channels;
            faceData[i] = (u8*)stbi_load(faces[i], &width, &height, &channels, 4);
            if (!faceData[i])
            {
                error("Failed to load image: ", faces[i], " (", stbi_failure_reason(), ")\n");
            }
            this->width = (u32)width;
            this->height = (u32)height;
        }

        u32 mipLevels = 1 + (u32)(glm::log2((f32)glm::max(width, height)));
        GLuint internalFormat = GL_SRGB8;
        GLuint baseFormat = GL_RGBA;

        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &handle);
        glTextureStorage2D(handle, mipLevels, internalFormat, width, height);
        for (u32 i=0; i<6; ++i)
        {
            glTextureSubImage3D(handle, 0, 0, 0, i, width, height, 1,
                    baseFormat, GL_UNSIGNED_BYTE, faceData[i]);
            stbi_image_free(faceData[i]);
        }
        glTextureParameteri(handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(handle, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(handle, GL_TEXTURE_MAX_ANISOTROPY, 8);
        glGenerateTextureMipmap(handle);
    }

    ~Texture()
    {
        // TODO: cleanup
    }
};

