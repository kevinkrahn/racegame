#pragma once

#include "renderable.h"
#include "renderer.h"

class QuadRenderable : public Renderable2D
{
    struct Vertex
    {
        glm::vec2 xy;
        glm::vec2 uv;
    };

    Texture* tex;
    Vertex points[4];
    glm::vec4 color;
    ShaderHandle shaderHandle = getShaderHandle("quad2D", { {"COLOR"} });

public:
    QuadRenderable(Texture* texture, glm::vec2 p1, glm::vec2 p2, glm::vec2 t1=glm::vec2(0), glm::vec2 t2=glm::vec2(1),
            glm::vec3 color=glm::vec3(1.f), f32 alpha=1.f) : tex(texture), color(glm::vec4(color, alpha))
    {
        points[0] = { p1, t1 };
        points[1] = { { p2.x, p1.y }, { t2.x, t1.y } };
        points[2] = { { p1.x, p2.y }, { t1.x, t2.y } };
        points[3] = { p2, t2 };
    }

    QuadRenderable(Texture* texture, glm::vec2 p1, f32 width, f32 height,
            glm::vec3 color=glm::vec3(1.f), f32 alpha=1.f, bool blurBackground=false, bool mirror=false)
        : tex(texture), color(glm::vec4(color, alpha))
    {
        glm::vec2 t1(mirror ? 1.f : 0.f);
        glm::vec2 t2(mirror ? 0.f : 1.f);
        glm::vec2 p2 = p1 + glm::vec2(width, height);
        points[0] = { p1, t1 };
        points[1] = { { p2.x, p1.y }, { t2.x, t1.y } };
        points[2] = { { p1.x, p2.y }, { t1.x, t2.y } };
        points[3] = { p2, t2 };
        this->shaderHandle = getShaderHandle("quad2D", { {blurBackground ? "BLUR" : "COLOR" } });
    }

    QuadRenderable(Texture* texture, glm::vec2 p1, f32 width, f32 height,
            glm::vec2 t1, glm::vec2 t2, glm::vec3 color=glm::vec3(1.f), f32 alpha=1.f)
        : tex(texture), color(glm::vec4(color, alpha))
    {
        glm::vec2 p2 = p1 + glm::vec2(width, height);
        points[0] = { p1, t1 };
        points[1] = { { p2.x, p1.y }, { t2.x, t1.y } };
        points[2] = { { p1.x, p2.y }, { t1.x, t2.y } };
        points[3] = { p2, t2 };
    }

    void on2DPass(Renderer* renderer) override
    {
        glUseProgram(renderer->getShaderProgram(shaderHandle));
        glBindTextureUnit(1, tex->handle);
        glUniform4fv(0, 4, (GLfloat*)&points);
        glUniform4fv(4, 1, (GLfloat*)&color);
        glUniform1f(5, 1.f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
};

class Quad : public Renderable2D
{
    struct Vertex
    {
        glm::vec2 xy;
        glm::vec2 uv;
    };

    Texture* tex;
    Vertex points[4];
    glm::vec4 color;
    f32 alpha = 1.f;
    ShaderHandle shaderHandle = getShaderHandle("quad2D", { {"BLUR"} });

public:
    Quad(Texture* texture, glm::vec2 p1, glm::vec2 p2, glm::vec2 t1=glm::vec2(0), glm::vec2 t2=glm::vec2(1),
            glm::vec4 color=glm::vec4(1.f), f32 alpha=1.f) : tex(texture), color(color), alpha(alpha)
    {
        points[0] = { p1, t1 };
        points[1] = { { p2.x, p1.y }, { t2.x, t1.y } };
        points[2] = { { p1.x, p2.y }, { t1.x, t2.y } };
        points[3] = { p2, t2 };
    }

    Quad(Texture* texture, glm::vec2 p1, f32 width, f32 height,
            glm::vec4 color=glm::vec4(1.f), f32 alpha=1.f, bool mirror=false)
        : tex(texture), color(color), alpha(alpha)
    {
        glm::vec2 t1(mirror ? 1.f : 0.f);
        glm::vec2 t2(mirror ? 0.f : 1.f);
        glm::vec2 p2 = p1 + glm::vec2(width, height);
        points[0] = { p1, t1 };
        points[1] = { { p2.x, p1.y }, { t2.x, t1.y } };
        points[2] = { { p1.x, p2.y }, { t1.x, t2.y } };
        points[3] = { p2, t2 };
    }

    Quad(Texture* texture, glm::vec2 p1, f32 width, f32 height,
            glm::vec2 t1, glm::vec2 t2, glm::vec4 color=glm::vec4(1.f), f32 alpha=1.f)
        : tex(texture), color(color), alpha(alpha)
    {
        glm::vec2 p2 = p1 + glm::vec2(width, height);
        points[0] = { p1, t1 };
        points[1] = { { p2.x, p1.y }, { t2.x, t1.y } };
        points[2] = { { p1.x, p2.y }, { t1.x, t2.y } };
        points[3] = { p2, t2 };
    }

    void on2DPass(Renderer* renderer) override
    {
        glUseProgram(renderer->getShaderProgram(shaderHandle));
        glBindTextureUnit(1, tex->handle);
        glUniform4fv(0, 4, (GLfloat*)&points);
        glUniform4fv(4, 1, (GLfloat*)&color);
        glUniform1f(5, alpha);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
};
