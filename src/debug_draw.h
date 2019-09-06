#pragma once

#include "math.h"
#include "dynamic_buffer.h"
#include "renderable.h"
#include "bounding_box.h"
#include "renderer.h"

class DebugDraw : public Renderable
{
    struct Vertex
    {
        glm::vec3 position;
        glm::vec4 color;
    };
    std::vector<Vertex> verts;
    DynamicBuffer buffer = DynamicBuffer(sizeof(Vertex) * 300000);
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

    i32 getPriority() const override { return 1000000; }

    void line(glm::vec3 const& p1, glm::vec3 const& p2,
            glm::vec4 const& c1 = glm::vec4(1), glm::vec4 const& c2 = glm::vec4(1))
    {
        verts.push_back({ p1, c1 });
        verts.push_back({ p2, c2 });
    }

    void boundingBox(BoundingBox const& bb, glm::mat4 const& transform, glm::vec4 const& color)
    {
        glm::vec3 p[8] = {
            glm::vec3(transform * glm::vec4(bb.min.x, bb.min.y, bb.min.z, 1.f)),
            glm::vec3(transform * glm::vec4(bb.max.x, bb.min.y, bb.min.z, 1.f)),
            glm::vec3(transform * glm::vec4(bb.max.x, bb.max.y, bb.min.z, 1.f)),
            glm::vec3(transform * glm::vec4(bb.min.x, bb.max.y, bb.min.z, 1.f)),
            glm::vec3(transform * glm::vec4(bb.min.x, bb.min.y, bb.max.z, 1.f)),
            glm::vec3(transform * glm::vec4(bb.max.x, bb.min.y, bb.max.z, 1.f)),
            glm::vec3(transform * glm::vec4(bb.max.x, bb.max.y, bb.max.z, 1.f)),
            glm::vec3(transform * glm::vec4(bb.min.x, bb.max.y, bb.max.z, 1.f)),
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

    void onLitPass(Renderer* renderer) override
    {
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_CULL_FACE);

        if (verts.size() > 0)
        {
            Camera const& camera = renderer->getCamera(0);
            buffer.updateData(verts.data(), verts.size() * sizeof(Vertex));
            glVertexArrayVertexBuffer(vao, 0, buffer.getBuffer(), 0, sizeof(Vertex));

            glUseProgram(renderer->getShaderProgram("debug"));
            glUniform1i(3, 0);
            glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(camera.viewProjection));

            glBindVertexArray(vao);
            glDrawArrays(GL_LINES, 0, verts.size());
            verts.clear();
        }
    }

    std::string getDebugString() const override { return "DebugDraw"; };
};
