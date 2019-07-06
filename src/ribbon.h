#pragma once

#include "math.h"
#include "renderable.h"
#include "gl.h"
#include "dynamic_buffer.h"
#include "resources.h"
#include "renderer.h"
#include <vector>

struct RibbonPoint
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 color;
    f32 width;
    f32 life;
    f32 texU;
    bool isEnd;
};

struct RibbonVertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 color;
    glm::vec2 uv;
};

class Ribbon
{
private:
    std::vector<RibbonPoint> points;
    static constexpr f32 minDistanceBetweenPoints = 0.9f;
    f32 texU = 0.f;
    f32 fadeRate = 0.1f;
    f32 fadeDelay = 8.f;
    RibbonPoint lastPoint;

    bool connected(std::vector<RibbonPoint>::iterator p)
    {
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
    void addPoint(glm::vec3 const& position, glm::vec3 const& normal, f32 width, glm::vec4 const& color, bool endChain=false)
    {
        lastPoint = { position, normal, color, width, 0.f, texU, endChain };
        if (endChain ||
            points.empty() ||
            points.back().isEnd ||
            glm::length2(position - points.back().position) > square(minDistanceBetweenPoints))
        {
            if (points.empty() || points.back().isEnd)
            {
                lastPoint.color.a = 0.f;
            }
            if (!points.empty() && !points.back().isEnd)
            {
                glm::vec3 diff = position - points.back().position;
                texU += (length(diff) / ((width + points.back().width) / 2)) / 8;
            }
            points.push_back(lastPoint);
        }
    }

    void capWithLastPoint()
    {
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
            points.push_back(lastPoint);
        }
    }

    void update(f32 deltaTime)
    {
        for (auto p = points.begin(); p != points.end();)
        {
            if (p->life > fadeDelay)
            {
                p->color.a = glm::max(0.f, p->color.a - deltaTime * fadeRate);
                if (p->color.a <= 0.f && !connected(p))
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
            glm::length2(points.back().position - lastPoint.position) > square(0.1f))
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
            glm::vec3 diff = v.position - holdPoint.position;
            glm::vec2 offsetDir = glm::normalize(glm::vec2(-diff.y, diff.x));
            glm::vec3 offset = glm::vec3(offsetDir * v.width, 0);

            if (firstInChain)
            {
                firstInChain = false;
                glm::vec3 prevOffset(offsetDir * holdPoint.width, 0);
                holdVerts[0] = { holdPoint.position + prevOffset,
                    holdPoint.normal, holdPoint.color, glm::vec2(holdPoint.texU, 1) };
                holdVerts[1] = { holdPoint.position - prevOffset,
                    holdPoint.normal, holdPoint.color, glm::vec2(holdPoint.texU, 0) };
            }

            d[0] = { v.position - offset, v.normal, v.color, glm::vec2(v.texU, 0) };
            d[1] = { v.position + offset, v.normal, v.color, glm::vec2(v.texU, 1) };
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
            glm::length2(points.back().position - lastPoint.position) > square(0.1f))
        {
            writePoint(lastPoint);
        }

        return vertexCount;
    }
};

class RibbonRenderable : public Renderable {
    struct Chunk
    {
        Ribbon* ribbon;
        u32 count;
    };

    std::vector<Chunk> chunks;
    DynamicBuffer vertexBuffer;
    GLuint vao;

public:
    RibbonRenderable() : vertexBuffer(sizeof(RibbonVertex) * 40000)
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

    ~RibbonRenderable()
    {
        vertexBuffer.destroy();
        glDeleteVertexArrays(1, &vao);
    }

    i32 getPriority() const override { return 9000; }

    void addChunk(Ribbon* ribbon)
    {
        u32 size = ribbon->getRequiredBufferSize();
        if (size > 0)
        {
            void* mem = vertexBuffer.map(size);
            u32 count = ribbon->writeVerts(mem);
            vertexBuffer.unmap();
            chunks.push_back({ ribbon, count });
        }
    }

    void onUpdate(f32 deltaTime) override
    {
        for (auto& chunk : chunks)
        {
            chunk.ribbon->update(deltaTime);
        }
    }

    void onLitPass(Renderer* renderer) override
    {
        if (chunks.empty())
        {
            return;
        }

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0.f, -1000.f);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        glUseProgram(renderer->getShaderProgram("ribbon"));

        glVertexArrayVertexBuffer(vao, 0, vertexBuffer.getBuffer(), 0, sizeof(RibbonVertex));
        glBindVertexArray(vao);

        Texture* tex = g_resources.getTexture("tiremarks");
        glBindTextureUnit(0, tex->handle);

        u32 offset = 0;
        for (auto const& chunk : chunks)
        {
            glDrawArrays(GL_TRIANGLES, offset, chunk.count);
            offset += chunk.count;
        }

        glDisable(GL_POLYGON_OFFSET_FILL);

        chunks.clear();
    }
};
