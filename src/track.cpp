#include "track.h"
#include "scene.h"
#include "renderable.h"
#include "renderer.h"
#include "mesh_renderables.h"
#include "input.h"
#include "game.h"
#include "track_graph.h"
#include "2d.h"

glm::vec4 red = { 1.f, 0.f, 0.f, 1.f };
glm::vec4 brightRed = { 1.f, 0.25f, 0.25f, 1.f };
glm::vec4 orange = { 1.f, 0.5f, 0.f, 1.f };
glm::vec4 brightOrange = { 1.f, 0.65f, 0.1f, 1.f };
glm::vec4 blue = { 0.f, 0.0f, 1.f, 1.f };
glm::vec4 brightBlue = { 0.25f, 0.25f, 1.0f, 1.f };

void Track::onCreate(Scene* scene)
{
    actor = g_game.physx.physics->createRigidStatic(PxTransform(PxIdentity));
    ActorUserData* userData = new ActorUserData;
    userData->entityType = ActorUserData::TRACK;
    userData->entity = this;
    physicsUserData.reset(userData);
    actor->userData = userData;
    scene->getPhysicsScene()->addActor(*actor);
}

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
                scene->debugDraw.line(p, prevP, orange, orange);
            }
            prevP = p;
        }
        */
        if (c.isDirty || c.vertices.empty())
        {
            createSegmentMesh(c, scene);
        }
    }

    for (auto const& railing : railings)
    {
        for (size_t i=1; i<railing.points.size(); ++i)
        {
            RailingPoint const& point = railing.points[i];
            RailingPoint const& prevPoint = railing.points[i-1];
            glm::vec3 prevP;
            for (f32 t=0.f; t<=1.f; t+=0.01f)
            {
                glm::vec3 p = getPointOnBezierCurve(
                        prevPoint.position,
                        prevPoint.position + prevPoint.handleOffsetB,
                        point.position + point.handleOffsetA,
                        point.position, t);
                if (t > 0.f)
                {
                    scene->debugDraw.line(p, prevP, blue, blue);
                }
                prevP = p;
            }
        }
    }

    renderer->add(this);
    renderer->push(LitRenderable(g_resources.getMesh("world.Cube.002"), getStart(), nullptr));
}

void Track::clearSelection()
{
    selectedPoints.clear();
    for (auto& railing : railings)
    {
        railing.selectedPoints.clear();
    }
}

