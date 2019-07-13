#include "track.h"
#include "scene.h"
#include "renderable.h"
#include "renderer.h"
#include "mesh_renderables.h"
#include "input.h"
#include "game.h"

glm::vec4 red = glm::vec4(1, 0, 0, 1);
glm::vec4 orange = glm::vec4(1.0, 0.5, 0, 1);

void Track::onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime)
{
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
    }
}

void Track::trackModeUpdate(Renderer* renderer, Scene* scene, f32 deltaTime, bool isMouseHandled)
{
    for (auto& c : connections)
    {
        scene->debugDraw.line(points[c.pointIndexA].position,
                points[c.pointIndexA].position + c.handleOffsetA,
                orange, orange);
        scene->debugDraw.line(points[c.pointIndexB].position,
                points[c.pointIndexB].position + c.handleOffsetB,
                orange, orange);

        glm::vec2 mousePos = g_input.getMousePosition();
        f32 radius = 20;

        glm::vec3 colorA = orange;
        glm::vec3 handleA = points[c.pointIndexA].position + c.handleOffsetA;
        glm::vec2 handleAScreen = project(handleA, renderer->getCamera(0).viewProjection)
            * glm::vec2(g_game.windowWidth, g_game.windowHeight);
        if (glm::length(handleAScreen - mousePos) < radius)
        {
            colorA *= 1.3f;
        }

        glm::vec3 colorB = orange;
        glm::vec3 handleB = points[c.pointIndexB].position + c.handleOffsetB;
        glm::vec2 handleBScreen = project(handleB, renderer->getCamera(0).viewProjection)
            * glm::vec2(g_game.windowWidth, g_game.windowHeight);
        if (glm::length(handleBScreen - mousePos) < radius)
        {
            colorB *= 1.3f;
        }

        Mesh* sphere = g_resources.getMesh("world.Sphere");
        renderer->push(OverlayRenderable(sphere, 0,
                    glm::translate(glm::mat4(1.f), points[c.pointIndexA].position) *
                    glm::scale(glm::mat4(1.f), glm::vec3(1.f)), red));
        renderer->push(OverlayRenderable(sphere, 0,
                    glm::translate(glm::mat4(1.f), points[c.pointIndexB].position) *
                    glm::scale(glm::mat4(1.f), glm::vec3(1.f)), red));
        renderer->push(OverlayRenderable(sphere, 0,
                    glm::translate(glm::mat4(1.f), handleA) *
                    glm::scale(glm::mat4(1.f), glm::vec3(1.f)), colorA));
        renderer->push(OverlayRenderable(sphere, 0,
                    glm::translate(glm::mat4(1.f), handleB) *
                    glm::scale(glm::mat4(1.f), glm::vec3(1.f)), colorB));
    }
}
