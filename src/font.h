#pragma once

#include "math.h"
#include <vector>

enum struct HorizontalAlign
{
    LEFT,
    RIGHT,
    CENTER
};

enum struct VerticalAlign
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
private:
    u32 fontAtlasHandle;
    f32 height;
    f32 lineHeight;
    u32 startingChar;
    std::vector<GlyphMetrics> glyphs;
    std::vector<f32> kerningTable;

public:
    Font() {};
    Font(std::string const& filename, f32 height, u32 startingChar=32, u32 numGlyphs=95);

    glm::vec2 stringDimensions(const char* str, bool onlyFirstLine=false) const;

    void drawText(const char* str, f32 x, f32 y, glm::vec3 color, f32 alpha=1.f,
            f32 scale=1.f, HorizontalAlign halign=HorizontalAlign::LEFT,
            VerticalAlign valign=VerticalAlign::TOP);
};