void Track::trackModeUpdate(Renderer* renderer, Scene* scene, f32 deltaTime, bool& isMouseHandled, GridSettings* gridSettings)
{
    Mesh* sphere = g_resources.getMesh("world.Sphere");
    glm::vec2 mousePos = g_input.getMousePosition();
    Camera const& cam = renderer->getCamera(0);
    glm::vec3 rayDir = screenToWorldRay(mousePos,
            glm::vec2(g_game.windowWidth, g_game.windowHeight), cam.view, cam.projection);
    f32 radius = 18;

    if (!isMouseHandled && g_input.isMouseButtonPressed(MOUSE_LEFT)
            && !g_input.isKeyDown(KEY_LCTRL) && !g_input.isKeyDown(KEY_LSHIFT))
    {
        clearSelection();
    }

    // track points
    for (i32 i=0; i<points.size();)
    {
        glm::vec3 point = points[i].position;
        glm::vec2 pointScreen = project(point, renderer->getCamera(0).viewProjection)
            * glm::vec2(g_game.windowWidth, g_game.windowHeight);

        auto it = std::find_if(selectedPoints.begin(), selectedPoints.end(), [&i](Selection& s) -> bool {
            return s.pointIndex == i;
        });
        bool isSelected = it != selectedPoints.end();

        if (isSelected)
        {
            i32 d = (i32)g_input.isKeyPressed(KEY_Q) - (i32)g_input.isKeyPressed(KEY_E);
            if (d != 0)
            {
                points[i].position.z += 2.f * d;
                for (auto& c : connections)
                {
                    if (c.pointIndexA == i || c.pointIndexB == i)
                    {
                        c.isDirty = true;
                    }
                }
            }

            if (points.size() > 1 && connections.size() > 1 && g_input.isKeyPressed(KEY_DELETE))
            {
                for (auto it = connections.begin(); it != connections.end();)
                {
                    if (it->pointIndexA == i || it->pointIndexB == i)
                    {
                        it->destroy(this);
                        connections.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
                selectedPoints.erase(it);
                points.erase(points.begin() + i);
                for (auto& conn : connections)
                {
                    if (conn.pointIndexA > i)
                    {
                        --conn.pointIndexA;
                    }
                    if (conn.pointIndexB > i)
                    {
                        --conn.pointIndexB;
                    }
                }
                continue;
            }
        }

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
        ++i;
    }

    // track connections
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
                for (i32 connectionIndex = 0; connectionIndex < connections.size(); ++connectionIndex)
                {
                    BezierSegment const& c2 = connections[connectionIndex];
                    if (&c2 != &c)
                    {
                        if (c2.pointIndexB == c.pointIndexA)
                        {
                            if (1.f - glm::dot(-glm::normalize(c2.handleOffsetB),
                                        glm::normalize(c.handleOffsetA)) < 0.01f)
                            {
                                dragOppositeConnectionIndex = connectionIndex;
                                dragOppositeConnectionHandle = 1;
                                break;
                            }
                        }
                        else if (c2.pointIndexA == c.pointIndexA)
                        {
                            if (1.f - glm::dot(-glm::normalize(c2.handleOffsetA),
                                        glm::normalize(c.handleOffsetA)) < 0.01f)
                            {
                                dragOppositeConnectionIndex = connectionIndex;
                                dragOppositeConnectionHandle = 0;
                                break;
                            }
                        }
                    }
                }
            }
        }
        if (dragRailingIndex == -1 && dragConnectionIndex == i && dragConnectionHandle == 0)
        {
            c.isDirty = true;
            colorA = brightOrange;
            f32 t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1), handleA);
            glm::vec3 p = cam.position + rayDir * t + dragOffset;
            c.handleOffsetA = p - points[c.pointIndexA].position;
            handleA = points[c.pointIndexA].position + c.handleOffsetA;
            if (dragOppositeConnectionIndex != -1)
            {
                BezierSegment& c2 = connections[dragOppositeConnectionIndex];
                c2.isDirty = true;
                if (dragOppositeConnectionHandle == 0)
                {
                    c2.handleOffsetA = -c.handleOffsetA;
                }
                else if (dragOppositeConnectionHandle == 1)
                {
                    c2.handleOffsetB = -c.handleOffsetA;
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
                for (i32 connectionIndex = 0; connectionIndex < connections.size(); ++connectionIndex)
                {
                    BezierSegment const& c2 = connections[connectionIndex];
                    if (&c2 == &c)
                    {
                        continue;
                    }
                    if (c2.pointIndexB == c.pointIndexB)
                    {
                        if (1.f - glm::dot(-glm::normalize(c2.handleOffsetB),
                                    glm::normalize(c.handleOffsetB)) < 0.01f)
                        {
                            dragOppositeConnectionIndex = connectionIndex;
                            dragOppositeConnectionHandle = 1;
                            break;
                        }
                    }
                    else if (c2.pointIndexA == c.pointIndexB)
                    {
                        if (1.f - glm::dot(-glm::normalize(c2.handleOffsetA),
                                    glm::normalize(c.handleOffsetB)) < 0.01f)
                        {
                            dragOppositeConnectionIndex = connectionIndex;
                            dragOppositeConnectionHandle = 0;
                            break;
                        }
                    }
                }
            }
        }
        if (dragRailingIndex == -1 && dragConnectionIndex == i && dragConnectionHandle == 1)
        {
            c.isDirty = true;
            colorB = brightOrange;
            f32 t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1), handleB);
            glm::vec3 p = cam.position + rayDir * t + dragOffset;
            c.handleOffsetB = p - points[c.pointIndexB].position;
            handleB = points[c.pointIndexB].position + c.handleOffsetB;
            if (dragOppositeConnectionIndex != -1)
            {
                BezierSegment& c2 = connections[dragOppositeConnectionIndex];
                c2.isDirty = true;
                if (dragOppositeConnectionHandle == 0)
                {
                    c2.handleOffsetA = -c.handleOffsetB;
                }
                else if (dragOppositeConnectionHandle == 1)
                {
                    c2.handleOffsetB = -c.handleOffsetB;
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

    // railing points
    for (auto rit = railings.begin(); rit != railings.end();)
    {
        for (auto it = rit->points.begin(); it != rit->points.end();)
        {
            auto found = std::find_if(rit->selectedPoints.begin(), rit->selectedPoints.end(), [&](Selection& s) -> bool {
                return s.pointIndex == (i32)(it - rit->points.begin());
            });
            bool isSelected = (found != rit->selectedPoints.end());

            if (isSelected)
            {
                i32 d = (i32)g_input.isKeyPressed(KEY_Q) - (i32)g_input.isKeyPressed(KEY_E);
                if (d != 0)
                {
                    it->position.z += 2.f * d;
                }

                if (g_input.isKeyPressed(KEY_DELETE))
                {
                    rit->selectedPoints.erase(std::remove_if(
                                rit->selectedPoints.begin(), rit->selectedPoints.end(),
                                [&](auto& s) {
                                    return s.pointIndex == (i32)(it - rit->points.begin());
                                }));
                    rit->points.erase(it);
                    continue;
                }
            }

            glm::vec3 point = it->position;
            glm::vec2 pointScreen = project(point, renderer->getCamera(0).viewProjection)
                * glm::vec2(g_game.windowWidth, g_game.windowHeight);

            if (glm::length(pointScreen - mousePos) < radius && !isDragging)
            {
                if (!isMouseHandled && g_input.isMouseButtonPressed(MOUSE_LEFT))
                {
                    isMouseHandled = true;
                    if (g_input.isKeyDown(KEY_LSHIFT))
                    {
                        if (isSelected)
                        {
                            rit->selectedPoints.erase(found);
                        }
                    }
                    else
                    {
                        selectMousePos = mousePos;
                        if (!isSelected)
                        {
                            rit->selectedPoints.push_back({ (i32)(it - rit->points.begin()), {} });
                        }
                    }
                }
            }
            if (isSelected)
            {
                renderer->push(OverlayRenderable(sphere, 0,
                            glm::translate(glm::mat4(1.f), it->position) *
                            glm::scale(glm::mat4(0.78f), glm::vec3(1.f)), { 1, 1, 1 }, -1));
            }
            glm::vec3 color = isSelected ? brightBlue : blue;
            renderer->push(OverlayRenderable(sphere, 0,
                        glm::translate(glm::mat4(1.f), it->position) *
                        glm::scale(glm::mat4(0.7f), glm::vec3(1.f)), color));

            i32 currentRailingIndex = (i32)(rit - railings.begin());
            i32 currentPointIndex = (i32)(it - rit->points.begin());

            glm::vec3 colorA = orange;
            glm::vec3 handleA = it->position + it->handleOffsetA;
            glm::vec2 handleAScreen = project(handleA, renderer->getCamera(0).viewProjection)
                * glm::vec2(g_game.windowWidth, g_game.windowHeight);
            if (!isDragging && glm::length(handleAScreen - mousePos) < radius)
            {
                colorA = brightOrange;
                if (!isMouseHandled && g_input.isMouseButtonPressed(MOUSE_LEFT))
                {
                    isMouseHandled = true;
                    dragRailingIndex = currentRailingIndex;
                    dragConnectionIndex = currentPointIndex;
                    dragConnectionHandle = 0;
                    f32 t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1), handleA);
                    glm::vec3 p = cam.position + rayDir * t;
                    dragOffset = handleA - p;
                    isDragging = true;
                }
            }
            if (dragRailingIndex == currentRailingIndex
                    && dragConnectionIndex == currentPointIndex
                    && dragConnectionHandle == 0)
            {
                rit->isDirty = true;
                colorA = brightOrange;
                f32 t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1), handleA);
                glm::vec3 p = cam.position + rayDir * t + dragOffset;
                it->handleOffsetA = p - it->position;
                handleA = it->position + it->handleOffsetA;
                it->handleOffsetB = -it->handleOffsetA;
                renderer->push(OverlayRenderable(sphere, 0,
                            glm::translate(glm::mat4(1.f), handleA) *
                            glm::scale(glm::mat4(0.7f), glm::vec3(1.f)), { 1, 1, 1 }, -1));
            }

            glm::vec3 colorB = orange;
            glm::vec3 handleB = it->position + it->handleOffsetB;
            glm::vec2 handleBScreen = project(handleB, renderer->getCamera(0).viewProjection)
                * glm::vec2(g_game.windowWidth, g_game.windowHeight);
            if (!isDragging && glm::length(handleBScreen - mousePos) < radius)
            {
                colorB = brightOrange;
                if (!isMouseHandled && g_input.isMouseButtonPressed(MOUSE_LEFT))
                {
                    isMouseHandled = true;
                    dragRailingIndex = currentRailingIndex;
                    dragConnectionIndex = currentPointIndex;
                    dragConnectionHandle = 1;
                    f32 t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1), handleB);
                    glm::vec3 p = cam.position + rayDir * t;
                    dragOffset = handleB - p;
                    isDragging = true;
                }
            }
            if (dragRailingIndex == currentRailingIndex
                    && dragConnectionIndex == currentPointIndex
                    && dragConnectionHandle == 1)
            {
                rit->isDirty = true;
                colorB = brightOrange;
                f32 t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1), handleB);
                glm::vec3 p = cam.position + rayDir * t + dragOffset;
                it->handleOffsetB = p - it->position;
                handleB = it->position + it->handleOffsetB;
                it->handleOffsetA = -it->handleOffsetB;
                renderer->push(OverlayRenderable(sphere, 0,
                            glm::translate(glm::mat4(1.f), handleB) *
                            glm::scale(glm::mat4(0.7f), glm::vec3(1.f)), { 1, 1, 1 }, -1));
            }

            renderer->push(OverlayRenderable(sphere, 0,
                        glm::translate(glm::mat4(1.f), handleA) *
                        glm::scale(glm::mat4(0.6f), glm::vec3(1.f)), colorA));
            renderer->push(OverlayRenderable(sphere, 0,
                        glm::translate(glm::mat4(1.f), handleB) *
                        glm::scale(glm::mat4(0.6f), glm::vec3(1.f)), colorB));

            scene->debugDraw.line(it->position + glm::vec3(0, 0, 0.01f),
                    it->position + it->handleOffsetA + glm::vec3(0, 0, 0.01f),
                    glm::vec4(colorA, 1.f), glm::vec4(colorA, 1.f));
            scene->debugDraw.line(it->position + glm::vec3(0, 0, 0.01f),
                    it->position + it->handleOffsetB + glm::vec3(0, 0, 0.01f),
                    glm::vec4(colorB, 1.f), glm::vec4(colorB, 1.f));

            ++it;
        }

        if (rit->points.empty())
        {
            railings.erase(rit);
            continue;
        }
        ++rit;
    }

    bool hasRailingPointSelected = false;
    for (auto& railing : railings)
    {
        if (railing.selectedPoints.size() > 0)
        {
            hasRailingPointSelected = true;
            break;
        }
    }

    // place new railing points
    if (!isDragging && !isMouseHandled)
    {
        for (auto& railing : railings)
        {
            if (railing.selectedPoints.size() > 0 && g_input.isKeyDown(KEY_LCTRL))
            {
                glm::vec3 hitPos = previewRailingPlacement(scene, renderer, cam.position, rayDir);
                if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                {
                    f32 d1 = glm::length2(railing.points.front().position - hitPos);
                    f32 d2 = glm::length2(railing.points.back().position - hitPos);
                    auto insertPos = d1 < d2 ? railing.points.begin() : railing.points.end();
                    auto const& fromPoint = d1 < d2 ? railing.points.front() : railing.points.back();
                    glm::vec3 handleOffset = (hitPos - fromPoint.position) * 0.35f;
                    glm::vec3 handleOffsetA = d1 < d2 ? handleOffset : -handleOffset;
                    glm::vec3 handleOffsetB = d1 < d2 ? -handleOffset : handleOffset;
                    auto inserted = railing.points.insert(insertPos, {
                        hitPos,
                        handleOffsetA,
                        handleOffsetB
                    });
                    railing.selectedPoints.clear();
                    railing.selectedPoints.push_back({ (i32)(inserted - railing.points.begin()) });
                    isMouseHandled = true;
                }
                break;
            }
        }
    }

    // handle dragging of points
    if (dragConnectionHandle == -1
        && (selectedPoints.size() > 0 || hasRailingPointSelected)
        && g_input.isMouseButtonDown(MOUSE_LEFT)
        && glm::length(mousePos - selectMousePos) > g_game.windowHeight * 0.005f)
    {
        glm::vec3 dragTranslation;
        f32 t;
        f32 startZ;
        if (selectedPoints.size() > 0)
        {
            t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1),
                    points[selectedPoints.back().pointIndex].position);
            startZ = points[selectedPoints.back().pointIndex].position.z;
        }
        for (auto& railing : railings)
        {
            if (railing.selectedPoints.size() > 0)
            {
                t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1),
                        railing.points[railing.selectedPoints.back().pointIndex].position);
                startZ = railing.points[railing.selectedPoints.back().pointIndex].position.z;
                break;
            }
        }

        glm::vec3 hitPoint = cam.position + rayDir * t;
        if (!isDragging)
        {
            dragStartPoint = hitPoint;
        }
        if (gridSettings->snap)
        {
            dragTranslation = glm::vec3(snap(glm::vec2(hitPoint) +
                        glm::vec2(0.f), gridSettings->cellSize)
                    - snap(glm::vec2(dragStartPoint), gridSettings->cellSize), 0.f);
        }
        else
        {
            dragTranslation = hitPoint - dragStartPoint;
        }

        for (auto& s : selectedPoints)
        {
            if (!isDragging)
            {
                s.dragStartPoint = points[s.pointIndex].position;
                s.dragStartPoint.z = startZ;
            }
            points[s.pointIndex].position = s.dragStartPoint + dragTranslation;
        }

        for (auto& railing : railings)
        {
            for (auto& s : railing.selectedPoints)
            {
                if (!isDragging)
                {
                    s.dragStartPoint = railing.points[s.pointIndex].position;
                    s.dragStartPoint.z = startZ;
                }
                railing.points[s.pointIndex].position = s.dragStartPoint + dragTranslation;
            }
        }
        isDragging = true;
    }

    if (g_input.isMouseButtonReleased(MOUSE_LEFT))
    {
        dragRailingIndex = -1;
        dragConnectionIndex = -1;
        dragConnectionHandle = -1;
        dragOppositeConnectionIndex = -1;
        dragOppositeConnectionHandle = -1;
        isDragging = false;
    }
}

