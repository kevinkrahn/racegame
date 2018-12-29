#include "font.h"
#include "game.h"
#include <fstream>
#include <sstream>
#include <stb_rect_pack.h>
#include <stb_truetype.h>

Font::Font(std::string const& filename, f32 height, u32 startingChar, u32 numGlyphs)
{
    this->startingChar = startingChar;
    this->height = height;

    std::ifstream file(filename, std::ios::binary | std::ios::in | std::ios::ate);
    if(!file)
    {
        FATAL_ERROR("Failed to load file: ", filename);
    }

    auto size = file.tellg();
    std::vector<u8> fontData(size);
    file.seekg(0, std::ios::beg);
    file.read((char*)fontData.data(), size);

    stbtt_fontinfo fontInfo;
    stbtt_InitFont(&fontInfo, fontData.data(), stbtt_GetFontOffsetForIndex(fontData.data(), 0));

    f32 scale = stbtt_ScaleForPixelHeight(&fontInfo, height);

    i32 ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

    stbtt_pack_context c;
    std::vector<stbtt_packedchar> charData(numGlyphs);

    i32 w = 1024, h = 1024;
    std::vector<u8> texData(w * h);

    const i32 padding = 4;
    stbtt_PackBegin(&c, texData.data(), w, h, 0, padding, nullptr);

    if (!stbtt_PackFontRange(&c, fontData.data(), 0, height, startingChar, numGlyphs, charData.data()))
    {
        // this might mean the atlas is too small
        FATAL_ERROR("Failed to pack font atlas.");
    }
    stbtt_PackEnd(&c);

    glyphs.resize(numGlyphs);
    kerningTable.resize(numGlyphs * numGlyphs);
    for (u32 i=startingChar; i<startingChar+numGlyphs; ++i)
    {
        u32 g = i - startingChar;
        stbtt_packedchar info = charData[g];

        glyphs[g].x0 = info.x0 / (f32)w;
        glyphs[g].y0 = info.y0 / (f32)h;
        glyphs[g].x1 = info.x1 / (f32)w;
        glyphs[g].y1 = info.y1 / (f32)h;

        glyphs[g].advance = info.xadvance;
        glyphs[g].xOff = info.xoff;
        glyphs[g].yOff = info.yoff;
        glyphs[g].width = info.xoff2 - info.xoff;
        glyphs[g].height = info.yoff2 - info.yoff;

        for (u32 c=0; c<numGlyphs; ++c)
        {
            kerningTable[(i-startingChar)*numGlyphs + c] =
                stbtt_GetCodepointKernAdvance(&fontInfo, i, c+startingChar) * scale;
        }
    }

    Texture fontAtlas;
    fontAtlas.width = w;
    fontAtlas.height = h;
    fontAtlas.format = Texture::Format::R8;

    fontAtlasHandle = game.renderer.loadTexture(fontAtlas, texData.data(), texData.size());
}

glm::vec2 Font::stringDimensions(const char* str, bool onlyFirstLine) const
{
    f32 maxWidth = 0;
    f32 currentWidth = 0;
    f32 currentHeight = 0;

    while (true)
    {
        if (!(*str))
        {
            break;
        }
        if (*str == '\n')
        {
            maxWidth = std::max(currentWidth, maxWidth);
            currentWidth = 0;
            currentHeight += lineHeight;
            if (onlyFirstLine)
            {
                break;
            }
            ++str;

            continue;
        }

        GlyphMetrics const& g = glyphs[*str-startingChar];
        currentWidth += g.advance;

        if (!*(str+1))
        {
            currentHeight += height;
            break;
        }

        currentWidth += kerningTable[(*str - startingChar)*glyphs.size() + (*(str+1) - startingChar)];
        ++str;
    }

    return { std::max(currentWidth, maxWidth), currentHeight };
}

void Font::drawText(const char* str, f32 x, f32 y, glm::vec3 color, f32 alpha,
        f32 scale, HorizontalAlign halign, VerticalAlign valign)
{
    f32 startX = x;

    if (halign != HorizontalAlign::LEFT)
    {
        f32 lineWidth = stringDimensions(str, true).x * scale;
        if (halign == HorizontalAlign::CENTER)
        {
            x -= lineWidth * 0.5f;
        }
        else if (halign == HorizontalAlign::RIGHT)
        {
            x -= lineWidth;
        }
    }

    y += height * scale;
    if (valign != VerticalAlign::TOP)
    {
        f32 stringHeight = stringDimensions(str).y * scale;
        if (valign == VerticalAlign::BOTTOM)
        {
            y -= stringHeight;
        }
        else if (valign == VerticalAlign::CENTER)
        {
            y -= stringHeight * 0.5f;
        }
    }

    while (*str)
    {
        if (*str == '\n')
        {
            ++str;
            f32 nextLineWidth = stringDimensions(str, true).x * scale;

            if (halign == HorizontalAlign::CENTER)
            {
                x = startX - nextLineWidth * 0.5f;
            }
            else if (halign == HorizontalAlign::LEFT)
            {
                x = startX;
            }
            else
            {
                x = startX - nextLineWidth;
            }

            y += lineHeight * scale;

            continue;
        }

        auto &g = glyphs[(u32)(*str - startingChar)];

        f32 x0 = x + g.xOff * scale;
        f32 y0 = y + g.yOff * scale;
        f32 x1 = x0 + g.width * scale;
        f32 y1 = y0 + g.height * scale;

        game.renderer.drawQuad2D(fontAtlasHandle,
                { x0, y0 }, { x1, y1 }, { g.x0, g.y0 }, { g.x1, g.y1 }, color, alpha);

        x += g.advance * scale;

        // kerning
        if (*(str+1))
        {
            x += kerningTable[(*str - startingChar) * glyphs.size() + (*(str+1) - startingChar)] * scale;
        }

        ++str;
    }
}
