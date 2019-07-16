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
        /*
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
        */
        if (c.isDirty || c.vertices.empty())
        {
            createSegmentMesh(c);
        }
    }
    renderer->add(this);
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
        selectedPoints.clear();
    }

    for (i32 i=0; i<points.size(); ++i)
    {
        glm::vec3 point = points[i].position;
        glm::vec2 pointScreen = project(point, renderer->getCamera(0).viewProjection)
            * glm::vec2(g_game.windowWidth, g_game.windowHeight);

        auto it = std::find_if(selectedPoints.begin(), selectedPoints.end(), [&i](Selection& s) -> bool {
            return s.pointIndex == i;
        });
        bool isSelected = it != selectedPoints.end();
        if (glm::length(pointScreen - mousePos) < radius && !isDragging)
        {
            if (!isMouseHandled && g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                isMouseHandled = true;
                if (g_input.isKeyDown(KEY_LSHIFT))
                {
                    if (it != selectedPoints.end())
                    {
                        selectedPoints.erase(it);
                    }
                }
                else
                {
                    selectMousePos = mousePos;
                    if (!isSelected)
                    {
                        selectedPoints.push_back({ i, {} });
                    }
                    wasAnythingClickedOn = true;
                }
            }
        }
        if (isSelected)
        {
            renderer->push(OverlayRenderable(sphere, 0,
                        glm::translate(glm::mat4(1.f), points[i].position) *
                        glm::scale(glm::mat4(1.08f), glm::vec3(1.f)), { 1, 1, 1 }, -1));
        }
        glm::vec3 color = isSelected ? brightRed : red;
        renderer->push(OverlayRenderable(sphere, 0,
                    glm::translate(glm::mat4(1.f), points[i].position) *
                    glm::scale(glm::mat4(1.f), glm::vec3(1.f)), color));
    }

    if (selectedPoints.size() > 0 && g_input.isMouseButtonDown(MOUSE_LEFT))
    {
        if (glm::length(mousePos - selectMousePos) > g_game.windowHeight * 0.005f)
        {
            f32 t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1),
                    points[selectedPoints.back().pointIndex].position);
            glm::vec3 hitPoint = cam.position + rayDir * t;
            if (!isDragging)
            {
                dragStartPoint = hitPoint;
            }
            for (auto& s : selectedPoints)
            {
                if (!isDragging)
                {
                    s.dragStartPoint = points[s.pointIndex].position;
                    s.dragStartPoint.z = points[selectedPoints.back().pointIndex].position.z;
                }
                points[s.pointIndex].position = s.dragStartPoint + (hitPoint - dragStartPoint);
            }
            isDragging = true;
        }
    }

    for (i32 i=0; i<connections.size(); ++i)
    {
        BezierSegment& c = connections[i];
        if (isDragging)
        {
            auto it = std::find_if(selectedPoints.begin(), selectedPoints.end(), [&c](Selection& s) -> bool {
                return s.pointIndex == c.pointIndexA || s.pointIndex == c.pointIndexB;
            });
            if (it != selectedPoints.end())
            {
                c.isDirty = true;
            }
        }

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
            c.isDirty = true;
            colorA = brightOrange;
            f32 t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1), handleA);
            glm::vec3 p = cam.position + rayDir * t + dragOffset;
            c.handleOffsetA = p - points[c.pointIndexA].position;
            handleA = points[c.pointIndexA].position + c.handleOffsetA;
            for (auto& c2 : connections)
            {
                if (&c2 != &c)
                {
                    if (c2.pointIndexB == c.pointIndexA)
                    {
                        c2.handleOffsetB = -c.handleOffsetA;
                        c2.isDirty = true;
                    }
                    else if (c2.pointIndexA == c.pointIndexA)
                    {
                        c2.handleOffsetA = -c.handleOffsetA;
                        c2.isDirty = true;
                    }
                }
            }
            renderer->push(OverlayRenderable(sphere, 0,
                        glm::translate(glm::mat4(1.f), handleA) *
                        glm::scale(glm::mat4(0.9f), glm::vec3(1.f)), { 1, 1, 1 }, -1));
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
            c.isDirty = true;
            colorB = brightOrange;
            f32 t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1), handleB);
            glm::vec3 p = cam.position + rayDir * t + dragOffset;
            c.handleOffsetB = p - points[c.pointIndexB].position;
            handleB = points[c.pointIndexB].position + c.handleOffsetB;
            for (auto& c2 : connections)
            {
                if (&c2 != &c)
                {
                    if (c2.pointIndexA == c.pointIndexB)
                    {
                        c2.handleOffsetA = -c.handleOffsetB;
                        c2.isDirty = true;
                    }
                    else if (c2.pointIndexB == c.pointIndexB)
                    {
                        c2.handleOffsetB = -c.handleOffsetB;
                        c2.isDirty = true;
                    }
                }
            }
            renderer->push(OverlayRenderable(sphere, 0,
                        glm::translate(glm::mat4(1.f), handleB) *
                        glm::scale(glm::mat4(0.9f), glm::vec3(1.f)), { 1, 1, 1 }, -1));
        }
        renderer->push(OverlayRenderable(sphere, 0,
                    glm::translate(glm::mat4(1.f), handleA) *
                    glm::scale(glm::mat4(0.8f), glm::vec3(1.f)), colorA));
        renderer->push(OverlayRenderable(sphere, 0,
                    glm::translate(glm::mat4(1.f), handleB) *
                    glm::scale(glm::mat4(0.8f), glm::vec3(1.f)), colorB));

        scene->debugDraw.line(points[c.pointIndexA].position + glm::vec3(0, 0, 0.01f),
                points[c.pointIndexA].position + c.handleOffsetA + glm::vec3(0, 0, 0.01f),
                glm::vec4(colorA, 1.f), glm::vec4(colorA, 1.f));
        scene->debugDraw.line(points[c.pointIndexB].position + glm::vec3(0, 0, 0.01f),
                points[c.pointIndexB].position + c.handleOffsetB + glm::vec3(0, 0, 0.01f),
                glm::vec4(colorB, 1.f), glm::vec4(colorB, 1.f));
    }

    if (g_input.isMouseButtonReleased(MOUSE_LEFT))
    {
        dragConnectionIndex = -1;
        dragConnectionHandle = -1;
        isDragging = false;
    }
}

