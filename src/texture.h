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

struct Texture
{
public:
    i64 guid = 0;
    std::string name;
    bool usedForDecal = false;
    bool usedForBillboard = false;
    bool repeat = true;
    bool generateMipMaps = true;
    f32 lodBias = -0.2f;
    i32 anisotropy = 8;
    i32 filter = TextureFilter::TRILINEAR;

    struct SourceFile
    {
        std::string path;
        std::vector<u8> data;
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

private:
    i32 textureType = TextureType::COLOR;

    std::vector<SourceFile> sourceFiles;

    void loadSourceFile(u32 index);
    void initGLTexture(u32 index);
    void initCubemap();

    GLuint cubemapHandle = 0;

public:
    u32 width = 0;
    u32 height = 0;

    // is the same as sourceFile's previewHandle or cubemap handle
    GLuint handle = 0;

    Texture() {}
    Texture(const char* name, u32 width, u32 height, u8* data, u32 textureType)
        : name(name), textureType(textureType), width(width), height(height)
    {
        sourceFiles.push_back({ "", std::vector<u8>(data, data+width*height*4), width, height });
        regenerate();
    }

    void serialize(Serializer& s);
    void regenerate();
    void reloadSourceFiles();
    void setTextureType(u32 textureType);
    void setSourceFile(u32 index, std::string const& path);
    GLuint getPreviewHandle() const { return sourceFiles[0].previewHandle; }
    SourceFile const& getSourceFile(u32 index) const { return sourceFiles[index]; }
    u32 getSourceFileCount() const { return (u32)sourceFiles.size(); }
    i32 getTextureType() const { return textureType; }

    ~Texture()
    {
        // TODO: cleanup
    }
};

