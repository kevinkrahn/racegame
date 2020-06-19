#include "font.h"
#include "renderer.h"
#include "game.h"
#include <stb_rect_pack.h>
#include <stb_truetype.h>

Font::Font(const char* filename, f32 fontSize, u32 startingChar, u32 numGlyphs)
{
    this->startingChar = startingChar;

    Buffer fontData = readFileBytes(filename);

    stbtt_fontinfo fontInfo;
    stbtt_InitFont(&fontInfo, fontData.data.get(), stbtt_GetFontOffsetForIndex(fontData.data.get(), 0));

    f32 scale = stbtt_ScaleForPixelHeight(&fontInfo, fontSize);

    i32 ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

    stbtt_pack_context c;
    Array<stbtt_packedchar> charData(numGlyphs);

    i32 w = 1024, h = 1024;
    Array<u8> texData(w * h);

    const i32 padding = 4;
    stbtt_PackBegin(&c, texData.data(), w, h, 0, padding, nullptr);

    if (!stbtt_PackFontRange(&c, fontData.data.get(), 0, fontSize, startingChar, numGlyphs, charData.data()))
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

    // set the line height based on the height of an uppercase character
    auto &f = glyphs['M'-startingChar];
    height = f.height;
    lineHeight = ((ascent - descent) + lineGap) * scale;

    textureAtlas = Texture(filename, w, h, texData.data(), (u32)texData.size(),
            TextureType::GRAYSCALE);
}

Vec2 Font::stringDimensions(const char* str, bool onlyFirstLine) const
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
            maxWidth = max(currentWidth, maxWidth);
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

        if (*(str+1) != '\n')
        {
            currentWidth += kerningTable[(*str - startingChar)*glyphs.size() + (*(str+1) - startingChar)];
        }
        ++str;
    }

    return { max(currentWidth, maxWidth), currentHeight };
}

void Font::draw(const char* text, Vec2 pos, Vec3 color, f32 alpha,
            f32 scale, HAlign halign, VAlign valign)
{
    char* str = (char*)text;
    Vec2 p = pos;
    f32 startX = p.x;

    if (halign != HAlign::LEFT)
    {
        f32 lineWidth = stringDimensions(str, true).x * scale;
        if (halign == HAlign::CENTER)
        {
            p.x -= lineWidth * 0.5f;
        }
        else if (halign == HAlign::RIGHT)
        {
            p.x -= lineWidth;
        }
    }

    p.y += height * scale;
    if (valign != VAlign::TOP)
    {
        f32 stringHeight = stringDimensions(str).y * scale;
        if (valign == VAlign::BOTTOM)
        {
            p.y -= stringHeight;
        }
        else if (valign == VAlign::CENTER)
        {
            p.y -= stringHeight * 0.5f;
        }
    }

    glBindTextureUnit(1, textureAtlas.handle);

    while (*str)
    {
        if (*str == '\n')
        {
            ++str;
            f32 nextLineWidth = stringDimensions(str, true).x * scale;

            if (halign == HAlign::CENTER)
            {
                p.x = startX - nextLineWidth * 0.5f;
            }
            else if (halign == HAlign::LEFT)
            {
                p.x = startX;
            }
            else
            {
                p.x = startX - nextLineWidth;
            }

            p.y += lineHeight * scale;

            continue;
        }

        auto &g = glyphs[(u32)(*str - startingChar)];

        f32 x0 = floorf(p.x + g.xOff * scale);
        f32 y0 = floorf(p.y + g.yOff * scale);
        //f32 x1 = floorf(x0 + (g.x1 - g.x0) * textureAtlas.width * scale);
        //f32 y1 = floorf(y0 + (g.y1 - g.y0) * textureAtlas.height * scale);
        //f32 x1 = x0 + g.width * scale;
        //f32 y1 = y0 + g.height * scale;
        f32 x1 = floorf(x0 + g.width * scale);
        f32 y1 = floorf(y0 + g.height * scale);

        struct QuadPoint
        {
            Vec2 xy;
            Vec2 uv;
        };
        Vec2 p1 = { x0, y0 };
        Vec2 p2 = { x1, y1 };
        Vec2 t1 = { g.x0, g.y0 };
        Vec2 t2 = { g.x1, g.y1 };
        QuadPoint points[4] = {
            { p1, t1 },
            { { p2.x, p1.y }, { t2.x, t1.y } },
            { { p1.x, p2.y }, { t1.x, t2.y } },
            { p2, t2 }
        };

        Vec4 col(color, alpha);
        glUniform4fv(0, 4, (GLfloat*)&points);
        glUniform4fv(4, 1, (GLfloat*)&col);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        p.x += g.advance * scale;

        // kerning
        if (*(str+1) && *(str+1) != '\n')
        {
            p.x += kerningTable[(*str - startingChar) * glyphs.size() +
                (*(str+1) - startingChar)] * scale;
        }

        ++str;
    }
}