Track::BezierSegment* Track::getPointConnection(u32 pointIndex)
{
    for (auto& c : connections)
    {
        if (c.pointIndexA == pointIndex || c.pointIndexB == pointIndex)
        {
            return &c;
        }
    }
    return nullptr;
}

glm::vec3 Track::getPointDir(u32 pointIndex)
{
    glm::vec3 dir;
    u32 count = 0;
    for (auto& c : connections)
    {
        if (c.pointIndexA == pointIndex)
        {
            ++count;
            if (count > 1)
            {
                break;
            }
            dir = -glm::normalize(
                    getPointOnBezierCurve(
                        points[c.pointIndexA].position,
                        points[c.pointIndexA].position + c.handleOffsetA,
                        points[c.pointIndexB].position + c.handleOffsetB,
                        points[c.pointIndexB].position, 0.01f) -
                    points[c.pointIndexA].position);
        }
        else if (c.pointIndexB == pointIndex)
        {
            ++count;
            if (count > 1)
            {
                break;
            }
            dir = -glm::normalize(
                    getPointOnBezierCurve(
                        points[c.pointIndexA].position,
                        points[c.pointIndexA].position + c.handleOffsetA,
                        points[c.pointIndexB].position + c.handleOffsetB,
                        points[c.pointIndexB].position, 0.99f) -
                    points[c.pointIndexB].position);
        }
    }
    if (count == 1)
    {
        return dir;
    }
    return { 0, 0, 0 };
}

