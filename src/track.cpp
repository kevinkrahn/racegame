#include "track.h"
#include "scene.h"
#include "renderable.h"
#include "renderer.h"

void Track::onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime)
{
    glm::vec4 red = glm::vec4(1, 0, 0, 1);
    glm::vec4 orange = glm::vec4(1.0, 0.5, 0, 1);
    for (auto& c : connections)
    {
        glm::vec3 prevP;
        for (f32 t=0.f; t<=1.f; t+=0.01f)
        {
            glm::vec3 p = getPointOnBezierCurve(
                    points[c.pointIndexA].position,
                    points[c.pointIndexA].position + c.handleOffsetA,
                    points[c.pointIndexB].position + c.handleOffsetB,
                    points[c.pointIndexB].position, t);
            if (t > 0.f)
            {
                scene->debugDraw.line(p, prevP, red, red);
            }
            prevP = p;
        }

        scene->debugDraw.line(points[c.pointIndexA].position,
                points[c.pointIndexA].position + glm::vec3(0, 0, 4),
                red, red);
        scene->debugDraw.line(points[c.pointIndexB].position,
                points[c.pointIndexB].position + glm::vec3(0, 0, 4),
                red, red);
        scene->debugDraw.line(points[c.pointIndexA].position + c.handleOffsetA,
                points[c.pointIndexA].position + c.handleOffsetA + glm::vec3(0, 0, 4),
                orange, orange);
        scene->debugDraw.line(points[c.pointIndexA].position,
                points[c.pointIndexA].position + c.handleOffsetA,
                orange, orange);
        scene->debugDraw.line(points[c.pointIndexB].position + c.handleOffsetB,
                points[c.pointIndexB].position + c.handleOffsetB + glm::vec3(0, 0, 4),
                orange, orange);
        scene->debugDraw.line(points[c.pointIndexB].position,
                points[c.pointIndexB].position + c.handleOffsetB,
                orange, orange);
    }
}

