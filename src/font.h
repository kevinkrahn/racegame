#pragma once

#include "math.h"
#include "texture.h"

enum struct HAlign
{
    LEFT,
    RIGHT,
    CENTER
};

enum struct VAlign
{
    TOP,
    BOTTOM,
    CENTER
};

struct GlyphMetrics
{
    f32 x0, y0, x1, y1; // texCoords
    f32 advance;
    f32 xOff, yOff;
    f32 width, height;
};

class Font
{
    Texture textureAtlas;
    f32 height;
    f32 lineHeight;
    u32 startingChar;
    Array<GlyphMetrics> glyphs;
    Array<f32> kerningTable;

public:
    Font() {}
    Font(std::string const& filename, f32 height, u32 startingChar=32, u32 numGlyphs=95);

    glm::vec2 stringDimensions(const char* str, bool onlyFirstLine=false) const;

    f32 getHeight() const { return height; }
    f32 getLineHeight() const { return lineHeight; }

    void draw(const char* text, glm::vec2 pos, glm::vec3 color, f32 alpha=1.f,
            f32 scale=1.f, HAlign halign=HAlign::LEFT, VAlign valign=VAlign::TOP);
};
