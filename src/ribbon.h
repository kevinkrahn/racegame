#pragma once

#include "math.h"
#include "gl.h"
#include "dynamic_buffer.h"
#include "resources.h"
#include "renderer.h"

struct RibbonPoint
{
    Vec3 position;
    Vec3 normal;
    Vec4 color;
    f32 width;
    f32 life;
    f32 texU;
    bool isEnd;
};

struct RibbonVertex
{
    Vec3 position;
    Vec3 normal;
    Vec4 color;
    Vec2 uv;
};

class Ribbon
{
private:
    Array<RibbonPoint> points;
    static constexpr f32 minDistanceBetweenPoints = 0.9f;
    f32 texU = 0.f;
    f32 fadeRate = 0.1f;
    f32 fadeDelay = 8.f;
    RibbonPoint lastPoint;

    bool connected(size_t pointIndex)
    {
        auto p = points.begin() + pointIndex;
        if (p->isEnd)
        {
            if (p != points.begin())
            {
                return true;
            }
            return false;
        }
		auto next = p + 1;
        if (next != points.end() && next->color.a > 0.f)
        {
            return true;
        }
        if (p != points.begin() && (p - 1)->color.a > 0.f)
        {
            return true;
        }
        return false;
    }

public:
    void addPoint(Vec3 const& position, Vec3 const& normal, f32 width, Vec4 const& color, bool endChain=false)
    {
        lastPoint = { position, normal, color, width, 0.f, texU, endChain };
        if (endChain ||
            points.empty() ||
            points.back().isEnd ||
            lengthSquared(position - points.back().position) > square(minDistanceBetweenPoints))
        {
            if (points.empty() || points.back().isEnd)
            {
                lastPoint.color.a = 0.f;
            }
            if (!points.empty() && !points.back().isEnd)
            {
                Vec3 diff = position - points.back().position;
                texU += (length(diff) / ((width + points.back().width) / 2)) / 8;
            }
            points.push(lastPoint);
        }
    }

    void capWithLastPoint()
    {
        if (points.empty())
        {
            return;
        }
        RibbonPoint& prevPoint = points.back();
        if (prevPoint.position == lastPoint.position)
        {
            prevPoint.isEnd = true;
            prevPoint.color.a = 0.f;
        }
        else
        {
            lastPoint.isEnd = true;
            lastPoint.color.a = 0.f;
            points.push(lastPoint);
        }
    }

    void update(f32 deltaTime)
    {
        for (auto p = points.begin(); p != points.end();)
        {
            if (p->life > fadeDelay)
            {
                p->color.a = max(0.f, p->color.a - deltaTime * fadeRate);
                if (p->color.a <= 0.f && !connected(p - points.begin()))
                {
                    p = points.erase(p);
                    continue;
                }
            }
            p->life += deltaTime;
            ++p;
        }
    }

    u32 getRequiredBufferSize() const
    {
        if (points.size() <= 1)
        {
            return 0;
        }

        u32 size = 0;
        bool hasHoldPoint = false;
        auto countPoint = [&](RibbonPoint const& v) {
            if (!hasHoldPoint)
            {
                if (!v.isEnd) hasHoldPoint = true;
                return;
            }
            size += sizeof(RibbonVertex) * 6;
            if (v.isEnd) hasHoldPoint = false;
        };

        for (auto const& v : points)
        {
            countPoint(v);
        }
        if (!points.back().isEnd &&
            lengthSquared(points.back().position - lastPoint.position) > square(0.1f))
        {
            countPoint(lastPoint);
        }

        return size;
    }

