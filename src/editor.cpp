#include "editor.h"
#include "game.h"
#include "input.h"
#include "renderer.h"
#include "scene.h"
#include "input.h"
#include "terrain.h"
#include "mesh_renderables.h"
#include "track.h"
#include "gui.h"
#include "entities/static_mesh.h"
#include "entities/static_decal.h"
#include "entities/tree.h"
#include "entities/booster.h"
#include <functional>

struct EntityType
{
    const char* name;
    std::function<PlaceableEntity*(glm::vec3 const& p, RandomSeries& s)> make;
};

#define fn [](glm::vec3 const& p, RandomSeries& s) -> PlaceableEntity*

std::vector<EntityType> entityTypes = {
    { "Rock", fn { return new StaticMesh(0, p, glm::vec3(random(s, 0.5f, 1.f)), random(s, 0, PI * 2.f)); } },
    { "Sign", fn { return new StaticMesh(2, p, glm::vec3(1.f), 0.f); } },
    { "Tree", fn { return new Tree(p, glm::vec3(random(s, 1.0f, 1.5f)), random(s, 0, PI * 2.f)); } },
    { "Tunnel", fn { return new StaticMesh(1, p, glm::vec3(1.f), 0.f); } },
    { "Straight Arrow", fn { return new StaticDecal(0, p); } },
    { "Left Arrow", fn { return new StaticDecal(1, p); } },
    { "Right Arrow", fn { return new StaticDecal(2, p); } },
    { "Booster", fn { return new Booster(p); } },
};

#undef fn

glm::vec4 baseButtonColor = glm::vec4(0.f, 0.f, 0.f, 0.9f);
glm::vec4 hoverButtonColor = glm::vec4(0.06f, 0.06f, 0.06f, 0.92f);
glm::vec4 selectedButtonColor = glm::vec4(0.06f, 0.06f, 0.06f, 0.92f);
glm::vec4 disabledButtonColor = glm::vec4(0.f, 0.f, 0.f, 0.75f);

