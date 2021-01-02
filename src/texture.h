#pragma once

#include "math.h"
#include "gl.h"
#include "resource.h"
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

namespace TextureType
{
    enum
    {
        COLOR = 0,
        GRAYSCALE = 1,
        NORMAL_MAP = 2,
        CUBE_MAP = 3,
        MAX
    };
};

struct Texture : public Resource
{
public:
    bool repeat = true;
    bool generateMipMaps = true;
    f32 lodBias = -0.2f;
    i32 anisotropy = 8;
    i32 filter = TextureFilter::TRILINEAR;

    struct SourceFile
    {
        Str64 path;
        Array<u8> data;
        u32 width = 0;
        u32 height = 0;

        GLuint previewHandle = 0;

        void serialize(Serializer& s)
        {
            s.field(path);
            s.field(data);
            s.field(width);
            s.field(height);
        }
    };

    void serialize(Serializer& s) override
    {
        Resource::serialize(s);

        s.field(textureType);
        s.field(repeat);
        s.field(generateMipMaps);
        s.field(lodBias);
        s.field(anisotropy);
        s.field(filter);
        s.field(sourceFiles);

        if (s.deserialize)
        {
            regenerate();
        }
    }

private:
    i32 textureType = TextureType::COLOR;

    Array<SourceFile> sourceFiles;

    void loadSourceFile(u32 index);
    void initGLTexture(u32 index);
    void initCubemap();

    GLuint cubemapHandle = 0;

public:
    u32 width = 0;
    u32 height = 0;

    // is the same as sourceFile's previewHandle or cubemap handle
    GLuint handle = 0;

    Texture() { setTextureType(TextureType::COLOR); }
    Texture(const char* name, u32 width, u32 height, u8* data, u32 dataSize, u32 textureType)
        : textureType(textureType), width(width), height(height)
    {
        this->name = name;
        sourceFiles.push({ "", Array<u8>(data, data+dataSize), width, height });
        regenerate();
    }

    void regenerate();
    void reloadSourceFiles();
    void setTextureType(u32 textureType);
    void setSourceFile(u32 index, const char* path);
    GLuint getPreviewHandle() const { return sourceFiles[0].previewHandle; }
    u32 getPreviewTexture() override { return getPreviewHandle(); }
    SourceFile const& getSourceFile(u32 index) const { return sourceFiles[index]; }
    u32 getSourceFileCount() const { return (u32)sourceFiles.size(); }
    i32 getTextureType() const { return textureType; }
    void destroy();

    ~Texture()
    {
        // TODO: cleanup
    }
};