    u32 writeVerts(void* buffer) const
    {
        if (points.size() <= 1)
        {
            return 0;
        }

        RibbonVertex* d = (RibbonVertex*)buffer;
        RibbonPoint holdPoint;
        RibbonVertex holdVerts[2];
        bool hasHoldPoint = false;
        bool firstInChain = false;
        u32 vertexCount = 0;

        auto writePoint = [&] (RibbonPoint const& v) {
            if (!hasHoldPoint)
            {
                if (v.isEnd)
                {
                    return;
                }
                holdPoint = v;
                hasHoldPoint = true;
                firstInChain = true;
                return;
            }

            // TODO: offset based on normal?
            Vec3 diff = v.position - holdPoint.position;
            Vec2 offsetDir = normalize(Vec2(-diff.y, diff.x));
            Vec3 offset = Vec3(offsetDir * v.width, 0);

            if (firstInChain)
            {
                firstInChain = false;
                Vec3 prevOffset(offsetDir * holdPoint.width, 0);
                holdVerts[0] = { holdPoint.position + prevOffset,
                    holdPoint.normal, holdPoint.color, Vec2(holdPoint.texU, 1) };
                holdVerts[1] = { holdPoint.position - prevOffset,
                    holdPoint.normal, holdPoint.color, Vec2(holdPoint.texU, 0) };
            }

            d[0] = { v.position - offset, v.normal, v.color, Vec2(v.texU, 0) };
            d[1] = { v.position + offset, v.normal, v.color, Vec2(v.texU, 1) };
            d[2] = holdVerts[0];

            d[3] = holdVerts[0];
            d[4] = holdVerts[1];
            d[5] = d[0];

            holdPoint = v;
            holdVerts[0] = d[1];
            holdVerts[1] = d[0];
            d += 6;

            vertexCount += 6;

            if (v.isEnd)
            {
                hasHoldPoint = false;
            }
        };

        for (auto const& v : points)
        {
            writePoint(v);
        }
        if (!points.back().isEnd &&
            lengthSquared(points.back().position - lastPoint.position) > square(0.1f))
        {
            writePoint(lastPoint);
        }

        return vertexCount;
    }
};

class RibbonRenderer {
    struct Chunk
    {
        Ribbon* ribbon;
        u32 count;
    };

    Array<Chunk> chunks;
    DynamicBuffer vertexBuffer;
    GLuint vao;

public:
    RibbonRenderer() : vertexBuffer(sizeof(RibbonVertex) * 40000)
    {
        glCreateVertexArrays(1, &vao);

        glEnableVertexArrayAttrib(vao, 0);
        glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao, 0, 0);

        glEnableVertexArrayAttrib(vao, 1);
        glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, 12);
        glVertexArrayAttribBinding(vao, 1, 0);

        glEnableVertexArrayAttrib(vao, 2);
        glVertexArrayAttribFormat(vao, 2, 4, GL_FLOAT, GL_FALSE, 12 + 12);
        glVertexArrayAttribBinding(vao, 2, 0);

        glEnableVertexArrayAttrib(vao, 3);
        glVertexArrayAttribFormat(vao, 3, 2, GL_FLOAT, GL_FALSE, 12 + 12 + 16);
        glVertexArrayAttribBinding(vao, 3, 0);
    }

    ~RibbonRenderer()
    {
        vertexBuffer.destroy();
        glDeleteVertexArrays(1, &vao);
    }

    void addChunk(Ribbon* ribbon)
    {
        u32 size = ribbon->getRequiredBufferSize();
        if (size > 0)
        {
            void* mem = vertexBuffer.map(size);
            u32 count = ribbon->writeVerts(mem);
            vertexBuffer.unmap();
            chunks.push({ ribbon, count });
        }
    }

    void update(f32 deltaTime)
    {
        for (auto& chunk : chunks)
        {
            chunk.ribbon->update(deltaTime);
        }
    }

    void endUpdate()
    {
        chunks.clear();
    }

    void draw(RenderWorld* rw)
    {
        static ShaderHandle shader = getShaderHandle("ribbon", {}, RenderFlags::DEPTH_READ, -1000.f);
        if (chunks.empty())
        {
            return;
        }
        auto render = [](void* renderData) {
            RibbonRenderer* r = (RibbonRenderer*)renderData;

            glBindTextureUnit(0, g_res.getTexture("tiremarks")->handle);
            glVertexArrayVertexBuffer(r->vao, 0, r->vertexBuffer.getBuffer(), 0, sizeof(RibbonVertex));
            glBindVertexArray(r->vao);

            u32 offset = 0;
            for (auto const& chunk : r->chunks)
            {
                glDrawArrays(GL_TRIANGLES, offset, chunk.count);
                offset += chunk.count;
            }
        };
        rw->transparentPass({ shader, TransparentDepth::TIRE_MARKS, this, render });
    }
};
