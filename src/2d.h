#pragma once

#include "renderable.h"
#include "renderer.h"

class QuadRenderable : public Renderable2D {
    struct Vertex
    {
        glm::vec2 xy;
        glm::vec2 uv;
    };

    Texture* tex;
    Vertex points[4];
    glm::vec4 color;

public:
    QuadRenderable(Texture* texture, glm::vec2 p1, glm::vec2 p2, glm::vec2 t1=glm::vec2(0), glm::vec2 t2=glm::vec2(1),
            glm::vec3 color=glm::vec3(1.0), f32 alpha=1.f) : tex(texture), color(glm::vec4(color, alpha))
    {
        points[0] = { p1, t1 };
        points[1] = { { p2.x, p1.y }, { t2.x, t1.y } };
        points[2] = { { p1.x, p2.y }, { t1.x, t2.y } };
        points[3] = { p2, t2 };
    }

    void on2DPass(class Renderer* renderer) override
    {
        glUseProgram(renderer->getShaderProgram("tex2D"));
        glBindTextureUnit(0, tex->handle);
        glUniform4fv(0, 4, (GLfloat*)&points);
        glUniform4fv(4, 1, (GLfloat*)&color);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
};