void Track::extendTrack(i32 prefabCurveIndex)
{
    i32 pointIndex = getSelectedPointIndex();
    BezierSegment* bezierConnection = getPointConnection(pointIndex);
    glm::vec3 fromHandleOffset = (bezierConnection->pointIndexA == pointIndex)
        ? bezierConnection->handleOffsetA : bezierConnection->handleOffsetB;
    glm::vec3 xDir = getPointDir(pointIndex);
    glm::vec3 yDir = glm::cross(xDir, glm::vec3(0, 0, 1));
    glm::vec3 zDir = glm::cross(yDir, xDir);
    glm::mat4 m(1.f);
    m[0] = glm::vec4(xDir, m[0].w);
    m[1] = glm::vec4(yDir, m[1].w);
    m[2] = glm::vec4(zDir, m[2].w);
    i32 pIndex = pointIndex;
    selectedPoints.clear();
    for (u32 c = 0; c<prefabTrackItems[prefabCurveIndex].curves.size(); ++c)
    {
        glm::vec3 p = glm::vec3(m * glm::vec4(prefabTrackItems[prefabCurveIndex].curves[c].offset, 1.f))
            + points[pIndex].position;
        points.push_back({ p });
        glm::vec3 h(m * glm::vec4(prefabTrackItems[prefabCurveIndex].curves[c].handleOffset, 1.f));
        connections.push_back({
            -fromHandleOffset, pIndex, h, (i32)points.size() - 1
        });
        pIndex = (i32)points.size() - 1;
        fromHandleOffset = h;
        selectedPoints.push_back({ (i32)points.size() - 1, {} });
    }
}

