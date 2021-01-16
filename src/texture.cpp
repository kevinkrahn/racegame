#include "texture.h"
#include "resources.h"
#include "game.h"

#include <stb_image.h>
#include <stb_dxt.h>

static Array<u8> compressDXT(u32* data, i32 w, i32 h, bool alpha)
{
    u32 block[16];
    u32 compressedBlockSize = alpha ? 16 : 8;
    Array<u8> outData((w * h) / 16 * compressedBlockSize);
    u8* out = outData.data();
    for (i32 j=0; j<h; j+=4)
    {
        for (i32 i=0; i<w; i+=4)
        {
            i32 offset = j * w + i;
            u32* srcData = data + offset;
            for (i32 row=0; row<4; ++row)
            {
                for (i32 col=0; col<4; ++col)
                {
                    block[row * 4 + col] = *(srcData + w * row + col);
                }
            }
            stb_compress_dxt_block(out, (u8*)block, alpha, STB_DXT_HIGHQUAL);
            out += compressedBlockSize;
        }
    }
    return outData;
}

static Array<u8> compressBC4(u8* data, i32 w, i32 h)
{
    u8 block[16];
    const u32 compressedBlockSize = 8;
    Array<u8> outData((w * h) / 16 * compressedBlockSize);
    u8* out = outData.data();
    for (i32 j=0; j<h; j+=4)
    {
        for (i32 i=0; i<w; i+=4)
        {
            i32 offset = j * w + i;
            u8* srcData = data + offset;
            for (i32 row=0; row<4; ++row)
            {
                for (i32 col=0; col<4; ++col)
                {
                    block[row * 4 + col] = *(srcData + w * row + col);
                }
            }
            stb_compress_bc4_block(out, (u8*)block);
            out += compressedBlockSize;
        }
    }
    return outData;
}

static Array<u8> compressBC5(u16* data, i32 w, i32 h)
{
    u16 block[16];
    const u32 compressedBlockSize = 16;
    Array<u8> outData((w * h) / 16 * compressedBlockSize);
    u8* out = outData.data();
    for (i32 j=0; j<h; j+=4)
    {
        for (i32 i=0; i<w; i+=4)
        {
            i32 offset = j * w + i;
            u16* srcData = data + offset;
            for (i32 row=0; row<4; ++row)
            {
                for (i32 col=0; col<4; ++col)
                {
                    block[row * 4 + col] = *(srcData + w * row + col);
                }
            }
            stb_compress_bc5_block(out, (u8*)block);
            out += compressedBlockSize;
        }
    }
    return outData;
}

bool Texture::loadSourceFile(u32 index)
{
    i32 w, h;
    i32 channels = 4;
    if (textureType == TextureType::GRAYSCALE)
    {
        channels = 1;
    }
    const char* fullPath = tmpStr("%s/%s", ASSET_DIRECTORY, sourceFiles[index].path.data());
    u8* data = (u8*)stbi_load(fullPath, &w, &h, &channels, channels);
    if (!data)
    {
        error("Failed to load image: %s (%s)", fullPath, stbi_failure_reason());
        return false;
    }
    this->width = (u32)w;
    this->height = (u32)h;
    sourceFiles[index].width = width;
    sourceFiles[index].height = height;
    Array<u8> sourceData;

    // convert RGBA to RG
    if (textureType == TextureType::NORMAL_MAP)
    {
        channels = 2;
        sourceData.resize(w * h * channels);
        for (i32 j=0; j<h; ++j)
        {
            for (i32 i=0; i<w; ++i)
            {
                i32 offset = j * w + i;
                *((u16*)sourceData.data() + offset) = *(u16*)((u32*)data + offset);
            }
        }
    }
    else
    {
        sourceData.assign(data, data + w * h * channels);
    }
    stbi_image_free(data);

    if (compressed)
    {
        switch(textureType)
        {
            case TextureType::COLOR:
            case TextureType::CUBE_MAP:
                sourceFiles[index].data = move(
                    compressDXT((u32*)sourceData.data(), w, h, preserveAlpha && textureType == TextureType::COLOR));
                break;
            case TextureType::GRAYSCALE:
                sourceFiles[index].data = move(compressBC4(sourceData.data(), w, h));
                break;
            case TextureType::NORMAL_MAP:
                sourceFiles[index].data = move(compressBC5((u16*)sourceData.data(), w, h));
                break;
        }
    }
    else
    {
        sourceFiles[index].data = move(sourceData);
    }
    return true;
}

bool Texture::sourceFilesExist()
{
    for (auto& s : sourceFiles)
    {
        const char* fullPath = tmpStr("%s/%s", ASSET_DIRECTORY, s.path.data());
        SDL_RWops* file = SDL_RWFromFile(fullPath, "r+b");
        if (!file)
        {
            error("Texture source file does not exist: %s", fullPath);
            return false;
        }
        SDL_RWclose(file);
    }
    return true;
}

