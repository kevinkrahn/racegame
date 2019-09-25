#pragma once

#include "math.h"
#include "renderable.h"
#include "texture.h"
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
    Texture textureAtlas;
    f32 height;
    f32 lineHeight;
    u32 startingChar;
    std::vector<GlyphMetrics> glyphs;
    std::vector<f32> kerningTable;

    friend class TextRenderable;

public:
    Font() {};
    Font(std::string const& filename, f32 height, u32 startingChar=32, u32 numGlyphs=95);

    glm::vec2 stringDimensions(const char* str, bool onlyFirstLine=false) const;

    f32 getHeight() const { return height; }
    f32 getLineHeight() const { return lineHeight; }
};

class TextRenderable : public Renderable2D
{
public:
    Font* font;
    const char* text;
    glm::vec2 pos;
    glm::vec3 color;
    f32 alpha = 1.f;
    f32 scale = 1.f;
    HorizontalAlign halign;
    VerticalAlign valign;

    TextRenderable(Font* font, const char* text, glm::vec2 pos, glm::vec3 color, f32 alpha=1.f,
            f32 scale=1.f, HorizontalAlign halign=HorizontalAlign::LEFT,
            VerticalAlign valign=VerticalAlign::TOP) : font(font), text(text), pos(pos), color(color),
            alpha(alpha), scale(scale), halign(halign), valign(valign) {}
    void on2DPass(Renderer* renderer) override;
};