// TODO: something is still wrong with this
void Track::connectPoints()
{
    assert(canConnect());

    i32 index1 = selectedPoints[0].pointIndex;
    i32 index2 = selectedPoints[1].pointIndex;
    Point const& p1 = points[index1];
    Point const& p2 = points[index2];

    glm::vec3 handle1 = (p1.position - p2.position) * 0.3f;
    glm::vec3 handle2 = -handle1;

    u32 connectionCount1 = 0;
    u32 connectionCount2 = 0;
    for (auto& c : connections)
    {
        if (c.pointIndexA == index1 || c.pointIndexB == index1)
        {
            ++connectionCount1;
        }
        if (c.pointIndexA == index2 || c.pointIndexB == index2)
        {
            ++connectionCount2;
        }
    }

    if (connectionCount1 == 1)
    {
        auto c1 = getPointConnection(index1);
        handle1 = c1->pointIndexA == index1 ? c1->handleOffsetA : c1->handleOffsetB;
    }

    if (connectionCount2 == 1)
    {
        auto c2 = getPointConnection(index2);
        handle2 = c2->pointIndexA == index2 ? c2->handleOffsetA : c2->handleOffsetB;
    }

    connections.push_back({
        -handle1, index1,
        -handle2, index2
    });
}

void Track::subdividePoints()
{
    // TODO: implement
}