bool Texture::reloadSourceFiles()
{
    for (u32 i=0; i<sourceFiles.size(); ++i)
    {
        if (!loadSourceFile(i))
        {
            return false;
        }
    }
    regenerate();
    return true;
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

    u32 unpackAlignment = 4;
    GLuint internalFormat, baseFormat;
    switch (textureType)
    {
        case TextureType::NORMAL_MAP:
            internalFormat = compressed ? GL_COMPRESSED_RG_RGTC2 : GL_RG8;
            baseFormat = compressed ? internalFormat : GL_RG;
            unpackAlignment = 2;
            break;
        case TextureType::GRAYSCALE:
            // NOTE: Compressed grayscale textures are always in linear color space
            internalFormat = compressed
                ? GL_COMPRESSED_RED_RGTC1 : (srgbSourceData ? GL_SR8_EXT : GL_R8);
            baseFormat = compressed ? internalFormat : GL_RED;
            unpackAlignment = 1;
            break;
        case TextureType::CUBE_MAP:
            internalFormat = compressed ? GL_COMPRESSED_SRGB_S3TC_DXT1_EXT : GL_SRGB8;
            baseFormat = compressed ? internalFormat : GL_RGBA;
            break;
        case TextureType::COLOR:
            internalFormat = compressed
                ? (preserveAlpha ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_SRGB_S3TC_DXT1_EXT)
                : (preserveAlpha ? GL_SRGB8_ALPHA8 : GL_SRGB8);
            baseFormat = compressed ? internalFormat : GL_RGBA;
            break;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);

    SourceFile& s = sourceFiles[index];
    u32 mipLevels = generateMipMaps ? 1 + (u32)(log2f((f32)max(s.width, s.height))) : 1;
    glCreateTextures(GL_TEXTURE_2D, 1, &s.previewHandle);
    glTextureStorage2D(s.previewHandle, mipLevels, internalFormat, s.width, s.height);
    if (compressed)
    {
        int v;
        glGetTextureLevelParameteriv(s.previewHandle, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &v);
        assert(s.data.size() == v);
        glCompressedTextureSubImage2D(s.previewHandle, 0, 0, 0, s.width, s.height, baseFormat,
                s.data.size(), s.data.data());
    }
    else
    {
        glTextureSubImage2D(s.previewHandle, 0, 0, 0, s.width, s.height, baseFormat,
                GL_UNSIGNED_BYTE, s.data.data());
    }

    if (textureType == TextureType::GRAYSCALE)
    {
        GLint swizzle[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
        glTextureParameteriv(s.previewHandle, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
    }
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
    glTextureParameterf(s.previewHandle, GL_TEXTURE_MAX_ANISOTROPY, min(max((f32)anisotropy, 1.f),
                max((f32)g_game.config.graphics.anisotropicFilteringLevel, 1.f)));
    glTextureParameterf(s.previewHandle, GL_TEXTURE_LOD_BIAS, lodBias);
    if (generateMipMaps)
    {
        glGenerateTextureMipmap(s.previewHandle);
    }

#ifndef NDEBUG
    glObjectLabel(GL_TEXTURE, s.previewHandle, name.size(), name.data());
#endif
}

void Texture::onUpdateGlobalTextureSettings()
{
    for (auto& s : sourceFiles)
    {
        glTextureParameterf(s.previewHandle, GL_TEXTURE_MAX_ANISOTROPY, min(max((f32)anisotropy, 1.f),
                max((f32)g_game.config.graphics.anisotropicFilteringLevel, 1.f)));
    }
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
    GLuint internalFormat = compressed ? GL_COMPRESSED_SRGB_S3TC_DXT1_EXT : GL_SRGB8;
    GLuint baseFormat = compressed ? internalFormat : GL_RGBA;

#define DSA 0
#if DSA
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &cubemapHandle);
    glTextureStorage2D(cubemapHandle, mipLevels, internalFormat, width, height);
#else
    glGenTextures(1, &cubemapHandle);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapHandle);
#endif
    for (u32 i=0; i<6; ++i)
    {
        if (compressed)
        {
#if DSA
            int v;
            glGetTextureLevelParameteriv(cubemapHandle, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &v);
            assert(sourceFiles[i].data.size() == v);
            // TODO: find out why DSA doesn't work for compressed cubemaps
            glCompressedTextureSubImage3D(cubemapHandle, 0, 0, 0, i, width, height, 1,
                    baseFormat, sourceFiles[i].data.size(), sourceFiles[i].data.data());
#else
            glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height,
                    0, sourceFiles[i].data.size(), sourceFiles[i].data.data());
#endif
        }
        else
        {
#if DSA
            glTextureSubImage3D(cubemapHandle, 0, 0, 0, i, width, height, 1,
                    baseFormat, GL_UNSIGNED_BYTE, sourceFiles[i].data.data());
#else
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height,
                    0, baseFormat, GL_UNSIGNED_BYTE, sourceFiles[i].data.data());
#endif
        }
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
