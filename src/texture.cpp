#include "texture.h"
#include "resources.h"
#include "game.h"

#include <stb_image.h>
#include <stb_image_resize.h>
#include <stb_dxt.h>

static Array<u8> compressDXT(u32* data, i32 w, i32 h, bool alpha)
{
    assert(w >= 4 && h >=4);
    assert(w % 4 == 0 && h % 4 == 0);

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
    assert(w >= 4 && h >=4);
    assert(w % 4 == 0 && h % 4 == 0);

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
    assert(w >= 4 && h >=4);
    assert(w % 4 == 0 && h % 4 == 0);

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
    i32 w, h, outChannels;
    i32 channels = 4;
    if (textureType == TextureType::GRAYSCALE)
    {
        channels = 1;
    }
    const char* fullPath = tmpStr("%s/%s", ASSET_DIRECTORY, sourceFiles[index].path.data());
    println("Loading image %s", fullPath);
    u8* data = (u8*)stbi_load(fullPath, &w, &h, &outChannels, channels);
    if (!data)
    {
        error("Failed to load image: %s (%s)", fullPath, stbi_failure_reason());
        return false;
    }
    if (width % 4 != 0 || height % 4 != 0)
    {
        stbi_image_free(data);
        showError("Image dimensions must be a multiple of 4");
        return false;
    }
    if (width < 4 || height < 4)
    {
        stbi_image_free(data);
        showError("Image width and height must be at least 4 pixels");
        return false;
    }

    this->width = (u32)w;
    this->height = (u32)h;
    sourceFiles[index].width = width;
    sourceFiles[index].height = height;

    // smallest mipmap dimension is 4 pixels
    u32 mipLevels = generateMipMaps ? (1 + (u32)max((i32)log2(min(width, height)) - 2, 0)) : 1;
    Array<Array<u8>> sourceData(mipLevels);
    sourceData[0].assign(data, data + w * h * channels);
    stbi_image_free(data);
    for (u32 level=1; level<mipLevels; ++level)
    {
        i32 sourceWidth = w >> (level - 1);
        i32 sourceHeight = h >> (level - 1);
        i32 outputWidth = w >> level;
        i32 outputHeight = h >> level;

        if (outputWidth % 4 != 0 || outputHeight % 4 != 0)
        {
            println("Mip #%i dimensions are not a multiple of 4 (%ix%i), so mip #%i will be the maximum mip level",
                    level, outputWidth, outputHeight, level-1);
            sourceData.resize(level);
            mipLevels = level;
            break;
        }

        sourceData[level].resize(outputWidth * outputHeight * channels);
        auto wrapMode = (repeat && textureType != TextureType::CUBE_MAP)
            ? STBIR_EDGE_WRAP : STBIR_EDGE_CLAMP;
        // TODO: Investigate other filter modes. Do mipmaps look better when more or less blurry?
        // TODO: Add option to choose downsampling algorithm.
        auto filter = STBIR_FILTER_DEFAULT;
        i32 flags = 0;

        println("Resizing mip #%i from %ix%i to %ix%i", level, sourceWidth, sourceHeight,
                outputWidth, outputHeight);

        if (textureType == TextureType::GRAYSCALE)
        {
            assert(channels == 1);
            if (!compressed && srgbSourceData)
            {
                if (!stbir_resize_uint8_generic(sourceData[level-1].data(), sourceWidth, sourceHeight,
                        0, sourceData[level].data(), outputWidth, outputHeight, 0, channels,
                        STBIR_ALPHA_CHANNEL_NONE, flags, wrapMode, filter,
                        STBIR_COLORSPACE_SRGB, nullptr))
                {
                    error("Image resize for 1-channel linear image failed");
                }
            }
            else
            {
                if (!stbir_resize_uint8_generic(sourceData[level-1].data(), sourceWidth, sourceHeight,
                        0, sourceData[level].data(), outputWidth, outputHeight, 0, channels,
                        STBIR_ALPHA_CHANNEL_NONE, flags, wrapMode, filter,
                        STBIR_COLORSPACE_LINEAR, nullptr))
                {
                    error("Image resize for 1-channel srgb image failed");
                }
            }
        }
        else
        {
            assert(channels == 4);
            i32 alphaChannel = 3;
            if (!stbir_resize_uint8_generic(sourceData[level-1].data(), sourceWidth, sourceHeight,
                    0, sourceData[level].data(), outputWidth, outputHeight, 0, channels,
                    alphaChannel, flags, wrapMode, filter, STBIR_COLORSPACE_SRGB, nullptr))
            {
                error("Image resize for 4-channel srgb image failed");
            }
        }
    }

    // convert RGBA to RG for normal maps
    if (textureType == TextureType::NORMAL_MAP)
    {
        channels = 2;
        for (u32 level=0; level<mipLevels; ++level)
        {
            i32 width = w >> level;
            i32 height = h >> level;
            Array<u8> rgData(width * height * channels);
            for (i32 j=0; j<height; ++j)
            {
                for (i32 i=0; i<width; ++i)
                {
                    i32 offset = j * width + i;
                    *((u16*)rgData.data() + offset) = *(u16*)((u32*)sourceData[level].data() + offset);
                }
            }
            sourceData[level] = move(rgData);
        }
    }

    if (compressed)
    {
        sourceFiles[index].mipLevels.resize(mipLevels);
        for (u32 level=0; level<mipLevels; ++level)
        {
            i32 sw = w >> level;
            i32 sh = h >> level;
            switch(textureType)
            {
                case TextureType::COLOR:
                case TextureType::CUBE_MAP:
                    println("Compressing mip #%i %ix%i with DXT", level, sw, sh);
                    sourceFiles[index].mipLevels[level] = move(
                        compressDXT((u32*)sourceData[level].data(), sw, sh,
                        preserveAlpha && textureType == TextureType::COLOR));
                    break;
                case TextureType::GRAYSCALE:
                    println("Compressing mip #%i %ix%i with BC4", level, sw, sh);
                    sourceFiles[index].mipLevels[level] = move(
                        compressBC4(sourceData[level].data(), sw, sh));
                    break;
                case TextureType::NORMAL_MAP:
                    println("Compressing mip #%i %ix%i with BC5", level, sw, sh);
                    sourceFiles[index].mipLevels[level] = move(
                        compressBC5((u16*)sourceData[level].data(), sw, sh));
                    break;
            }
        }
    }
    else
    {
        sourceFiles[index].mipLevels = move(sourceData);
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
    if (sourceFiles[index].mipLevels.empty())
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
    u32 mipLevels = s.mipLevels.size();

    glCreateTextures(GL_TEXTURE_2D, 1, &s.previewHandle);
    glTextureStorage2D(s.previewHandle, mipLevels, internalFormat, s.width, s.height);
    for (u32 level=0; level<mipLevels; ++level)
    {
        i32 width = s.width >> level;
        i32 height = s.height >> level;
        if (compressed)
        {
#ifndef NDEBUG
            int v;
            glGetTextureLevelParameteriv(s.previewHandle, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &v);
            assert(s.mipLevels[level].size() == v);
#endif
            glCompressedTextureSubImage2D(s.previewHandle, level, 0, 0, width, height, baseFormat,
                    s.mipLevels[level].size(), s.mipLevels[level].data());
        }
        else
        {
            glTextureSubImage2D(s.previewHandle, level, 0, 0, width, height, baseFormat,
                    GL_UNSIGNED_BYTE, s.mipLevels[level].data());
        }
    }

    if (textureType == TextureType::GRAYSCALE)
    {
        GLint swizzle[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
        glTextureParameteriv(s.previewHandle, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
    }
    if (repeat && textureType != TextureType::CUBE_MAP)
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
    u32 mipLevels = sourceFiles[0].mipLevels.size();
    for (u32 i=0; i<6; ++i)
    {
        if (sourceFiles[i].mipLevels.empty())
        {
            return;
        }
        if (sourceFiles[i].mipLevels.size() != mipLevels)
        {
            showError("All faces must be the same size.");
            return;
        }
    }

    GLuint internalFormat = compressed ? GL_COMPRESSED_SRGB_S3TC_DXT1_EXT : GL_SRGB8;
    GLuint baseFormat = compressed ? internalFormat : GL_RGBA;

    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &cubemapHandle);
    glTextureStorage2D(cubemapHandle, mipLevels, internalFormat, width, height);
    for (u32 i=0; i<6; ++i)
    {
        for (u32 level=0; level<mipLevels; ++level)
        {
            i32 mipWidth = width >> level;
            i32 mipHeight = height >> level;
            if (compressed)
            {
#if 0
				// returns different values on Intel vs NVidia
                int v;
                glGetTextureLevelParameteriv(cubemapHandle, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &v);
                assert(sourceFiles[i].mipLevels[level].size() == v/6);
#endif
                glCompressedTextureSubImage3D(cubemapHandle, level, 0, 0, i, mipWidth, mipHeight, 1,
                        baseFormat, sourceFiles[i].mipLevels[level].size(),
                        sourceFiles[i].mipLevels[level].data());
            }
            else
            {
                glTextureSubImage3D(cubemapHandle, level, 0, 0, i, mipWidth, mipHeight, 1,
                        baseFormat, GL_UNSIGNED_BYTE, sourceFiles[i].mipLevels[level].data());
            }
        }
    }
    glTextureParameteri(cubemapHandle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(cubemapHandle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(cubemapHandle, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTextureParameteri(cubemapHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(cubemapHandle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameterf(cubemapHandle, GL_TEXTURE_MAX_ANISOTROPY, min(max((f32)anisotropy, 1.f),
                max((f32)g_game.config.graphics.anisotropicFilteringLevel, 1.f)));
}
