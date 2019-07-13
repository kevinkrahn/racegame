#include "track.h"
#include "scene.h"
#include "renderable.h"
#include "renderer.h"
#include "mesh_renderables.h"
#include "input.h"
#include "game.h"

glm::vec4 red = { 1, 0, 0, 1 };
glm::vec4 brightRed = { 1.f, 0.25f, 0.25f, 1.f };
glm::vec4 orange = { 1.f, 0.5f, 0.f, 1.f };
glm::vec4 brightOrange = { 1.f, 0.65f, 0.1f, 1.f };

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

void Track::trackModeUpdate(Renderer* renderer, Scene* scene, f32 deltaTime, bool& isMouseHandled)
{
    Mesh* sphere = g_resources.getMesh("world.Sphere");
    glm::vec2 mousePos = g_input.getMousePosition();
    Camera const& cam = renderer->getCamera(0);
    glm::vec3 rayDir = screenToWorldRay(mousePos,
            glm::vec2(g_game.windowWidth, g_game.windowHeight), cam.view, cam.projection);
    f32 radius = 20;

    bool wasAnythingClickedOn = false;

    if (!isMouseHandled && g_input.isMouseButtonPressed(MOUSE_LEFT)
            && !g_input.isKeyDown(KEY_LCTRL) && !g_input.isKeyDown(KEY_LSHIFT))
    {
        for (auto& point : points)
        {
            point.isSelected = false;
        }
        lastSelectedPoint = -1;
    }

    for (i32 i=0; i<points.size(); ++i)
    {
        glm::vec3 point = points[i].position;
        glm::vec2 pointScreen = project(point, renderer->getCamera(0).viewProjection)
            * glm::vec2(g_game.windowWidth, g_game.windowHeight);

        if (glm::length(pointScreen - mousePos) < radius && !isDragging)
        {
            if (!isMouseHandled && g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                isMouseHandled = true;
                if (g_input.isKeyDown(KEY_LSHIFT))
                {
                    points[i].isSelected = false;
                }
                else
                {
                    selectMousePos = mousePos;
                    points[i].isSelected = true;
                    wasAnythingClickedOn = true;
                    lastSelectedPoint = i;
                }
            }
        }
        if (points[i].isSelected)
        {
            renderer->push(OverlayRenderable(sphere, 0,
                        glm::translate(glm::mat4(1.f), points[i].position) *
                        glm::scale(glm::mat4(1.08f), glm::vec3(1.f)), { 1, 1, 1 }));
        }
        glm::vec3 color = points[i].isSelected ? brightRed : red;
        renderer->push(OverlayRenderable(sphere, 0,
                    glm::translate(glm::mat4(1.f), points[i].position) *
                    glm::scale(glm::mat4(1.f), glm::vec3(1.f)), color));
    }

    if (lastSelectedPoint != -1 && g_input.isMouseButtonDown(MOUSE_LEFT))
    {
        if (glm::length(mousePos - selectMousePos) > g_game.windowHeight * 0.005f)
        {
            f32 t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1),
                    points[lastSelectedPoint].position);
            glm::vec3 hitPoint = cam.position + rayDir * t;
            if (!isDragging)
            {
                dragStartPoint = hitPoint;
            }
            for (auto& point : points)
            {
                if (point.isSelected)
                {
                    if (!isDragging)
                    {
                        point.dragStartPoint = point.position;
                    }
                    point.position = point.dragStartPoint + (hitPoint - dragStartPoint);
                }
            }
            isDragging = true;
        }
    }

    for (i32 i=0; i<connections.size(); ++i)
    {
        BezierSegment& c = connections[i];

        glm::vec3 colorA = orange;
        glm::vec3 handleA = points[c.pointIndexA].position + c.handleOffsetA;
        glm::vec2 handleAScreen = project(handleA, renderer->getCamera(0).viewProjection)
            * glm::vec2(g_game.windowWidth, g_game.windowHeight);
        if (!isDragging && glm::length(handleAScreen - mousePos) < radius)
        {
            colorA = brightOrange;
            if (!isMouseHandled && g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                isMouseHandled = true;
                dragConnectionIndex = i;
                dragConnectionHandle = 0;
                f32 t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1), handleA);
                glm::vec3 p = cam.position + rayDir * t;
                dragOffset = handleA - p;
                isDragging = true;
            }
        }
        if (dragConnectionIndex == i && dragConnectionHandle == 0)
        {
            colorA = brightOrange;
            f32 t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1), handleA);
            glm::vec3 p = cam.position + rayDir * t + dragOffset;
            c.handleOffsetA = p - points[c.pointIndexA].position;
            handleA = points[c.pointIndexA].position + c.handleOffsetA;
            for (auto& c2 : connections)
            {
                if (&c2 != &c && c2.pointIndexB == c.pointIndexA)
                {
                    c2.handleOffsetB = -c.handleOffsetA;
                }
            }
            renderer->push(OverlayRenderable(sphere, 0,
                        glm::translate(glm::mat4(1.f), handleA) *
                        glm::scale(glm::mat4(0.9f), glm::vec3(1.f)), { 1, 1, 1 }));
        }

        glm::vec3 colorB = orange;
        glm::vec3 handleB = points[c.pointIndexB].position + c.handleOffsetB;
        glm::vec2 handleBScreen = project(handleB, renderer->getCamera(0).viewProjection)
            * glm::vec2(g_game.windowWidth, g_game.windowHeight);
        if (!isDragging && glm::length(handleBScreen - mousePos) < radius)
        {
            colorB = brightOrange;
            if (!isMouseHandled && g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                isMouseHandled = true;
                dragConnectionIndex = i;
                dragConnectionHandle = 1;
                f32 t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1), handleB);
                glm::vec3 p = cam.position + rayDir * t;
                dragOffset = handleB - p;
                isDragging = true;
            }
        }
        if (dragConnectionIndex == i && dragConnectionHandle == 1)
        {
            colorB = brightOrange;
            f32 t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1), handleB);
            glm::vec3 p = cam.position + rayDir * t + dragOffset;
            c.handleOffsetB = p - points[c.pointIndexB].position;
            handleB = points[c.pointIndexB].position + c.handleOffsetB;
            for (auto& c2 : connections)
            {
                if (&c2 != &c && c2.pointIndexA == c.pointIndexB)
                {
                    c2.handleOffsetA = -c.handleOffsetB;
                }
            }
            renderer->push(OverlayRenderable(sphere, 0,
                        glm::translate(glm::mat4(1.f), handleB) *
                        glm::scale(glm::mat4(0.9f), glm::vec3(1.f)), { 1, 1, 1 }));
        }
        renderer->push(OverlayRenderable(sphere, 0,
                    glm::translate(glm::mat4(1.f), handleA) *
                    glm::scale(glm::mat4(0.8f), glm::vec3(1.f)), colorA));
        renderer->push(OverlayRenderable(sphere, 0,
                    glm::translate(glm::mat4(1.f), handleB) *
                    glm::scale(glm::mat4(0.8f), glm::vec3(1.f)), colorB));

        scene->debugDraw.line(points[c.pointIndexA].position,
                points[c.pointIndexA].position + c.handleOffsetA,
                glm::vec4(colorA, 1.f), glm::vec4(colorA, 1.f));
        scene->debugDraw.line(points[c.pointIndexB].position,
                points[c.pointIndexB].position + c.handleOffsetB,
                glm::vec4(colorB, 1.f), glm::vec4(colorB, 1.f));

    }

    if (g_input.isMouseButtonReleased(MOUSE_LEFT))
    {
        dragConnectionIndex = -1;
        dragConnectionHandle = -1;
        isDragging = false;
        lastSelectedPoint = -1;
    }
}