void Track::createSegmentMesh(BezierSegment& c)
{
    c.isDirty = false;

    if (c.vertices.empty())
    {
        glCreateBuffers(1, &c.vbo);
        glCreateBuffers(1, &c.ebo);

        enum
        {
            POSITION_BIND_INDEX = 0,
            NORMAL_BIND_INDEX = 1,
            COLOR_BIND_INDEX = 2
        };

        glCreateVertexArrays(1, &c.vao);

        glEnableVertexArrayAttrib(c.vao, POSITION_BIND_INDEX);
        glVertexArrayAttribFormat(c.vao, POSITION_BIND_INDEX, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(c.vao, POSITION_BIND_INDEX, 0);

        glEnableVertexArrayAttrib(c.vao, NORMAL_BIND_INDEX);
        glVertexArrayAttribFormat(c.vao, NORMAL_BIND_INDEX, 3, GL_FLOAT, GL_FALSE, 12);
        glVertexArrayAttribBinding(c.vao, NORMAL_BIND_INDEX, 0);

        glEnableVertexArrayAttrib(c.vao, COLOR_BIND_INDEX);
        glVertexArrayAttribFormat(c.vao, COLOR_BIND_INDEX, 3, GL_FLOAT, GL_FALSE, 12 + 12);
        glVertexArrayAttribBinding(c.vao, COLOR_BIND_INDEX, 0);
    }

    f32 totalLength = 0.f;
    glm::vec3 prevP = points[c.pointIndexA].position;
    for (u32 i=1; i<=10; ++i)
    {
        glm::vec3 p = getPointOnBezierCurve(
                points[c.pointIndexA].position,
                points[c.pointIndexA].position + c.handleOffsetA,
                points[c.pointIndexB].position + c.handleOffsetB,
                points[c.pointIndexB].position, i / 10.f);
        totalLength += glm::length(prevP - p);
        prevP = p;
    }

    c.vertices.clear();
    c.indices.clear();
    f32 stepSize = 1.f;
    u32 totalSteps = totalLength / stepSize;
    prevP = points[c.pointIndexA].position;
    for (u32 i=0; i<=totalSteps; ++i)
    {
        glm::vec3 p = getPointOnBezierCurve(
                points[c.pointIndexA].position,
                points[c.pointIndexA].position + c.handleOffsetA,
                points[c.pointIndexB].position + c.handleOffsetB,
                points[c.pointIndexB].position, i / (f32)totalSteps);
        glm::vec3 xDir = glm::normalize(i == 0 ? c.handleOffsetA :
                (i == totalSteps ? -c.handleOffsetB : glm::normalize(p - prevP)));
        glm::vec3 yDir = glm::cross(xDir, glm::vec3(0, 0, 1));
        glm::vec3 zDir = glm::cross(yDir, xDir);
        f32 width = 6.f;
        glm::vec3 p1 = p + yDir * width;
        glm::vec3 p2 = p - yDir * width;
        glm::vec3 color = { 0, 0, 0 };
        c.vertices.push_back(Vertex{ p1, zDir, color });
        c.vertices.push_back(Vertex{ p2, zDir, color });
        if (i > 0)
        {
            c.indices.push_back(i * 2 - 1);
            c.indices.push_back(i * 2 - 2);
            c.indices.push_back(i * 2);
            c.indices.push_back(i * 2);
            c.indices.push_back(i * 2 + 1);
            c.indices.push_back(i * 2 - 1);
        }
        prevP = p;
    }

    glBindVertexArray(c.vao);
    glNamedBufferData(c.vbo, c.vertices.size() * sizeof(Vertex), c.vertices.data(), GL_DYNAMIC_DRAW);
    glNamedBufferData(c.ebo, c.indices.size() * sizeof(u32), c.indices.data(), GL_DYNAMIC_DRAW);
    glVertexArrayVertexBuffer(c.vao, 0, c.vbo, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(c.vao, c.ebo);
}

void Track::onShadowPass(class Renderer* renderer)
{
    glUseProgram(renderer->getShaderProgram("track"));
    for (auto& c : connections)
    {
        glBindVertexArray(c.vao);
        glDrawElements(GL_TRIANGLES, c.indices.size(), GL_UNSIGNED_INT, 0);
    }
}

void Track::onDepthPrepass(class Renderer* renderer)
{
    glUseProgram(renderer->getShaderProgram("track"));
    for (auto& c : connections)
    {
        glBindVertexArray(c.vao);
        glDrawElements(GL_TRIANGLES, c.indices.size(), GL_UNSIGNED_INT, 0);
    }
}

void Track::onLitPass(class Renderer* renderer)
{
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_EQUAL);
    glEnable(GL_CULL_FACE);
    glBindTextureUnit(0, g_resources.getTexture("tarmac")->handle);
    glUseProgram(renderer->getShaderProgram("track"));
    for (auto& c : connections)
    {
        glBindVertexArray(c.vao);
        glDrawElements(GL_TRIANGLES, c.indices.size(), GL_UNSIGNED_INT, 0);
    }
}
