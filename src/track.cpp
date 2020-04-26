#include "track.h"
#include "scene.h"
#include "renderable.h"
#include "renderer.h"
#include "mesh_renderables.h"
#include "input.h"
#include "game.h"
#include "track_graph.h"
#include "2d.h"
#include "entities/start.h"

glm::vec4 red = { 1.f, 0.f, 0.f, 1.f };
glm::vec4 brightRed = { 1.f, 0.25f, 0.25f, 1.f };
glm::vec4 orange = { 1.f, 0.5f, 0.f, 1.f };
glm::vec4 brightOrange = { 1.f, 0.65f, 0.1f, 1.f };
glm::vec4 blue = { 0.f, 0.0f, 1.f, 1.f };
glm::vec4 brightBlue = { 0.25f, 0.25f, 1.0f, 1.f };

void Track::onCreate(Scene* scene)
{
    actor = g_game.physx.physics->createRigidStatic(PxTransform(PxIdentity));
    physicsUserData.entityType = ActorUserData::ENTITY;
    physicsUserData.entity = this;
    actor->userData = &physicsUserData;
    scene->getPhysicsScene()->addActor(*actor);
    this->scene = scene;
    for (auto& c : connections)
    {
        if (c->isDirty || c->vertices.empty())
        {
            createSegmentMesh(*c, scene);
        }
    }
    if (!scene->track)
    {
        scene->track = this;
    }

    if (railingData.hasValue())
    {
        for (auto& r : railingData.array().val())
        {
            r.dict().val()["entityID"] = DataFile::makeInteger(5);
            scene->deserializeEntity(r);
        }
        railingData = DataFile::Value();
    }
}

