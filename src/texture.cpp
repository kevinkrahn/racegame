#include "texture.h"
#include "resources.h"

void Texture::loadSourceFile(u32 index)
{
    i32 w, h, channels;
    const char* fullPath = tmpStr("%s/%s", ASSET_DIRECTORY, sourceFiles[index].path.cstr);
    u8* data = (u8*)stbi_load(fullPath, &w, &h, &channels, 4);
    if (!data)
    {
        error("Failed to load image: %s (%s)", fullPath, stbi_failure_reason());
    }
    this->width = w;
    this->height = h;
    sourceFiles[index].width = width;
    sourceFiles[index].height = height;
    sourceFiles[index].data.assign(data, data + width * height * 4);
    stbi_image_free(data);
}

void Texture::reloadSourceFiles()
{
    for (u32 i=0; i<sourceFiles.size(); ++i)
    {
        loadSourceFile(i);
    }
    regenerate();
}

void Texture::destroy()
{
    for (u32 i=0; i<sourceFiles.size(); ++i)
    {
        if (sourceFiles[i].previewHandle)
        {
            glDeleteTextures(1, &sourceFiles[i].previewHandle);
            sourceFiles[i].previewHandle = 0;
        }
    }
    if (cubemapHandle)
    {
        glDeleteTextures(1, &cubemapHandle);
        cubemapHandle = 0;
    }
    handle = 0;
}

void Texture::setTextureType(u32 textureType)
{
    destroy();
    this->textureType = textureType;
    if (textureType == TextureType::CUBE_MAP)
    {
        sourceFiles.resize(6);
    }
    else
    {
        sourceFiles.resize(1);
    }
}

void Texture::setSourceFile(u32 index, const char* path)
{
    assert(index < sourceFiles.size());
    sourceFiles[index].path = path;
    loadSourceFile(index);
}

void Texture::regenerate()
{
    destroy();
    if (!sourceFiles.empty())
    {
        for (u32 i=0; i<sourceFiles.size(); ++i)
        {
            initGLTexture(i);
        }
        width = sourceFiles[0].width;
        height = sourceFiles[0].height;
        if (textureType == TextureType::CUBE_MAP)
        {
            initCubemap();
            handle = cubemapHandle;
        }
        else
        {
            handle = sourceFiles[0].previewHandle;
        }
    }
}

void Texture::initGLTexture(u32 index)
{
    if (sourceFiles[index].data.empty())
    {
        return;
    }

    GLuint internalFormat, baseFormat;
    switch (textureType)
    {
        case TextureType::NORMAL_MAP:
            internalFormat = GL_RGBA8;
            baseFormat = GL_RGBA;
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            break;
        case TextureType::GRAYSCALE:
            internalFormat = GL_R8;
            //internalFormat = GL_SR8_EXT;
            baseFormat = GL_RED;
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            break;
        case TextureType::COLOR:
        default:
            internalFormat = GL_SRGB8_ALPHA8;
            baseFormat = GL_RGBA;
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            break;
    }

    SourceFile& s = sourceFiles[index];
    u32 mipLevels = generateMipMaps ? 1 + (u32)(log2f((f32)max(s.width, s.height))) : 1;

    glCreateTextures(GL_TEXTURE_2D, 1, &s.previewHandle);
    glTextureStorage2D(s.previewHandle, mipLevels, internalFormat, s.width, s.height);
    glTextureSubImage2D(s.previewHandle, 0, 0, 0, s.width, s.height, baseFormat,
            GL_UNSIGNED_BYTE, s.data.data());
    if (repeat)
    {
        glTextureParameteri(s.previewHandle, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(s.previewHandle, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    else
    {
        glTextureParameteri(s.previewHandle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(s.previewHandle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    if (filter == TextureFilter::NEAREST)
    {
        glTextureParameteri(s.previewHandle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(s.previewHandle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    else if (filter == TextureFilter::BILINEAR)
    {
        glTextureParameteri(s.previewHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(s.previewHandle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else if (filter == TextureFilter::TRILINEAR)
    {
        glTextureParameteri(s.previewHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(s.previewHandle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    glTextureParameterf(s.previewHandle, GL_TEXTURE_MAX_ANISOTROPY, max((f32)anisotropy, 1.f));
    glTextureParameterf(s.previewHandle, GL_TEXTURE_LOD_BIAS, lodBias);
    if (generateMipMaps)
    {
        glGenerateTextureMipmap(s.previewHandle);
    }

#ifndef NDEBUG
    glObjectLabel(GL_TEXTURE, s.previewHandle, name.size(), name.cstr);
#endif
}

void Texture::initCubemap()
{
    for (u32 i=0; i<6; ++i)
    {
        if (sourceFiles[i].data.empty())
        {
            return;
        }
    }

    u32 mipLevels = generateMipMaps ? 1 + (u32)(log2f((f32)max(width, height))) : 1;
    GLuint internalFormat = GL_SRGB8;
    GLuint baseFormat = GL_RGBA;

    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &cubemapHandle);
    glTextureStorage2D(cubemapHandle, mipLevels, internalFormat, width, height);
    for (u32 i=0; i<6; ++i)
    {
        glTextureSubImage3D(cubemapHandle, 0, 0, 0, i, width, height, 1,
                baseFormat, GL_UNSIGNED_BYTE, sourceFiles[i].data.data());
    }
    glTextureParameteri(cubemapHandle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(cubemapHandle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(cubemapHandle, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTextureParameteri(cubemapHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(cubemapHandle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(cubemapHandle, GL_TEXTURE_MAX_ANISOTROPY, anisotropy);
    if (generateMipMaps)
    {
        glGenerateTextureMipmap(cubemapHandle);
    }
}
