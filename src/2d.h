#pragma once

#include "game.h"
#include "font.h"

namespace ui
{
    enum Priority
    {
        BORDER = -100,
        BG = -50,
        IMAGE = 10,
        ICON = 50,
        TEXT = 100,
    };

    struct Vertex
    {
        Vec2 xy;
        Vec2 uv;
    };

    struct Quad
    {
        Texture* tex;
        Vertex points[4];
        Vec4 color1;
        Vec4 color2;
        f32 alpha = 1.f;
    };

    void drawQuad(Quad const& q)
    {
        glBindTextureUnit(1, q.tex->handle);
        glUniform4fv(0, 4, (GLfloat*)&q.points);
        glUniform4fv(4, 1, (GLfloat*)&q.color1);
        glUniform4fv(6, 1, (GLfloat*)&q.color2);
        glUniform1f(5, q.alpha);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    Mat4 transform(1.f);
    Vec2 scissorPos(0,0);
    Vec2 scissorSize(INFINITY, INFINITY);

    void setScissor(Vec2 pos, Vec2 size)
    {
        scissorPos = pos;
        scissorSize = size;
    }
    void setTransform(Mat4 const& t) { transform = t; }
    bool transformQuad(Quad& q, Mat4 const& transform, Vec2 scissorPos, Vec2 scissorSize)
    {
        // clip
        Vec2 sp1 = scissorPos;
        Vec2 sp2 = scissorPos + scissorSize;
        Vec2 size = q.points[3].xy - q.points[0].xy;
        Vec2 p1 = q.points[0].xy;
        Vec2 uv1 = q.points[0].uv;
        Vec2 uv2 = q.points[3].uv;

        q.points[0].xy = Vec2(
                clamp(q.points[0].xy.x, sp1.x, sp2.x),
                clamp(q.points[0].xy.y, sp1.y, sp2.y));
        Vec2 t = (q.points[0].xy - p1) / size;
        q.points[0].uv.u = lerp(uv1.u, uv2.u, t.u);
        q.points[0].uv.v = lerp(uv1.v, uv2.v, t.v);

        q.points[1].xy = Vec2(
                clamp(q.points[1].xy.x, sp1.x, sp2.x),
                clamp(q.points[1].xy.y, sp1.y, sp2.y));
        t = (q.points[1].xy - p1) / size;
        q.points[1].uv.u = lerp(uv1.u, uv2.u, t.u);
        q.points[1].uv.v = lerp(uv1.v, uv2.v, t.v);

        q.points[2].xy = Vec2(
                clamp(q.points[2].xy.x, sp1.x, sp2.x),
                clamp(q.points[2].xy.y, sp1.y, sp2.y));
        t = (q.points[2].xy - p1) / size;
        q.points[2].uv.u = lerp(uv1.u, uv2.u, t.u);
        q.points[2].uv.v = lerp(uv1.v, uv2.v, t.v);

        q.points[3].xy = Vec2(
                clamp(q.points[3].xy.x, sp1.x, sp2.x),
                clamp(q.points[3].xy.y, sp1.y, sp2.y));
        t = (q.points[3].xy - p1) / size;
        q.points[3].uv.u = lerp(uv1.u, uv2.u, t.u);
        q.points[3].uv.v = lerp(uv1.v, uv2.v, t.v);

        // TODO: see if it is actually faster to eliminate degenerate geometry
#if 0
        if (q.points[0].xy.x == q.points[1].xy.x || q.points[0].xy.y == q.points[2].xy.y)
        {
            return false;
        }
#endif

        // transform
        for (u32 i=0; i<4; ++i)
        {
            q.points[i].xy = (transform * Vec4(q.points[i].xy, 0.f, 1.f)).xy;
        }

        return true;
    }

    void rect(i32 priority, Texture* tex, Vec2 pos, Vec2 size,
            Vec3 const& color=Vec3(1.f), f32 alpha=1.f)
    {
        static ShaderHandle shader = getShaderHandle("quad2D", { {"GRADIENT"}, {"COLOR"} });
        Vec2 t1(0.f);
        Vec2 t2(1.f);
        Vec2 p1 = pos;
        Vec2 p2 = p1 + size;

        Quad q;
        q.tex = tex ? tex : &g_res.white;
        q.points[0] = { p1, t1 };
        q.points[1] = { { p2.x, p1.y }, { t2.x, t1.y } };
        q.points[2] = { { p1.x, p2.y }, { t1.x, t2.y } };
        q.points[3] = { p2, t2 };
        q.color1 = Vec4(color, 1.f);
        q.color2 = Vec4(color, 1.f);
        q.alpha = alpha;

        if (transformQuad(q, transform, scissorPos, scissorSize))
        {
            g_game.renderer->add2D(shader, priority, [q]{ drawQuad(q); });
        }
    }

    void rectUV(i32 priority, Texture* tex, Vec2 pos, Vec2 size,
            Vec2 t1, Vec2 t2, Vec3 const& color=Vec3(1.f), f32 alpha=1.f)
    {
        static ShaderHandle shader = getShaderHandle("quad2D", { {"GRADIENT"}, {"COLOR"} });
        Vec2 p1 = pos;
        Vec2 p2 = p1 + size;

        Quad q;
        q.tex = tex ? tex : &g_res.white;
        q.points[0] = { p1, t1 };
        q.points[1] = { { p2.x, p1.y }, { t2.x, t1.y } };
        q.points[2] = { { p1.x, p2.y }, { t1.x, t2.y } };
        q.points[3] = { p2, t2 };
        q.color1 = Vec4(color, 1.f);
        q.color2 = Vec4(color, 1.f);
        q.alpha = alpha;

        if (transformQuad(q, transform, scissorPos, scissorSize))
        {
            g_game.renderer->add2D(shader, priority, [q]{ drawQuad(q); });
        }
    }

    void rectBlur(i32 priority, Texture* tex, Vec2 pos, Vec2 size,
            Vec4 const& color=Vec4(1.f), f32 alpha=1.f)
    {
        static ShaderHandle shader = getShaderHandle("quad2D", { {"GRADIENT"}, {"BLUR"} });
        Vec2 t1(0.f);
        Vec2 t2(1.f);
        Vec2 p1 = pos;
        Vec2 p2 = p1 + size;

        Quad q;
        q.tex = tex ? tex : &g_res.white;
        q.points[0] = { p1, t1 };
        q.points[1] = { { p2.x, p1.y }, { t2.x, t1.y } };
        q.points[2] = { { p1.x, p2.y }, { t1.x, t2.y } };
        q.points[3] = { p2, t2 };
        q.color1 = color;
        q.color2 = color;
        q.alpha = alpha;

        if (transformQuad(q, transform, scissorPos, scissorSize))
        {
            g_game.renderer->add2D(shader, priority, [q]{ drawQuad(q); });
        }
    }

    void rectBlur(i32 priority, Texture* tex, Vec2 pos, Vec2 size,
            Vec4 const& color1, Vec4 const& color2, f32 alpha=1.f)
    {
        static ShaderHandle shader = getShaderHandle("quad2D", { {"GRADIENT"}, {"BLUR"} });
        Vec2 t1(0.f);
        Vec2 t2(1.f);
        Vec2 p1 = pos;
        Vec2 p2 = p1 + size;

        Quad q;
        q.tex = tex ? tex : &g_res.white;
        q.points[0] = { p1, t1 };
        q.points[1] = { { p2.x, p1.y }, { t2.x, t1.y } };
        q.points[2] = { { p1.x, p2.y }, { t1.x, t2.y } };
        q.points[3] = { p2, t2 };
        q.color1 = color1;
        q.color2 = color2;
        q.alpha = alpha;

        if (transformQuad(q, transform, scissorPos, scissorSize))
        {
            g_game.renderer->add2D(shader, priority, [q]{ drawQuad(q); });
        }
    }

    void rectUVBlur(i32 priority, Texture* tex, Vec2 pos, Vec2 size,
            Vec2 t1, Vec2 t2, Vec4 const& color=Vec4(1.f), f32 alpha=1.f)
    {
        static ShaderHandle shader = getShaderHandle("quad2D", { {"GRADIENT"}, {"BLUR"} });
        Vec2 p1 = pos;
        Vec2 p2 = p1 + size;

        Quad q;
        q.tex = tex ? tex : &g_res.white;
        q.points[0] = { p1, t1 };
        q.points[1] = { { p2.x, p1.y }, { t2.x, t1.y } };
        q.points[2] = { { p1.x, p2.y }, { t1.x, t2.y } };
        q.points[3] = { p2, t2 };
        q.color1 = color;
        q.color2 = color;
        q.alpha = alpha;

        if (transformQuad(q, transform, scissorPos, scissorSize))
        {
            g_game.renderer->add2D(shader, priority, [q]{ drawQuad(q); });
        }
    }

    struct Text
    {
        Font* font;
        const char* text;
        Vec2 pos;
        Vec3 color;
        f32 alpha = 1.f;
        f32 scale = 1.f;
        HAlign halign;
        VAlign valign;
        Mat4 transform;
        Vec2 scissorPos;
        Vec2 scissorSize;
    };

    void text(Font* font, const char* s, Vec2 pos, Vec3 color, f32 alpha=1.f,
            f32 scale=1.f, HAlign halign=HAlign::LEFT, VAlign valign=VAlign::TOP)
    {
        static ShaderHandle shader = getShaderHandle("quad2D", {});

        Text text;
        text.font = font;
        text.text = s;
        text.pos = pos;
        text.color = color;
        text.alpha = alpha;
        text.scale = scale;
        text.halign = halign;
        text.valign = valign;
        text.transform = transform;
        text.scissorPos = scissorPos;
        text.scissorSize = scissorSize;

        g_game.renderer->add2D(shader, TEXT, [text]{
            text.font->draw(text.text, text.pos, text.color, text.alpha, text.scale,
                    text.halign, text.valign, text.transform, text.scissorPos, text.scissorSize);
        });
    }
};