void Editor::onUpdate(Scene* scene, Renderer* renderer, f32 deltaTime)
{
    scene->terrain->setBrushSettings(1.f, 1.f, 1.f, { 0, 0, 1000000 });

    if (g_input.isKeyPressed(KEY_ESCAPE))
    {
        if (scene->isRaceInProgress)
        {
            scene->stopRace();
        }
        entityDragAxis = DragAxis::NONE;
    }

    if (scene->isRaceInProgress)
    {
        return;
    }

    Texture* white = g_resources.getTexture("white");

    f32 right = (f32)g_input.isKeyDown(KEY_D) - (f32)g_input.isKeyDown(KEY_A);
    f32 up = (f32)g_input.isKeyDown(KEY_S) - (f32)g_input.isKeyDown(KEY_W);
    glm::vec3 moveDir = (right != 0.f || up != 0.f) ? glm::normalize(glm::vec3(right, up, 0.f)) : glm::vec3(0, 0, 0);
    glm::vec3 forward(lengthdir(cameraAngle, 1.f), 0.f);
    glm::vec3 sideways(lengthdir(cameraAngle + PI / 2.f, 1.f), 0.f);

    cameraVelocity += (((forward * moveDir.y) + (sideways * moveDir.x)) * (deltaTime * (120.f + cameraDistance * 1.5f)));
    cameraTarget += cameraVelocity * deltaTime;
    cameraVelocity = smoothMove(cameraVelocity, glm::vec3(0, 0, 0), 7.f, deltaTime);

    if (g_input.isMouseButtonPressed(MOUSE_RIGHT))
    {
        lastMousePosition = g_input.getMousePosition();
    }
    else if (g_input.isMouseButtonDown(MOUSE_RIGHT))
    {
        cameraRotateSpeed = (((lastMousePosition.x) - g_input.getMousePosition().x) / g_game.windowWidth * 2.f) * (1.f / deltaTime);
        lastMousePosition = g_input.getMousePosition();
    }

    cameraAngle += cameraRotateSpeed * deltaTime;
    cameraRotateSpeed = smoothMove(cameraRotateSpeed, 0, 8.f, deltaTime);

    f32 height = (f32)g_game.windowHeight;
    glm::vec2 mousePos = g_input.getMousePosition();

    EditMode previousEditMode = editMode;

    if (g_input.isKeyPressed(KEY_TAB))
    {
        editMode = EditMode(((u32)editMode + 1) % (u32)EditMode::MAX);
    }

    g_gui.beginPanel("Track Editor", { 0.f, 0.f },
            0.f, false, false, false, 28, 4, 180);

    g_gui.beginSelect("Edit Mode", (i32*)&editMode, false);
    g_gui.option("Terrain", (i32)EditMode::TERRAIN, "terrain_icon");
    g_gui.option("Track", (i32)EditMode::TRACK, "track_icon");
    g_gui.option("Decoration", (i32)EditMode::DECORATION, "decoration_icon");
    g_gui.end();

    if (editMode == EditMode::TERRAIN)
    {
        if (g_input.isKeyPressed(KEY_SPACE))
        {
            terrainTool = TerrainTool(((u32)terrainTool + 1) % (u32)TerrainTool::MAX);
        }

        g_gui.beginSelect("Terrain Tool", (i32*)&terrainTool);
        g_gui.option("Raise / Lower", (i32)TerrainTool::RAISE, "terrain_icon");
        g_gui.option("Perturb", (i32)TerrainTool::PERTURB, "terrain_icon");
        g_gui.option("Flatten", (i32)TerrainTool::FLATTEN, "terrain_icon");
        g_gui.option("Erode", (i32)TerrainTool::ERODE, "terrain_icon");
        g_gui.option("Match Track", (i32)TerrainTool::MATCH_TRACK, "terrain_icon");
        g_gui.option("Paint", (i32)TerrainTool::PAINT, "terrain_icon");
        g_gui.end();

        g_gui.slider("Brush Radius", 2.f, 40.f, brushRadius);
        g_gui.slider("Brush Falloff", 0.2f, 10.f, brushFalloff);
        g_gui.slider("Brush Strength", -30.f, 30.f, brushStrength);

        if (terrainTool == TerrainTool::PAINT)
        {
            g_gui.beginSelect("Paint Material", &paintMaterialIndex);
            g_gui.option("Main", 0);
            g_gui.option("Rock", 1);
            g_gui.option("Offroad", 2);
            g_gui.option("Garbage", 3);
            g_gui.end();
        }
    }
    else if (editMode == EditMode::TRACK)
    {
        g_gui.toggle("Show Grid", gridSettings.show);
        g_gui.toggle("Snap to Grid", gridSettings.snap);

        if (g_gui.button("Connect Points [c]", scene->track->canConnect()) ||
                (g_input.isKeyPressed(KEY_C) && scene->track->canConnect()))
        {
            scene->track->connectPoints();
        }

        if (g_gui.button("Connect Railings [c]", scene->track->canConnectRailings()) ||
                (g_input.isKeyPressed(KEY_C) && scene->track->canConnectRailings()))
        {
            scene->track->connectRailings();
        }

        if (g_gui.button("Subdivide [n]", scene->track->canSubdivide()) ||
                (g_input.isKeyPressed(KEY_N) && scene->track->canSubdivide()))
        {
            scene->track->subdividePoints();
        }

        if (g_gui.button("Split [t]", scene->track->canSplit()) ||
                (g_input.isKeyPressed(KEY_T) && scene->track->canSubdivide()))
        {
            scene->track->split();
        }

        if (g_gui.button("New Railing"))
        {
            placeMode = PlaceMode::NEW_RAILING;
        }

        if (g_gui.button("New Track Marking"))
        {
            placeMode = PlaceMode::NEW_MARKING;
        }
    }
    else if (editMode == EditMode::DECORATION)
    {
        TransformMode previousTransformMode = transformMode;

        if (g_input.isKeyPressed(KEY_SPACE))
        {
            transformMode = (TransformMode)(((u32)transformMode + 1) % (u32)TransformMode::MAX);
        }

        g_gui.beginSelect("Transform Mode", (i32*)&transformMode);
        g_gui.option("Translate [g]", (i32)TransformMode::TRANSLATE);
        g_gui.option("Rotate [r]", (i32)TransformMode::ROTATE);
        g_gui.option("Scale [f]", (i32)TransformMode::SCALE);
        g_gui.end();

        if (transformMode != previousTransformMode)
        {
            entityDragAxis = DragAxis::NONE;
        }

        if (g_gui.button("Duplicate [b]", selectedEntities.size() > 0)
                || (selectedEntities.size() > 0 && g_input.isKeyPressed(KEY_B)))
        {
            std::vector<PlaceableEntity*> newEntities;
            for (auto e : selectedEntities)
            {
                auto data = e->serialize();
                newEntities.push_back((PlaceableEntity*)scene->deserializeEntity(data));
            }
            selectedEntities.clear();
            for (auto e : newEntities)
            {
                selectedEntities.push_back(e);
            }
        }

        if (g_gui.button("Delete [DELETE]", !entityDragAxis && selectedEntities.size() > 0) ||
                (!entityDragAxis && g_input.isKeyPressed(KEY_DELETE)))
        {
            for (PlaceableEntity* e : selectedEntities)
            {
                e->destroy();
            }
            selectedEntities.clear();
        }

        g_gui.beginSelect("Entity", &selectedEntityTypeIndex);
        for (i32 i=0; i<(i32)entityTypes.size(); ++i)
        {
            auto& entityType = entityTypes[i];
            g_gui.option(entityType.name, i);
        }
        g_gui.end();
    }

    // TODO: make this better by not having 3 panels for 3 buttons
    g_gui.beginPanel("Task1", { g_gui.convertSize(200), 0.f },
                0.f, false, false, false, 28, 0, 100);
    if (g_gui.button("Save Track [F9]") || g_input.isKeyPressed(KEY_F9))
    {
        const char* filename = "saved_scene.dat";
        print("Saving scene to file: ", filename, '\n');
        DataFile::save(scene->serialize(), filename);
    }
    g_gui.end();

    g_gui.beginPanel("Task2", { g_gui.convertSize(300), 0.f },
                0.f, false, false, false, 28, 0, 100);
    if (g_gui.button("Load Track [F10]") || g_input.isKeyPressed(KEY_F10))
    {
        g_game.changeScene("saved_scene.dat");
    }
    g_gui.end();

    g_gui.beginPanel("Task3", { g_gui.convertSize(400), 0.f },
                0.f, false, false, false, 28, 0, 100);
    if (g_gui.button("Test Track [F5]") || g_input.isKeyPressed(KEY_F5))
    {
        scene->terrain->regenerateCollisionMesh(scene);
        scene->startRace();
        entityDragAxis = DragAxis::NONE;
    }
    g_gui.end();

    if (selectedEntities.size() > 0)
    {
        g_gui.beginPanel("Entity Properties", { g_game.windowWidth, 0.f },
                1.f, false, false, false, 30, 4);
        g_gui.label(tstr(selectedEntities.front()->getName(), " Properties"));
        selectedEntities.front()->showDetails(scene);
        g_gui.end();
    }

    g_gui.end();

    bool isMouseClickHandled = g_gui.isMouseOverUI || g_gui.isMouseCaptured;

    if (editMode != previousEditMode)
    {
        scene->terrain->regenerateCollisionMesh(scene);
        entityDragAxis = DragAxis::NONE;
    }

    if (entityDragAxis)
    {
        if (g_input.isMouseButtonReleased(MOUSE_LEFT))
        {
            entityDragAxis = DragAxis::NONE;
        }
        else
        {
            isMouseClickHandled = true;
        }
    }

    if (clickHandledUntilRelease)
    {
        if (g_input.isMouseButtonReleased(MOUSE_LEFT))
        {
            clickHandledUntilRelease = false;
        }
        else
        {
            isMouseClickHandled = true;
        }
    }

    if (g_input.isKeyDown(KEY_LCTRL) && g_input.getMouseScroll() != 0)
    {
        brushRadius = clamp(brushRadius + g_input.getMouseScroll(), 2.0f, 40.f);
    }
    else if (g_input.getMouseScroll() != 0)
    {
        zoomSpeed = g_input.getMouseScroll() * 1.25f;
    }
    cameraDistance = clamp(cameraDistance - zoomSpeed, 10.f, 180.f);
    zoomSpeed = smoothMove(zoomSpeed, 0.f, 10.f, deltaTime);

    glm::vec3 cameraFrom = cameraTarget + glm::normalize(glm::vec3(lengthdir(cameraAngle, 1.f), 1.25f)) * cameraDistance;
    renderer->setViewportCamera(0, cameraFrom, cameraTarget, 5.f, 400.f);

    Camera const& cam = renderer->getCamera(0);
    glm::vec3 rayDir = screenToWorldRay(mousePos,
            glm::vec2(g_game.windowWidth, g_game.windowHeight), cam.view, cam.projection);

    if (editMode == EditMode::TERRAIN)
    {
        const f32 step = 0.01f;
        glm::vec3 p = cam.position;
        while (p.z > scene->terrain->getZ(glm::vec2(p)))
        {
            p += rayDir * step;
        }

        if (!isMouseClickHandled)
        {
            scene->terrain->setBrushSettings(brushRadius, brushFalloff, brushStrength, p);
            if (g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                brushStartZ = scene->terrain->getZ(glm::vec2(p));
                PxRaycastBuffer hit;
                if (scene->raycastStatic(cam.position, rayDir, 10000.f, &hit, COLLISION_FLAG_TRACK))
                {
                    brushStartZ = glm::max(brushStartZ, hit.block.position.z - 0.06f);
                }
            }
            if (g_input.isMouseButtonDown(MOUSE_LEFT))
            {
                switch (terrainTool)
                {
                case TerrainTool::RAISE:
                    scene->terrain->raise(glm::vec2(p), brushRadius, brushFalloff, brushStrength * deltaTime);
                    break;
                case TerrainTool::PERTURB:
                    scene->terrain->perturb(glm::vec2(p), brushRadius, brushFalloff, brushStrength * deltaTime);
                    break;
                case TerrainTool::FLATTEN:
                    scene->terrain->flatten(glm::vec2(p), brushRadius, brushFalloff, glm::abs(brushStrength) * deltaTime, brushStartZ);
                    break;
                case TerrainTool::SMOOTH:
                    scene->terrain->smooth(glm::vec2(p), brushRadius, brushFalloff, glm::abs(brushStrength) * deltaTime);
                    break;
                case TerrainTool::ERODE:
                    scene->terrain->erode(glm::vec2(p), brushRadius, brushFalloff, glm::abs(brushStrength) * deltaTime);
                    break;
                case TerrainTool::MATCH_TRACK:
                    scene->terrain->matchTrack(glm::vec2(p), brushRadius, brushFalloff, glm::abs(brushStrength) * deltaTime, scene);
                    break;
                case TerrainTool::PAINT:
                    scene->terrain->paint(glm::vec2(p), brushRadius, brushFalloff, glm::abs(brushStrength * 1.f) * deltaTime, paintMaterialIndex);
                    break;
                default:
                    break;
                }
            }
        }
        scene->terrain->regenerateMesh();
    }
    else if (editMode == EditMode::TRACK)
    {
        if (placeMode != PlaceMode::NONE)
        {
            glm::vec3 p = scene->track->previewRailingPlacement(scene, renderer, cam.position, rayDir);
            if (!isMouseClickHandled && g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                if (placeMode == PlaceMode::NEW_RAILING)
                {
                    scene->track->placeRailing(p);
                }
                else
                {
                    scene->track->placeMarking(p);
                }
                isMouseClickHandled = true;
                clickHandledUntilRelease = true;
                placeMode = PlaceMode::NONE;
            }

            if (g_input.isKeyPressed(KEY_ESCAPE))
            {
                placeMode = PlaceMode::NONE;
            }
        }

        if (scene->track->hasSelection())
        {
            if (scene->track->canExtendTrack())
            {
                u32 itemSize = (u32)(height * 0.08f);
                u32 iconSize = (u32)(height * 0.08f);
                u32 gap = (u32)(height * 0.015f);
                f32 totalWidth = (f32)(itemSize * ARRAY_SIZE(prefabTrackItems) + gap * (ARRAY_SIZE(prefabTrackItems) - 2));
                f32 cx = g_game.windowWidth * 0.5f;
                f32 yoffset = height * 0.02f;

                for (u32 i=0; i<ARRAY_SIZE(prefabTrackItems); ++i)
                {
                    f32 alpha = 0.8f;
                    glm::vec3 color(0.f);
                    if (pointInRectangle(g_input.getMousePosition(),
                        { cx - totalWidth * 0.5f + ((itemSize + gap) * i), g_game.windowHeight - itemSize - yoffset},
                        (f32)itemSize, (f32)itemSize))
                    {
                        alpha = 0.9f;
                        color = glm::vec3(0.08f);
                        isMouseClickHandled = true;
                        if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                        {
                            clickHandledUntilRelease = true;
                            scene->track->extendTrack(i);
                        }
                    }

                    glm::vec2 bp( cx - totalWidth * 0.5f + ((itemSize + gap) * i),
                            g_game.windowHeight - itemSize - yoffset);
                    renderer->push2D(QuadRenderable(white,
                        bp, (f32)itemSize, (f32)itemSize, color, alpha));
                    renderer->push2D(QuadRenderable(g_resources.getTexture(prefabTrackItems[i].icon),
                        bp + glm::vec2((f32)(itemSize - iconSize)) * 0.5f, (f32)iconSize, (f32)iconSize));
                }
            }
        }

        if (placeMode == PlaceMode::NONE)
        {
            scene->track->trackModeUpdate(renderer, scene, deltaTime, isMouseClickHandled, &gridSettings);
        }
    }
    else if (editMode == EditMode::DECORATION)
    {

        glm::vec3 minP(FLT_MAX);
        glm::vec3 maxP(-FLT_MAX);
        for (PlaceableEntity* e : selectedEntities)
        {
            minP = glm::min(minP, e->position);
            maxP = glm::max(maxP, e->position);
        }

        if (selectedEntities.size() > 0)
        {
            glm::vec3 p = minP + (maxP - minP) * 0.5f;
            f32 rot = (f32)M_PI * 0.5f;
            glm::vec2 windowSize(g_game.windowWidth, g_game.windowHeight);
            glm::mat4 viewProj = renderer->getCamera(0).viewProjection;

            if (g_input.isKeyPressed(KEY_G))
            {
                transformMode = TransformMode::TRANSLATE;
                entityDragAxis = DragAxis::ALL;
            }

            if (g_input.isKeyPressed(KEY_R))
            {
                transformMode = TransformMode::ROTATE;
                entityDragAxis = DragAxis::Z;
                rotatePivot = p;
                entityDragOffset.x = pointDirection(mousePos, project(rotatePivot, viewProj) * windowSize);
            }

            if (g_input.isKeyPressed(KEY_F))
            {
                transformMode = TransformMode::SCALE;
                entityDragAxis = DragAxis::ALL;
            }

            glm::vec3 xCol = glm::vec3(0.95f, 0, 0);
            glm::vec3 xColHighlight = glm::vec3(1, 0.1f, 0.1f);
            glm::vec3 yCol = glm::vec3(0, 0.85f, 0);
            glm::vec3 yColHighlight = glm::vec3(0.2f, 1, 0.2f);
            glm::vec3 zCol = glm::vec3(0, 0, 0.95f);
            glm::vec3 zColHighlight = glm::vec3(0.1f, 0.1f, 1.f);
            glm::vec3 centerCol = glm::vec3(0.8f, 0.8f, 0.8f);

            if (transformMode == TransformMode::TRANSLATE)
            {
                f32 t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1), p);
                glm::vec3 hitPos = cam.position + rayDir * t;
                t = rayPlaneIntersection(cam.position, rayDir,
                        glm::normalize(glm::vec3(glm::vec2(-rayDir), 0.f)), p);
                glm::vec3 hitPosZ = cam.position + rayDir * t;

                if (!isMouseClickHandled)
                {
                    f32 offset = 4.5f;
                    glm::vec2 size(g_game.windowWidth, g_game.windowHeight);
                    glm::vec2 xHandle = projectScale(p, glm::vec3(offset, 0, 0), viewProj) * size;
                    glm::vec2 yHandle = projectScale(p, glm::vec3(0, offset, 0), viewProj) * size;
                    glm::vec2 zHandle = projectScale(p, glm::vec3(0, 0, offset), viewProj) * size;
                    glm::vec2 center = project(p, viewProj) * size;

                    f32 radius = 18.f;
                    glm::vec2 mousePos = g_input.getMousePosition();

                    if (glm::length(xHandle - mousePos) < radius)
                    {
                        xCol = xColHighlight;
                        if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                        {
                            entityDragAxis = DragAxis::X;
                            isMouseClickHandled = true;
                            entityDragOffset = p - hitPos;
                        }
                    }

                    if (glm::length(yHandle - mousePos) < radius)
                    {
                        yCol = yColHighlight;
                        if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                        {
                            entityDragAxis = DragAxis::Y;
                            isMouseClickHandled = true;
                            entityDragOffset = p - hitPos;
                        }
                    }

                    if (glm::length(zHandle - mousePos) < radius)
                    {
                        zCol = zColHighlight;
                        if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                        {
                            entityDragAxis = DragAxis::Z;
                            isMouseClickHandled = true;
                            entityDragOffset = p - hitPosZ;
                        }
                    }

                    bool shortcut = g_input.isKeyPressed(KEY_G);
                    if (glm::length(center - mousePos) < radius || shortcut)
                    {
                        centerCol = glm::vec3(1.f);
                        if (g_input.isMouseButtonPressed(MOUSE_LEFT) || shortcut)
                        {
                            entityDragAxis = DragAxis::ALL;
                            isMouseClickHandled = true;
                            entityDragOffset = p - hitPos;
                        }
                    }
                }

                if (entityDragAxis)
                {
                    if (entityDragAxis & DragAxis::X)
                    {
                        for (PlaceableEntity* e : selectedEntities)
                        {
                            f32 diff = p.x - e->position.x;
                            e->position.x = hitPos.x + entityDragOffset.x - diff;
                        }
                        p.x = hitPos.x + entityDragOffset.x;
                        xCol = xColHighlight;
                    }

                    if (entityDragAxis & DragAxis::Y)
                    {
                        for (PlaceableEntity* e : selectedEntities)
                        {
                            f32 diff = p.y - e->position.y;
                            e->position.y = hitPos.y + entityDragOffset.y - diff;
                        }
                        p.y = hitPos.y + entityDragOffset.y;
                        yCol = yColHighlight;
                    }

                    if (entityDragAxis & DragAxis::Z)
                    {
                        for (PlaceableEntity* e : selectedEntities)
                        {
                            f32 diff = p.z - e->position.z;
                            e->position.z = hitPosZ.z + entityDragOffset.z - diff;
                        }
                        p.z = hitPosZ.z + entityDragOffset.z;
                        zCol = zColHighlight;
                    }

                    for (PlaceableEntity* e : selectedEntities)
                    {
                        e->updateTransform(scene);
                    }
                }

                Mesh* arrowMesh = g_resources.getMesh("world.TranslateArrow");
                Mesh* centerMesh = g_resources.getMesh("world.Sphere");
                renderer->push(OverlayRenderable(centerMesh, 0,
                        glm::translate(glm::mat4(1.f), p), centerCol, -1));

                renderer->push(OverlayRenderable(arrowMesh, 0,
                        glm::translate(glm::mat4(1.f), p), xCol));
                if (entityDragAxis & DragAxis::X)
                {
                    scene->debugDraw.line(
                            p - glm::vec3(100.f, 0.f, 0.f), p + glm::vec3(100.f, 0.f, 0.f),
                            glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1));
                }

                renderer->push(OverlayRenderable(arrowMesh, 0,
                        glm::translate(glm::mat4(1.f), p) *
                        glm::rotate(glm::mat4(1.f), rot, glm::vec3(0, 0, 1)), yCol));
                if (entityDragAxis & DragAxis::Y)
                {
                    scene->debugDraw.line(
                            p - glm::vec3(0.f, 100.f, 0.f), p + glm::vec3(0.f, 100.f, 0.f),
                            glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1));
                }

                renderer->push(OverlayRenderable(arrowMesh, 0,
                        glm::translate(glm::mat4(1.f), p) *
                        glm::rotate(glm::mat4(1.f), -rot, glm::vec3(0, 1, 0)), zCol));
                if (entityDragAxis & DragAxis::Z)
                {
                    scene->debugDraw.line(
                            p - glm::vec3(0.f, 0.f, 100.f), p + glm::vec3(0.f, 0.f, 100.f),
                            glm::vec4(0, 0, 1, 1), glm::vec4(0, 0, 1, 1));
                }
            }
            if (transformMode == TransformMode::ROTATE)
            {
                f32 dist = glm::length(cam.position - p);
                f32 fixedSize = 0.085f;
                f32 size = dist * fixedSize * glm::radians(cam.fov);
                glm::vec2 mousePos = g_input.getMousePosition();

                f32 sphereT = raySphereIntersection(cam.position, rayDir, p, size);
                if (sphereT > 0.f)
                {
                    glm::vec2 centerPos = project(p, viewProj) * windowSize;
                    glm::vec3 sphereHitPos = cam.position + rayDir * sphereT;
                    glm::vec3 localHitPos = glm::normalize(sphereHitPos - p);

                    if (!isMouseClickHandled)
                    {
                        if (glm::abs(localHitPos.x) < 0.2f)
                        {
                            xCol = xColHighlight;
                            if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                            {
                                entityDragAxis = DragAxis::X;
                                isMouseClickHandled = true;
                                entityDragOffset.x = pointDirection(mousePos, centerPos);
                                rotatePivot = p;
                            }
                        }
                        else if (glm::abs(localHitPos.y) < 0.2f)
                        {
                            yCol = yColHighlight;
                            if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                            {
                                entityDragAxis = DragAxis::Y;
                                isMouseClickHandled = true;
                                entityDragOffset.x = pointDirection(mousePos, centerPos);
                                rotatePivot = p;
                            }
                        }
                        else if (glm::abs(localHitPos.z) < 0.2f)
                        {
                            zCol = zColHighlight;
                            if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                            {
                                entityDragAxis = DragAxis::Z;
                                isMouseClickHandled = true;
                                entityDragOffset.x = pointDirection(mousePos, centerPos);
                                rotatePivot = p;
                            }
                        }
                    }
                }

                if (entityDragAxis)
                {
                    glm::vec2 centerPos = project(rotatePivot, viewProj) * windowSize;
                    f32 angle = pointDirection(mousePos, centerPos);
                    f32 angleDiff = angleDifference(angle, entityDragOffset.x);
                    if (entityDragAxis & DragAxis::X)
                    {
                        for (PlaceableEntity* e : selectedEntities)
                        {
                            glm::mat4 transform = glm::rotate(glm::mat4(1.f), angleDiff, glm::vec3(1, 0, 0)) *
                                glm::translate(glm::mat4(1.f), -rotatePivot);
                            e->position = glm::vec3(transform * glm::vec4(e->position, 1.f)) + rotatePivot;
                            e->rotation = glm::rotate(glm::identity<glm::quat>(),
                                    angleDiff, glm::vec3(1, 0, 0)) * e->rotation;
                        }
                    }
                    else if (entityDragAxis & DragAxis::Y)
                    {
                        for (PlaceableEntity* e : selectedEntities)
                        {
                            glm::mat4 transform = glm::rotate(glm::mat4(1.f), angleDiff, glm::vec3(0, 1, 0)) *
                                glm::translate(glm::mat4(1.f), -rotatePivot);
                            e->position = glm::vec3(transform * glm::vec4(e->position, 1.f)) + rotatePivot;
                            e->rotation = glm::rotate(glm::identity<glm::quat>(),
                                    angleDiff, glm::vec3(0, 1, 0)) * e->rotation;
                        }
                    }
                    else if (entityDragAxis & DragAxis::Z)
                    {
                        for (PlaceableEntity* e : selectedEntities)
                        {
                            glm::mat4 transform = glm::rotate(glm::mat4(1.f), angleDiff, glm::vec3(0, 0, 1)) *
                                glm::translate(glm::mat4(1.f), -rotatePivot);
                            e->position = glm::vec3(transform * glm::vec4(e->position, 1.f)) + rotatePivot;
                            e->rotation = glm::rotate(glm::identity<glm::quat>(),
                                    angleDiff, glm::vec3(0, 0, 1)) * e->rotation;
                        }
                    }
                    entityDragOffset.x = angle;

                    for (PlaceableEntity* e : selectedEntities)
                    {
                        e->updateTransform(scene);
                    }
                }
                else
                {
                    Mesh* arrowMesh = g_resources.getMesh("world.RotateArrow");
                    Mesh* sphereMesh = g_resources.getMesh("world.Sphere");
                    renderer->push(OverlayRenderable(sphereMesh, 0,
                            glm::translate(glm::mat4(1.f), p) * glm::scale(glm::mat4(1.f), glm::vec3(4.4f))
                            , {0,0,0}, -1, true));

                    renderer->push(OverlayRenderable(arrowMesh, 0,
                            glm::translate(glm::mat4(1.f), p) *
                            glm::rotate(glm::mat4(1.f), rot, glm::vec3(0, 1, 0)), xCol));
                    if (entityDragAxis & DragAxis::X)
                    {
                        scene->debugDraw.line(
                                p - glm::vec3(100.f, 0.f, 0.f), p + glm::vec3(100.f, 0.f, 0.f),
                                glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1));
                    }

                    renderer->push(OverlayRenderable(arrowMesh, 0,
                            glm::translate(glm::mat4(1.f), p) *
                            glm::rotate(glm::mat4(1.f), rot, glm::vec3(1, 0, 0)), yCol));
                    if (entityDragAxis & DragAxis::Y)
                    {
                        scene->debugDraw.line(
                                p - glm::vec3(0.f, 100.f, 0.f), p + glm::vec3(0.f, 100.f, 0.f),
                                glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1));
                    }

                    renderer->push(OverlayRenderable(arrowMesh, 0,
                            glm::translate(glm::mat4(1.f), p), zCol));
                    if (entityDragAxis & DragAxis::Z)
                    {
                        scene->debugDraw.line(
                                p - glm::vec3(0.f, 0.f, 100.f), p + glm::vec3(0.f, 0.f, 100.f),
                                glm::vec4(0, 0, 1, 1), glm::vec4(0, 0, 1, 1));
                    }
                }
            }
            if (transformMode == TransformMode::SCALE)
            {
                f32 t = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1), p);
                glm::vec3 hitPos = cam.position + rayDir * t;
                t = rayPlaneIntersection(cam.position, rayDir,
                        glm::normalize(glm::vec3(glm::vec2(-rayDir), 0.f)), p);
                glm::vec3 hitPosZ = cam.position + rayDir * t;

                glm::mat4 orientation(1.f);
                if (selectedEntities.size() == 1)
                {
                    orientation = glm::mat4_cast(selectedEntities[0]->rotation);
                }
                f32 offset = 4.5f;
                glm::vec3 xAxis = xAxisOf(orientation) * offset;
                glm::vec3 yAxis = yAxisOf(orientation) * offset;
                glm::vec3 zAxis = zAxisOf(orientation) * offset;

                if (!isMouseClickHandled)
                {
                    glm::vec2 size(g_game.windowWidth, g_game.windowHeight);
                    glm::vec2 xHandle = projectScale(p, xAxis, viewProj) * size;
                    glm::vec2 yHandle = projectScale(p, yAxis, viewProj) * size;
                    glm::vec2 zHandle = projectScale(p, zAxis, viewProj) * size;
                    glm::vec2 center = project(p, viewProj) * size;

                    f32 radius = 18.f;
                    glm::vec2 mousePos = g_input.getMousePosition();

                    if (glm::length(xHandle - mousePos) < radius)
                    {
                        xCol = xColHighlight;
                        if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                        {
                            entityDragAxis = DragAxis::X;
                            isMouseClickHandled = true;
                            entityDragOffset = p - hitPos;
                        }
                    }

                    if (glm::length(yHandle - mousePos) < radius)
                    {
                        yCol = yColHighlight;
                        if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                        {
                            entityDragAxis = DragAxis::Y;
                            isMouseClickHandled = true;
                            entityDragOffset = p - hitPos;
                        }
                    }

                    if (glm::length(zHandle - mousePos) < radius)
                    {
                        zCol = zColHighlight;
                        if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                        {
                            entityDragAxis = DragAxis::Z;
                            isMouseClickHandled = true;
                            entityDragOffset = p - hitPosZ;
                        }
                    }

                    bool shortcut = g_input.isKeyPressed(KEY_F);
                    if (glm::length(center - mousePos) < radius || shortcut)
                    {
                        centerCol = glm::vec3(1.f);
                        if (g_input.isMouseButtonPressed(MOUSE_LEFT) || shortcut)
                        {
                            entityDragAxis = DragAxis::ALL;
                            isMouseClickHandled = true;
                            entityDragOffset = p - hitPos;
                        }
                    }
                }

                if (entityDragAxis)
                {
                    if (selectedEntities.size() > 1)
                    {
                        f32 startDistance = glm::length(entityDragOffset);
                        f32 scaleFactor = glm::length(p - hitPos) / startDistance;
                        for (PlaceableEntity* e : selectedEntities)
                        {
                            glm::mat4 t = glm::translate(glm::mat4(1.f), e->position)
                                * glm::scale(glm::mat4(1.f), glm::vec3(e->scale));
                            glm::mat4 transform = glm::scale(glm::mat4(1.f), glm::vec3(scaleFactor))
                                * glm::translate(glm::mat4(1.f), -p) * t;
                            e->position = translationOf(transform) + p;
                            e->scale *= scaleFactor;
                        }
                        entityDragOffset = p - hitPos;
                        xCol = xColHighlight;
                        yCol = yColHighlight;
                        zCol = zColHighlight;
                    }
                    else if ((entityDragAxis & DragAxis::ALL) == DragAxis::ALL)
                    {
                        f32 startDistance = glm::length(entityDragOffset);
                        for (PlaceableEntity* e : selectedEntities)
                        {
                            e->scale *= glm::vec3(glm::length(p - hitPos) / startDistance);
                        }
                        entityDragOffset = p - hitPos;
                        xCol = xColHighlight;
                        yCol = yColHighlight;
                        zCol = zColHighlight;
                    }
                    else if (entityDragAxis & DragAxis::X)
                    {
                        f32 startDistance = glm::dot(entityDragOffset, xAxis);
                        for (PlaceableEntity* e : selectedEntities)
                        {
                            e->scale.x *= glm::dot(p - hitPos, xAxis) / startDistance;
                        }
                        entityDragOffset = p - hitPos;
                        xCol = xColHighlight;
                    }
                    else if (entityDragAxis & DragAxis::Y)
                    {
                        f32 startDistance = glm::dot(entityDragOffset, yAxis);
                        for (PlaceableEntity* e : selectedEntities)
                        {
                            e->scale.y *= glm::dot(p - hitPos, yAxis) / startDistance;
                        }
                        entityDragOffset = p - hitPos;
                        yCol = yColHighlight;
                    }
                    else if (entityDragAxis & DragAxis::Z)
                    {
                        f32 startDistance = glm::dot(entityDragOffset, zAxis);
                        for (PlaceableEntity* e : selectedEntities)
                        {
                            e->scale.z *= glm::dot(p - hitPosZ, zAxis) / startDistance;
                        }
                        entityDragOffset = p - hitPosZ;
                        zCol = zColHighlight;
                    }

                    for (PlaceableEntity* e : selectedEntities)
                    {
                        e->updateTransform(scene);
                    }
                }

                Mesh* arrowMesh = g_resources.getMesh("world.ScaleArrow");
                Mesh* centerMesh = g_resources.getMesh("world.UnitCube");
                renderer->push(OverlayRenderable(centerMesh, 0,
                        glm::translate(glm::mat4(1.f), p) * orientation, centerCol, -1));

                renderer->push(OverlayRenderable(arrowMesh, 0,
                        glm::translate(glm::mat4(1.f), p) * orientation, xCol));
                if (entityDragAxis & DragAxis::X)
                {
                    scene->debugDraw.line(
                            p - glm::vec3(100.f, 0.f, 0.f), p + glm::vec3(100.f, 0.f, 0.f),
                            glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1));
                }

                renderer->push(OverlayRenderable(arrowMesh, 0,
                        glm::translate(glm::mat4(1.f), p) *
                        orientation *
                        glm::rotate(glm::mat4(1.f), rot, glm::vec3(0, 0, 1)), yCol));
                if (entityDragAxis & DragAxis::Y)
                {
                    scene->debugDraw.line(
                            p - glm::vec3(0.f, 100.f, 0.f), p + glm::vec3(0.f, 100.f, 0.f),
                            glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1));
                }

                renderer->push(OverlayRenderable(arrowMesh, 0,
                        glm::translate(glm::mat4(1.f), p) *
                        orientation *
                        glm::rotate(glm::mat4(1.f), -rot, glm::vec3(0, 1, 0)), zCol));
                if (entityDragAxis & DragAxis::Z)
                {
                    scene->debugDraw.line(
                            p - glm::vec3(0.f, 0.f, 100.f), p + glm::vec3(0.f, 0.f, 100.f),
                            glm::vec4(0, 0, 1, 1), glm::vec4(0, 0, 1, 1));
                }
            }
        }

        if (!isMouseClickHandled)
        {
            if (g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                isMouseClickHandled = true;
                if (g_input.isKeyDown(KEY_LCTRL) && g_input.isKeyDown(KEY_LSHIFT))
                {
                    PxRaycastBuffer hit;
                    if (scene->raycastStatic(cam.position, rayDir, 10000.f, &hit))
                    {
                        glm::vec3 hitPoint = convert(hit.block.position);
                        PlaceableEntity* newEntity =
                            entityTypes[selectedEntityTypeIndex].make(hitPoint, scene->randomSeries);
                        newEntity->updateTransform(scene);
                        scene->addEntity(newEntity);
                    }
                }
                else
                {
                    if (!g_input.isKeyDown(KEY_LCTRL) && !g_input.isKeyDown(KEY_LSHIFT))
                    {
                        selectedEntities.clear();
                    }
                    PxRaycastBuffer hit;
                    if (scene->raycastStatic(cam.position, rayDir, 10000.f,
                                &hit, COLLISION_FLAG_SELECTABLE))
                    {
                        if (hit.block.actor)
                        {
                            ActorUserData* userData = (ActorUserData*)hit.block.actor->userData;
                            if (userData->entityType == ActorUserData::SELECTABLE_ENTITY)
                            {
                                auto it = std::find(selectedEntities.begin(),
                                    selectedEntities.end(), userData->placeableEntity);
                                if (g_input.isKeyDown(KEY_LSHIFT))
                                {
                                    if (it != selectedEntities.end())
                                    {
                                        selectedEntities.erase(it);
                                    }
                                }
                                else
                                {
                                    if (it == selectedEntities.end())
                                    {
                                        selectedEntities.push_back((PlaceableEntity*)userData->entity);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        for (auto& e : scene->getEntities())
        {
            bool isSelected = std::find(selectedEntities.begin(),
                    selectedEntities.end(), e.get()) != selectedEntities.end();
            e->onEditModeRender(renderer, scene, isSelected);
        }
    }

    if (gridSettings.show && editMode != EditMode::TERRAIN)
    {
        f32 gridSize = 40.f;
        glm::vec4 color = { 1.f, 1.f, 1.f, 0.2f };
        for (f32 x=-gridSize; x<=gridSize; x+=gridSettings.cellSize)
        {
            scene->debugDraw.line(
                { snap(cameraTarget.x + x, gridSettings.cellSize), snap(cameraTarget.y - gridSize, gridSettings.cellSize), gridSettings.z },
                { snap(cameraTarget.x + x, gridSettings.cellSize), snap(cameraTarget.y + gridSize, gridSettings.cellSize), gridSettings.z },
                color, color);
        }
        for (f32 y=-gridSize; y<=gridSize; y+=gridSettings.cellSize)
        {
            scene->debugDraw.line(
                { snap(cameraTarget.x - gridSize, gridSettings.cellSize), snap(cameraTarget.y + y, gridSettings.cellSize), gridSettings.z },
                { snap(cameraTarget.x + gridSize, gridSettings.cellSize), snap(cameraTarget.y + y, gridSettings.cellSize), gridSettings.z },
                color, color);
        }
    }
}
