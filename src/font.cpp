#include "font.h"
#include "renderer.h"
#include "game.h"
#include <fstream>
#include <sstream>
#include <stb_rect_pack.h>
#include <stb_truetype.h>

Font::Font(std::string const& filename, f32 fontSize, u32 startingChar, u32 numGlyphs)
{
    this->startingChar = startingChar;

    std::ifstream file(filename, std::ios::binary | std::ios::in | std::ios::ate);
    if(!file)
    {
        FATAL_ERROR("Failed to load file: ", filename);
    }

    auto size = file.tellg();
    std::vector<u8> fontData((u32)size);
    file.seekg(0, std::ios::beg);
    file.read((char*)fontData.data(), size);

    stbtt_fontinfo fontInfo;
    stbtt_InitFont(&fontInfo, fontData.data(), stbtt_GetFontOffsetForIndex(fontData.data(), 0));

    f32 scale = stbtt_ScaleForPixelHeight(&fontInfo, fontSize);

    i32 ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

    stbtt_pack_context c;
    std::vector<stbtt_packedchar> charData(numGlyphs);

    i32 w = 1024, h = 1024;
    std::vector<u8> texData(w * h);

    const i32 padding = 4;
    stbtt_PackBegin(&c, texData.data(), w, h, 0, padding, nullptr);

    if (!stbtt_PackFontRange(&c, fontData.data(), 0, fontSize, startingChar, numGlyphs, charData.data()))
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

    textureAtlas = Texture(filename.c_str(), w, h, texData.data(), Texture::Format::R8);
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
            maxWidth = glm::max(currentWidth, maxWidth);
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

    return { glm::max(currentWidth, maxWidth), currentHeight };
}

void TextRenderable::on2DPass(Renderer* renderer)
{
    char* str = (char*)text;
    glm::vec2 p = pos;
    f32 startX = p.x;

    if (halign != HorizontalAlign::LEFT)
    {
        f32 lineWidth = font->stringDimensions(str, true).x * scale;
        if (halign == HorizontalAlign::CENTER)
        {
            p.x -= lineWidth * 0.5f;
        }
        else if (halign == HorizontalAlign::RIGHT)
        {
            p.x -= lineWidth;
        }
    }

    p.y += font->height * scale;
    if (valign != VerticalAlign::TOP)
    {
        f32 stringHeight = font->stringDimensions(str).y * scale;
        if (valign == VerticalAlign::BOTTOM)
        {
            p.y -= stringHeight;
        }
        else if (valign == VerticalAlign::CENTER)
        {
            p.y -= stringHeight * 0.5f;
        }
    }

    glUseProgram(renderer->getShaderProgram("text2D"));
    glBindTextureUnit(1, font->textureAtlas.handle);

    while (*str)
    {
        if (*str == '\n')
        {
            ++str;
            f32 nextLineWidth = font->stringDimensions(str, true).x * scale;

            if (halign == HorizontalAlign::CENTER)
            {
                p.x = startX - nextLineWidth * 0.5f;
            }
            else if (halign == HorizontalAlign::LEFT)
            {
                p.x = startX;
            }
            else
            {
                p.x = startX - nextLineWidth;
            }

            p.y += font->lineHeight * scale;

            continue;
        }

        auto &g = font->glyphs[(u32)(*str - font->startingChar)];

        f32 x0 = glm::floor(p.x + g.xOff * scale);
        f32 y0 = glm::floor(p.y + g.yOff * scale);
        //f32 x1 = glm::floor(x0 + (g.x1 - g.x0) * font->textureAtlas.width * scale);
        //f32 y1 = glm::floor(y0 + (g.y1 - g.y0) * font->textureAtlas.height * scale);
        //f32 x1 = x0 + g.width * scale;
        //f32 y1 = y0 + g.height * scale;
        f32 x1 = glm::floor(x0 + g.width * scale);
        f32 y1 = glm::floor(y0 + g.height * scale);

        struct QuadPoint
        {
            glm::vec2 xy;
            glm::vec2 uv;
        };
        glm::vec2 p1 = { x0, y0 };
        glm::vec2 p2 = { x1, y1 };
        glm::vec2 t1 = { g.x0, g.y0 };
        glm::vec2 t2 = { g.x1, g.y1 };
        QuadPoint points[4] = {
            { p1, t1 },
            { { p2.x, p1.y }, { t2.x, t1.y } },
            { { p1.x, p2.y }, { t1.x, t2.y } },
            { p2, t2 }
        };

        glm::vec4 col(color, alpha);
        glUniform4fv(0, 4, (GLfloat*)&points);
        glUniform4fv(4, 1, (GLfloat*)&col);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        p.x += g.advance * scale;

        // kerning
        if (*(str+1) && *(str+1) != '\n')
        {
            p.x += font->kerningTable[(*str - font->startingChar) * font->glyphs.size() +
                (*(str+1) - font->startingChar)] * scale;
        }

        ++str;
    }
}

