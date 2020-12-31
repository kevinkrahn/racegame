#pragma once

#include "math.h"
#include "dynamic_buffer.h"
#include "bounding_box.h"
#include "renderer.h"

// TODO: this class should be part of RenderWorld
class DebugDraw
{
    struct Vertex
    {
        Vec3 position;
        Vec4 color;
    };
    Array<Vertex> verts;
    DynamicBuffer buffer = DynamicBuffer(sizeof(Vertex) * 500000);
    GLuint vao;

public:
    DebugDraw()
    {
        glCreateVertexArrays(1, &vao);

        glEnableVertexArrayAttrib(vao, 0);
        glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao, 0, 0);

        glEnableVertexArrayAttrib(vao, 1);
        glVertexArrayAttribFormat(vao, 1, 4, GL_FLOAT, GL_FALSE, 12);
        glVertexArrayAttribBinding(vao, 1, 0);
    }

    ~DebugDraw()
    {
        buffer.destroy();
        glDeleteVertexArrays(1, &vao);
    }

    void line(Vec3 const& p1, Vec3 const& p2,
            Vec4 const& c1 = Vec4(1), Vec4 const& c2 = Vec4(1))
    {
        verts.push({ p1, c1 });
        verts.push({ p2, c2 });
    }

    void boundingBox(BoundingBox const& bb, Mat4 const& transform, Vec4 const& color)
    {
        Vec3 p[8] = {
            Vec3(transform * Vec4(bb.min.x, bb.min.y, bb.min.z, 1.f)),
            Vec3(transform * Vec4(bb.max.x, bb.min.y, bb.min.z, 1.f)),
            Vec3(transform * Vec4(bb.max.x, bb.max.y, bb.min.z, 1.f)),
            Vec3(transform * Vec4(bb.min.x, bb.max.y, bb.min.z, 1.f)),
            Vec3(transform * Vec4(bb.min.x, bb.min.y, bb.max.z, 1.f)),
            Vec3(transform * Vec4(bb.max.x, bb.min.y, bb.max.z, 1.f)),
            Vec3(transform * Vec4(bb.max.x, bb.max.y, bb.max.z, 1.f)),
            Vec3(transform * Vec4(bb.min.x, bb.max.y, bb.max.z, 1.f)),
        };

        line(p[0], p[1], color, color);
        line(p[1], p[2], color, color);
        line(p[2], p[3], color, color);
        line(p[3], p[0], color, color);
        line(p[4], p[5], color, color);
        line(p[5], p[6], color, color);
        line(p[6], p[7], color, color);
        line(p[7], p[4], color, color);
        line(p[0], p[4], color, color);
        line(p[1], p[5], color, color);
        line(p[2], p[6], color, color);
        line(p[3], p[7], color, color);
    }

    void draw(RenderWorld* rw)
    {
        static ShaderHandle shader = getShaderHandle("debug", {}, RenderFlags::DEPTH_READ, 0.f);
        static Camera camera;
        camera = rw->getCamera(0);

        if (verts.empty())
        {
            return;
        }

        buffer.updateData(verts.data(), verts.size() * sizeof(Vertex));
        glVertexArrayVertexBuffer(vao, 0, buffer.getBuffer(), 0, sizeof(Vertex));

        auto render = [](void* renderData) {
            DebugDraw* dd = (DebugDraw*)renderData;

            glUniform1i(3, 0);
            glUniformMatrix4fv(1, 1, GL_FALSE, camera.viewProjection.valuePtr());

            glBindVertexArray(dd->vao);
            glDrawArrays(GL_LINES, 0, (GLsizei)dd->verts.size());
            dd->verts.clear();
        };
        rw->transparentPass({ shader, TransparentDepth::DEBUG_LINES, this, render });
    }
};
