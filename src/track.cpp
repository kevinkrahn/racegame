#include "track.h"
#include "scene.h"
#include "renderable.h"
#include "renderer.h"

void Track::onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime)
{
    glm::vec3 p1 = { 0, 0, 10 };
    glm::vec3 p2 = { 0, 30, 10 };
    glm::vec3 p3 = { 50, 50, 10 };
    glm::vec3 p4 = { 100, 100, 10 };
    glm::vec3 prevP = p1;
    glm::vec4 red = glm::vec4(1, 0, 0, 1);
    glm::vec4 orange = glm::vec4(1.0, 0.5, 0, 1);
    for (f32 t=0.01f; t<=1.f; t+=0.01f)
    {
        glm::vec3 p = getPointOnBezierCurve(p1, p2, p3, p4, t);
        scene->debugDraw.line(p, prevP, red, red);
        prevP = p;
    }
    scene->debugDraw.line(p1, p1 + glm::vec3(0, 0, 4), red, red);
    scene->debugDraw.line(p4, p4 + glm::vec3(0, 0, 4), red, red);
    scene->debugDraw.line(p2, p2 + glm::vec3(0, 0, 4), orange, orange);
    scene->debugDraw.line(p1, p2, orange, orange);
    scene->debugDraw.line(p3, p3 + glm::vec3(0, 0, 4), orange, orange);
    scene->debugDraw.line(p4, p3, orange, orange);
}