glm::vec3 Track::previewRailingPlacement(Scene* scene, Renderer* renderer, glm::vec3 const& camPos, glm::vec3 const& mouseRayDir)
{
    glm::vec3 p = { 0, 0, 0 };
    PxRaycastBuffer hit;
    if (scene->raycastStatic(camPos, mouseRayDir, 10000.f, &hit))
    {
        p = convert(hit.block.position);
        Mesh* sphere = g_resources.getMesh("world.Sphere");
        renderer->push(OverlayRenderable(sphere, 0,
                    glm::translate(glm::mat4(1.f), p) *
                    glm::scale(glm::mat4(0.6f), glm::vec3(1.f)), blue));
    }
    return p;
}

void Track::placeRailing(glm::vec3 const& p)
{
    railings.push_back({
        { { p, glm::vec3(10, 0, 0), glm::vec3(-10, 0, 0) } },
        {}
    });
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

glm::vec3 Track::getPointDir(u32 pointIndex) const
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

void Track::createSegmentMesh(BezierSegment& c, Scene* scene)
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

    f32 totalLength = c.getLength(points);

    c.boundingBox = { glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX) };
    c.vertices.clear();
    c.indices.clear();
    f32 stepSize = 1.f;
    u32 totalSteps = totalLength / stepSize;
    glm::vec3 prevP = points[c.pointIndexA].position;
    for (u32 i=0; i<=totalSteps; ++i)
    {
        f32 t = (f32)i / (f32)totalSteps;
        glm::vec3 p = getPointOnBezierCurve(
                points[c.pointIndexA].position,
                points[c.pointIndexA].position + c.handleOffsetA,
                points[c.pointIndexB].position + c.handleOffsetB,
                points[c.pointIndexB].position, t);
        glm::vec3 xDir = glm::normalize(i == 0 ? c.handleOffsetA :
                (i == totalSteps ? -c.handleOffsetB : glm::normalize(p - prevP)));
        glm::vec3 yDir = glm::normalize(glm::cross(xDir, glm::vec3(0, 0, 1)));
        glm::vec3 zDir = glm::normalize(glm::cross(yDir, xDir));
        f32 width = glm::lerp(c.widthA, c.widthB, t);
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
        c.boundingBox.min = glm::min(c.boundingBox.min, glm::min(p1, p2));
        c.boundingBox.max = glm::max(c.boundingBox.max, glm::max(p1, p2));
        prevP = p;
    }
    computeBoundingBox();

    glBindVertexArray(c.vao);
    glNamedBufferData(c.vbo, c.vertices.size() * sizeof(Vertex), c.vertices.data(), GL_DYNAMIC_DRAW);
    glNamedBufferData(c.ebo, c.indices.size() * sizeof(u32), c.indices.data(), GL_DYNAMIC_DRAW);
    glVertexArrayVertexBuffer(c.vao, 0, c.vbo, 0, sizeof(Vertex));
    glVertexArrayElementBuffer(c.vao, c.ebo);

    // collision mesh
    PxTriangleMeshDesc desc;
    desc.points.count = c.vertices.size();
    desc.points.stride = sizeof(Vertex);
    desc.points.data = c.vertices.data();
    desc.triangles.count = c.indices.size() / 3;
    desc.triangles.stride = 3 * sizeof(c.indices[0]);
    desc.triangles.data = c.indices.data();

    PxDefaultMemoryOutputStream writeBuffer;
    if (!g_game.physx.cooking->cookTriangleMesh(desc, writeBuffer))
    {
        FATAL_ERROR("Failed to create collision mesh for track segment");
    }

    PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
    PxTriangleMesh* triMesh = g_game.physx.physics->createTriangleMesh(readBuffer);

    if (!c.collisionShape)
    {
        c.collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
                PxTriangleMeshGeometry(triMesh), *scene->trackMaterial);
        c.collisionShape->setQueryFilterData(PxFilterData(
                    COLLISION_FLAG_TRACK, 0, 0, DRIVABLE_SURFACE));
        c.collisionShape->setSimulationFilterData(PxFilterData(
                    COLLISION_FLAG_TRACK, -1, 0, 0));
    }
    else
    {
        c.collisionShape->setGeometry(PxTriangleMeshGeometry(triMesh));
    }
    triMesh->release();
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