void Track::onRender(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    for (auto& c : connections)
    {
        /*
        glm::vec3 prevP;
        for (f32 t=0.f; t<=1.f; t+=0.01f)
        {
            glm::vec3 p = pointOnBezierCurve(
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
        if (c->isDirty || c->vertices.empty())
        {
            createSegmentMesh(*c, scene);
        }
    }

    rw->add(this);
}

void Track::clearSelection()
{
    selectedPoints.clear();
}

void Track::trackModeUpdate(Renderer* renderer, Scene* scene, f32 deltaTime, bool& isMouseHandled, GridSettings* gridSettings)
{
    Mesh* sphere = g_res.getMesh("world.Sphere");
    glm::vec2 mousePos = g_input.getMousePosition();
    Camera const& cam = renderer->getRenderWorld()->getCamera(0);
    glm::vec3 rayDir = screenToWorldRay(mousePos,
            glm::vec2(g_game.windowWidth, g_game.windowHeight), cam.view, cam.projection);
    f32 radius = 18;

    if (!isMouseHandled && g_input.isMouseButtonPressed(MOUSE_LEFT)
            && !g_input.isKeyDown(KEY_LCTRL) && !g_input.isKeyDown(KEY_LSHIFT))
    {
        clearSelection();
    }

    // track points
    for (i32 i=0; i<(i32)points.size();)
    {
        glm::vec3 point = points[i].position;
        glm::vec2 pointScreen = project(point, renderer->getRenderWorld()->getCamera(0).viewProjection)
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
                    if (c->pointIndexA == i || c->pointIndexB == i)
                    {
                        c->isDirty = true;
                    }
                }
            }

            if (points.size() > 1 && connections.size() > 1 && g_input.isKeyPressed(KEY_DELETE))
            {
                for (auto it = connections.begin(); it != connections.end();)
                {
                    if (it->get()->pointIndexA == i || it->get()->pointIndexB == i)
                    {
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
                    if (conn->pointIndexA > i)
                    {
                        --conn->pointIndexA;
                    }
                    if (conn->pointIndexB > i)
                    {
                        --conn->pointIndexB;
                    }
                }
                continue;
            }
            gridSettings->z = point.z + 0.15f;
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
            renderer->getRenderWorld()->push(OverlayRenderable(sphere, 0,
                        glm::translate(glm::mat4(1.f), points[i].position) *
                        glm::scale(glm::mat4(1.08f), glm::vec3(1.f)), { 1, 1, 1 }, -1));
        }
        glm::vec3 color = isSelected ? brightRed : red;
        renderer->getRenderWorld()->push(OverlayRenderable(sphere, 0,
                    glm::translate(glm::mat4(1.f), points[i].position) *
                    glm::scale(glm::mat4(1.f), glm::vec3(1.f)), color));
        ++i;
    }

    // track connections
    for (i32 i=0; i<(i32)connections.size(); ++i)
    {
        auto& c = connections[i];
        if (isDragging)
        {
            auto it = std::find_if(selectedPoints.begin(), selectedPoints.end(), [&c](Selection& s) -> bool {
                return s.pointIndex == c->pointIndexA || s.pointIndex == c->pointIndexB;
            });
            if (it != selectedPoints.end())
            {
                c->isDirty = true;
            }
        }

        f32 widthDiff = ((i32)g_input.isKeyPressed(KEY_Q) - (i32)g_input.isKeyPressed(KEY_E)) * 1.f;

        glm::vec3 colorA = orange;
        glm::vec3 handleA = points[c->pointIndexA].position + c->handleOffsetA;
        glm::vec2 handleAScreen = project(handleA, renderer->getRenderWorld()->getCamera(0).viewProjection)
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
                // TODO: don't move the other handle if a certain key is pressed (ALT?)
                for (i32 connectionIndex = 0; connectionIndex < (i32)connections.size(); ++connectionIndex)
                {
                    auto const& c2 = connections[connectionIndex];
                    if (&c2 != &c)
                    {
                        if (c2->pointIndexB == c->pointIndexA)
                        {
                            if (1.f - glm::dot(-glm::normalize(c2->handleOffsetB),
                                        glm::normalize(c->handleOffsetA)) < 0.01f)
                            {
                                dragOppositeConnectionIndex = connectionIndex;
                                dragOppositeConnectionHandle = 1;
                                break;
                            }
                        }
                        else if (c2->pointIndexA == c->pointIndexA)
                        {
                            if (1.f - glm::dot(-glm::normalize(c2->handleOffsetA),
                                        glm::normalize(c->handleOffsetA)) < 0.01f)
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
        if (dragConnectionIndex == i && dragConnectionHandle == 0)
        {
            c->widthA += widthDiff;
            c->isDirty = true;
            colorA = brightOrange;
            f32 t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1), handleA);
            glm::vec3 p = cam.position + rayDir * t + dragOffset;
            if (gridSettings->snap)
            {
                c->handleOffsetA = snapXY(p, gridSettings->cellSize) - points[c->pointIndexA].position;
            }
            else
            {
                c->handleOffsetA = p - points[c->pointIndexA].position;
            }
            handleA = points[c->pointIndexA].position + c->handleOffsetA;
            if (dragOppositeConnectionIndex != -1)
            {
                auto& c2 = connections[dragOppositeConnectionIndex];
                c2->isDirty = true;
                if (dragOppositeConnectionHandle == 0)
                {
                    c2->handleOffsetA = -c->handleOffsetA;
                }
                else if (dragOppositeConnectionHandle == 1)
                {
                    c2->handleOffsetB = -c->handleOffsetA;
                }
            }
            renderer->getRenderWorld()->push(OverlayRenderable(sphere, 0,
                        glm::translate(glm::mat4(1.f), handleA) *
                        glm::scale(glm::mat4(0.9f), glm::vec3(1.f)), { 1, 1, 1 }, -1));
        }

        glm::vec3 colorB = orange;
        glm::vec3 handleB = points[c->pointIndexB].position + c->handleOffsetB;
        glm::vec2 handleBScreen = project(handleB, renderer->getRenderWorld()->getCamera(0).viewProjection)
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
                // TODO: don't move the other handle if a certain key is pressed (ALT?)
                for (i32 connectionIndex = 0; connectionIndex < (i32)connections.size(); ++connectionIndex)
                {
                    auto const& c2 = connections[connectionIndex];
                    if (c2 == c)
                    {
                        continue;
                    }
                    if (c2->pointIndexB == c->pointIndexB)
                    {
                        if (1.f - glm::dot(-glm::normalize(c2->handleOffsetB),
                                    glm::normalize(c->handleOffsetB)) < 0.01f)
                        {
                            dragOppositeConnectionIndex = connectionIndex;
                            dragOppositeConnectionHandle = 1;
                            break;
                        }
                    }
                    else if (c2->pointIndexA == c->pointIndexB)
                    {
                        if (1.f - glm::dot(-glm::normalize(c2->handleOffsetA),
                                    glm::normalize(c->handleOffsetB)) < 0.01f)
                        {
                            dragOppositeConnectionIndex = connectionIndex;
                            dragOppositeConnectionHandle = 0;
                            break;
                        }
                    }
                }
            }
        }
        if (dragConnectionIndex == i && dragConnectionHandle == 1)
        {
            c->widthB += widthDiff;
            c->isDirty = true;
            colorB = brightOrange;
            f32 t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1), handleB);
            glm::vec3 p = cam.position + rayDir * t + dragOffset;
            if (gridSettings->snap)
            {
                c->handleOffsetB = snapXY(p, gridSettings->cellSize) - points[c->pointIndexB].position;
            }
            else
            {
                c->handleOffsetB = p - points[c->pointIndexB].position;
            }
            handleB = points[c->pointIndexB].position + c->handleOffsetB;
            if (dragOppositeConnectionIndex != -1)
            {
                auto& c2 = connections[dragOppositeConnectionIndex];
                c2->isDirty = true;
                if (dragOppositeConnectionHandle == 0)
                {
                    c2->handleOffsetA = -c->handleOffsetB;
                }
                else if (dragOppositeConnectionHandle == 1)
                {
                    c2->handleOffsetB = -c->handleOffsetB;
                }
            }
            renderer->getRenderWorld()->push(OverlayRenderable(sphere, 0,
                        glm::translate(glm::mat4(1.f), handleB) *
                        glm::scale(glm::mat4(0.9f), glm::vec3(1.f)), { 1, 1, 1 }, -1));
        }
        renderer->getRenderWorld()->push(OverlayRenderable(sphere, 0,
                    glm::translate(glm::mat4(1.f), handleA) *
                    glm::scale(glm::mat4(0.8f), glm::vec3(1.f)), colorA));
        renderer->getRenderWorld()->push(OverlayRenderable(sphere, 0,
                    glm::translate(glm::mat4(1.f), handleB) *
                    glm::scale(glm::mat4(0.8f), glm::vec3(1.f)), colorB));

        scene->debugDraw.line(points[c->pointIndexA].position + glm::vec3(0, 0, 0.01f),
                points[c->pointIndexA].position + c->handleOffsetA + glm::vec3(0, 0, 0.01f),
                glm::vec4(colorA, 1.f), glm::vec4(colorA, 1.f));
        scene->debugDraw.line(points[c->pointIndexB].position + glm::vec3(0, 0, 0.01f),
                points[c->pointIndexB].position + c->handleOffsetB + glm::vec3(0, 0, 0.01f),
                glm::vec4(colorB, 1.f), glm::vec4(colorB, 1.f));
    }

    // railing points
#if 0
    // place new railing points
    if (!isDragging && !isMouseHandled)
    {
        for (auto& railing : railings)
        {
            if (railing->selectedPoints.size() > 0 && g_input.isKeyDown(KEY_LCTRL))
            {
                glm::vec3 hitPos = previewRailingPlacement(scene, renderer, cam.position, rayDir);
                if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                {
                    f32 d1 = glm::length2(railing->points.front().position - hitPos);
                    f32 d2 = glm::length2(railing->points.back().position - hitPos);
                    auto insertPos = d1 < d2 ? railing->points.begin() : railing->points.end();
                    auto const& fromPoint = d1 < d2 ? railing->points.front() : railing->points.back();
                    glm::vec3 handleOffset = (hitPos - fromPoint.position) * 0.35f;
                    glm::vec3 handleOffsetA = d1 < d2 ? handleOffset : -handleOffset;
                    glm::vec3 handleOffsetB = d1 < d2 ? -handleOffset : handleOffset;
                    auto inserted = railing->points.insert(insertPos, {
                        hitPos,
                        handleOffsetA,
                        handleOffsetB
                    });
                    railing->selectedPoints.clear();
                    railing->selectedPoints.push_back({ (i32)(inserted - railing->points.begin()) });
                    isMouseHandled = true;
                }
                break;
            }
        }
    }
#endif

    // handle dragging of points
    if (dragConnectionHandle == -1
        && selectedPoints.size() > 0
        && (g_input.isMouseButtonDown(MOUSE_LEFT) && !isMouseHandled)
        && glm::length(mousePos - selectMousePos) > g_game.windowHeight * 0.005f)
    {
        f32 t = 0.f;
        f32 startZ = 0.f;
        if (selectedPoints.size() > 0)
        {
            t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1),
                    points[selectedPoints.back().pointIndex].position);
            startZ = points[selectedPoints.back().pointIndex].position.z;
        }

        glm::vec3 hitPoint = cam.position + rayDir * t;
        if (!isDragging)
        {
            dragStartPoint = hitPoint;
        }

        glm::vec3 dragTranslation = hitPoint - dragStartPoint;
        for (auto& s : selectedPoints)
        {
            if (!isDragging)
            {
                s.dragStartPoint = points[s.pointIndex].position;
                s.dragStartPoint.z = startZ;
            }
            points[s.pointIndex].position = s.dragStartPoint + dragTranslation;
            if (gridSettings->snap)
            {
                glm::vec2 p = snapXY(points[s.pointIndex].position, gridSettings->cellSize);
                points[s.pointIndex].position = glm::vec3(p, points[s.pointIndex].position.z);
            }
        }

        isDragging = true;
    }

    if (g_input.isMouseButtonReleased(MOUSE_LEFT))
    {
        dragConnectionIndex = -1;
        dragConnectionHandle = -1;
        dragOppositeConnectionIndex = -1;
        dragOppositeConnectionHandle = -1;
        isDragging = false;
    }
}

void Track::matchZ(bool lowest)
{
    if (selectedPoints.size() > 0)
    {
        f32 z = points[selectedPoints[0].pointIndex].position.z;
        for (auto& p : selectedPoints)
        {
            if (lowest)
            {
                if (points[p.pointIndex].position.z < z)
                {
                    z = points[p.pointIndex].position.z;
                }
            }
            else
            {
                if (points[p.pointIndex].position.z > z)
                {
                    z = points[p.pointIndex].position.z;
                }
            }
        }
        for (auto& p : selectedPoints)
        {
            points[p.pointIndex].position.z = z;
        }
    }
}

void Track::extendTrack(i32 prefabCurveIndex)
{
    i32 pointIndex = getSelectedPointIndex();
    BezierSegment* bezierConnection = getPointConnection(pointIndex);
    glm::vec3 fromHandleOffset = (bezierConnection->pointIndexA == pointIndex)
        ? bezierConnection->handleOffsetA : bezierConnection->handleOffsetB;
    f32 fromWidth = (bezierConnection->pointIndexA == pointIndex)
        ? bezierConnection->widthA : bezierConnection->widthB;
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

        auto segment = std::make_unique<BezierSegment>();
        segment->track = this;
        segment->handleOffsetA = -fromHandleOffset;
        segment->pointIndexA = pIndex;
        segment->handleOffsetB = h;
        segment->pointIndexB = (i32)points.size() - 1;
        segment->widthA = fromWidth;
        segment->widthB = fromWidth;
        connections.push_back(std::move(segment));

        pIndex = (i32)points.size() - 1;
        fromHandleOffset = h;
        selectedPoints.push_back({ (i32)points.size() - 1, {} });
    }
}

void Track::subdividePoints()
{
}

// TODO: something is still wrong with this
void Track::connectPoints()
{
    if (selectedPoints.size() != 2)
    {
        return;
    }

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
        if (c->pointIndexA == index1 || c->pointIndexB == index1)
        {
            ++connectionCount1;
        }
        if (c->pointIndexA == index2 || c->pointIndexB == index2)
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

    auto segment = std::make_unique<BezierSegment>();
    segment->track = this;
    segment->handleOffsetA = -handle1;
    segment->pointIndexA = index1;
    if (BezierSegment* s = getPointConnection(index1))
    {
        segment->widthA = s->pointIndexA == index1 ? s->widthA : s->widthB;
    }
    segment->handleOffsetB = -handle2;
    segment->pointIndexB = index2;
    if (BezierSegment* s = getPointConnection(index2))
    {
        segment->widthB = s->pointIndexA == index2 ? s->widthA : s->widthB;
    }
    connections.push_back(std::move(segment));
}

Track::BezierSegment* Track::getPointConnection(i32 pointIndex)
{
    for (auto& c : connections)
    {
        if (c->pointIndexA == pointIndex || c->pointIndexB == pointIndex)
        {
            return c.get();
        }
    }
    return nullptr;
}

glm::vec3 Track::getPointDir(i32 pointIndex) const
{
    glm::vec3 dir;
    u32 count = 0;
    for (auto& c : connections)
    {
        if (c->pointIndexA == pointIndex)
        {
            ++count;
            if (count > 1)
            {
                break;
            }
            dir = -glm::normalize(
                    pointOnBezierCurve(
                        points[c->pointIndexA].position,
                        points[c->pointIndexA].position + c->handleOffsetA,
                        points[c->pointIndexB].position + c->handleOffsetB,
                        points[c->pointIndexB].position, 0.01f) -
                    points[c->pointIndexA].position);
        }
        else if (c->pointIndexB == pointIndex)
        {
            ++count;
            if (count > 1)
            {
                break;
            }
            dir = -glm::normalize(
                    pointOnBezierCurve(
                        points[c->pointIndexA].position,
                        points[c->pointIndexA].position + c->handleOffsetA,
                        points[c->pointIndexB].position + c->handleOffsetB,
                        points[c->pointIndexB].position, 0.99f) -
                    points[c->pointIndexB].position);
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

    f32 totalLength = c.getLength();

    c.boundingBox = { glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX) };
    c.vertices.clear();
    c.indices.clear();
    f32 stepSize = 1.f;
    u32 totalSteps = (u32)(totalLength / stepSize);
    glm::vec3 prevP = points[c.pointIndexA].position;
    for (u32 i=0; i<=totalSteps; ++i)
    {
        f32 t = (f32)i / (f32)totalSteps;
        glm::vec3 p = pointOnBezierCurve(
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
    desc.points.count = (u32)c.vertices.size();
    desc.points.stride = sizeof(Vertex);
    desc.points.data = c.vertices.data();
    desc.triangles.count = (u32)c.indices.size() / 3;
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
                    COLLISION_FLAG_TRACK, DECAL_TRACK, 0, DRIVABLE_SURFACE));
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
        glBindVertexArray(c->vao);
        glDrawElements(GL_TRIANGLES, (GLsizei)c->indices.size(), GL_UNSIGNED_INT, 0);
    }
}

void Track::onDepthPrepass(class Renderer* renderer)
{
    glUseProgram(renderer->getShaderProgram("track"));
    for (auto& c : connections)
    {
        glBindVertexArray(c->vao);
        glDrawElements(GL_TRIANGLES, (GLsizei)c->indices.size(), GL_UNSIGNED_INT, 0);
    }
}

void Track::onLitPass(class Renderer* renderer)
{
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_EQUAL);
    glEnable(GL_CULL_FACE);
    glBindTextureUnit(0, g_res.getTexture("tarmac")->handle);
    glUseProgram(renderer->getShaderProgram("track"));
    for (auto& c : connections)
    {
        glBindVertexArray(c->vao);
        glDrawElements(GL_TRIANGLES, (GLsizei)c->indices.size(), GL_UNSIGNED_INT, 0);
    }
}

void Track::buildTrackGraph(TrackGraph* trackGraph, glm::mat4 const& startTransform)
{
    trackGraph->clear();
    for (Point& p : points)
    {
        trackGraph->addNode(p.position);
    }
    u32 nodeIndex = (u32)points.size();
    for (auto& c : connections)
    {
        f32 totalLength = c->getLength();
        f32 stepSize = 20.f;
        u32 totalSteps = glm::max(2u, (u32)(totalLength / stepSize));
        u32 startNodeIndex = nodeIndex;
        // TODO: distribute points more evenly
        for (u32 i=1; i<totalSteps; ++i)
        {
            glm::vec3 p = c->pointOnCurve(i / (f32)totalSteps);
            trackGraph->addNode(p);
            if (i > 1)
            {
                trackGraph->addConnection(nodeIndex - 1, nodeIndex);
            }
            ++nodeIndex;
        }
        trackGraph->addConnection(c->pointIndexA, startNodeIndex);
        trackGraph->addConnection(nodeIndex - 1, c->pointIndexB);
    }
    trackGraph->rebuild(startTransform);
}

void Track::computeBoundingBox()
{
    boundingBox = { glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX) };
    for (auto& c : connections)
    {
        boundingBox = boundingBox.growToFit(c->boundingBox);
    }
    f32 minSize = 100.f;
    glm::vec2 addSize = glm::max(glm::vec2(0.f),
        glm::vec2(minSize) - (glm::vec2(boundingBox.max) - glm::vec2(boundingBox.min)));
    boundingBox.min -= glm::vec3(addSize * 0.5f, 0.f);
    boundingBox.max += glm::vec3(addSize * 0.5f, 0.f);
}

void Track::drawTrackPreview(TrackPreview2D* trackPreview, glm::mat4 const& orthoProjection)
{
    for (auto& c : connections)
    {
        trackPreview->drawItem(c->vao, (u32)c->indices.size(),
                orthoProjection, glm::vec3(1.f), true);
    }
}

void Track::serializeState(Serializer& s)
{
    s.field(points);
    s.field(connections);

    if (s.deserialize)
    {
        for (auto& c : connections)
        {
            c->track = this;
        }

        railingData = s.dict["railings"];
    }
}