void Track::buildTrackGraph(TrackGraph* trackGraph)
{
    trackGraph->clear();
    for (Point& p : points)
    {
        trackGraph->addNode(p.position);
    }
    u32 nodeIndex = points.size();
    for (BezierSegment& c : connections)
    {
        f32 totalLength = c.getLength(points);
        f32 stepSize = 30.f;
        u32 totalSteps = glm::max(2u, (u32)(totalLength / stepSize));
        u32 startNodeIndex = nodeIndex;
        // TODO: distribute points more evenly
        for (u32 i=1; i<totalSteps; ++i)
        {
            glm::vec3 p = c.pointOnCurve(points, i / (f32)totalSteps);
            trackGraph->addNode(p);
            if (i > 1)
            {
                trackGraph->addConnection(nodeIndex - 1, nodeIndex);
            }
            ++nodeIndex;
        }
        trackGraph->addConnection(c.pointIndexA, startNodeIndex);
        trackGraph->addConnection(nodeIndex - 1, c.pointIndexB);
    }
    trackGraph->rebuild(getStart());
}

DataFile::Value Track::serialize()
{
    DataFile::Value dict = DataFile::makeDict();
    dict["entityID"] = DataFile::makeInteger((i64)SerializedEntityID::TRACK);

    dict["points"] = DataFile::makeArray();
    auto& pointsArray = dict["points"].array();
    for (auto& point : points)
    {
        auto pointData = DataFile::makeDict();
        pointData["position"] = DataFile::makeVec3(point.position);
        pointsArray.push_back(std::move(pointData));
    }

    dict["connections"] = DataFile::makeArray();
    auto& connectionArray = dict["connections"].array();
    for (auto& connection : connections)
    {
        auto connectionData = DataFile::makeDict();
        connectionData["handleOffsetA"] = DataFile::makeVec3(connection.handleOffsetA);
        connectionData["pointIndexA"] = DataFile::makeInteger(connection.pointIndexA);
        connectionData["handleOffsetB"] = DataFile::makeVec3(connection.handleOffsetB);
        connectionData["pointIndexB"] = DataFile::makeInteger(connection.pointIndexB);
        connectionData["widthA"] = DataFile::makeReal(connection.widthA);
        connectionData["widthB"] = DataFile::makeReal(connection.widthB);
        connectionArray.push_back(std::move(connectionData));
    }

    dict["railings"] = DataFile::makeArray();
    auto& railingArray = dict["railings"].array();
    for (auto& railing : railings)
    {
        auto railingData = DataFile::makeDict();
        railingData["points"] = DataFile::makeArray();
        auto& railingPointsArray = railingData["points"].array();
        for (auto& point : railing.points)
        {
            auto pointData = DataFile::makeDict();
            pointData["position"] = DataFile::makeVec3(point.position);
            pointData["handleOffsetA"] = DataFile::makeVec3(point.handleOffsetA);
            pointData["handleOffsetB"] = DataFile::makeVec3(point.handleOffsetB);
            railingPointsArray.push_back(std::move(pointData));
        }

        railingArray.push_back(std::move(railingData));
    }

    return dict;
}

void Track::deserialize(DataFile::Value& data)
{
    points.clear();
    connections.clear();
    railings.clear();

    auto& pointArray = data["points"].array();
    for (auto& p : pointArray)
    {
        points.push_back({
            p["position"].vec3()
        });
    }

    auto& connectionArray = data["connections"].array();
    for (auto& c : connectionArray)
    {
        connections.push_back({
            c["handleOffsetA"].vec3(),
            (i32)c["pointIndexA"].integer(),
            c["handleOffsetB"].vec3(),
            (i32)c["pointIndexB"].integer(),
            (f32)c["widthA"].real(),
            (f32)c["widthB"].real(),
        });
    }

    auto& railingArray = data["railings"].array();
    for (auto& r : railingArray)
    {
        auto& pointsArray = r["points"].array();
        Railing railing;
        for (auto& point : pointsArray)
        {
            railing.points.push_back({
                point["position"].vec3(),
                point["handleOffsetA"].vec3(),
                point["handleOffsetB"].vec3(),
            });
        }
        railings.push_back(std::move(railing));
    }
}

void Track::computeBoundingBox()
{
    boundingBox = { glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX) };
    for (auto& c : connections)
    {
        boundingBox = boundingBox.growToFit(c.boundingBox);
    }
    f32 minSize = 100.f;
    glm::vec2 addSize = glm::max(glm::vec2(0.f),
        glm::vec2(minSize) - (glm::vec2(boundingBox.max) - glm::vec2(boundingBox.min)));
    boundingBox.min -= glm::vec3(addSize * 0.5f, 0.f);
    boundingBox.max += glm::vec3(addSize * 0.5f, 0.f);
}

void Track::drawTrackPreview(TrackPreview2D* trackPreview, glm::mat4 const& orthoProjection)
{
    Mesh* quadMesh = g_resources.getMesh("world.Quad");
    trackPreview->drawItem(
        quadMesh->vao, quadMesh->numIndices,
        orthoProjection * getStart()* glm::translate(glm::mat4(1.f), { 0, 0, -2 })
            * glm::scale(glm::mat4(1.f), { 4, 24, 1 }), glm::vec3(0.2f), true);
    for (auto& c : connections)
    {
        trackPreview->drawItem(c.vao, c.indices.size(),
                orthoProjection, glm::vec3(1.0), true);
    }
}
