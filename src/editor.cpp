#include "editor.h"
#include "game.h"
#include "input.h"
#include "renderer.h"
#include "scene.h"
#include "input.h"
#include "terrain.h"
#include "mesh_renderables.h"
#include "track.h"
#include "entities/static_mesh.h"
#include "entities/static_decal.h"
#include "entities/tree.h"

struct EntityType
{
    std::string name;
    std::function<PlaceableEntity*(glm::vec3 const& p, RandomSeries& s)> make;
};

#define fn [](glm::vec3 const& p, RandomSeries& s) -> PlaceableEntity*

std::vector<EntityType> entityTypes = {
    { "Rock", fn { return new StaticMesh(0, p, glm::vec3(random(s, 0.5f, 1.f)), random(s, 0, M_PI * 2)); } },
    { "Tree", fn { return new Tree(p, glm::vec3(random(s, 1.0f, 1.5f)), random(s, 0, M_PI * 2)); } },
    { "Tunnel", fn { return new StaticMesh(1, p, glm::vec3(1.f), 0.f); } },
    { "Straight Arrow", fn { return new StaticDecal(0, p); } },
    { "Left Arrow", fn { return new StaticDecal(1, p); } },
    { "Right Arrow", fn { return new StaticDecal(2, p); } },
};

#undef fn

void Editor::onStart(Scene* scene)
{

}

void Editor::onUpdate(Scene* scene, Renderer* renderer, f32 deltaTime)
{
    scene->terrain->setBrushSettings(1.f, 1.f, 1.f, { 0, 0, 1000000 });

    if (g_input.isKeyPressed(KEY_F9))
    {
        const char* filename = "saved_scene.dat";
        print("Saving scene to file: ", filename, '\n');
        DataFile::save(scene->serialize(), filename);
    }

    if (g_input.isKeyPressed(KEY_F10))
    {
        g_game.changeScene("saved_scene.dat");
    }

    if (g_input.isKeyPressed(KEY_F5))
    {
        if (scene->isRaceInProgress)
        {
            scene->stopRace();
        }
        else
        {
            scene->terrain->regenerateCollisionMesh(scene);
            scene->startRace();
        }
        entityDragAxis = DragAxis::NONE;
    }
    else if (g_input.isKeyPressed(KEY_ESCAPE))
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
    glm::vec3 sideways(lengthdir(cameraAngle + M_PI / 2.f, 1.f), 0.f);

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

    bool isMouseClickHandled = false;
    //bool isMouseWheelHandled = g_input.getMouseScroll() != 0;

    f32 height = g_game.windowHeight;
    Font* font = &g_resources.getFont("font", height * 0.03f);
    Font* fontSmall = &g_resources.getFont("font", height * 0.02f);
    glm::vec2 mousePos = g_input.getMousePosition();
    auto button = [&](glm::vec2& pos, glm::vec2 spacing, const char* text, bool enabled=true) {
        f32 alpha = 0.9f;
        f32 textAlpha = 1.f;
        glm::vec3 color(0.f);
        bool wasClicked = false;
        u32 buttonWidth = height * 0.15f;
        u32 buttonHeight = height * 0.04f;
        if (!enabled)
        {
            alpha = 0.7f;
            textAlpha = 0.75f;
        }
        if (enabled && pointInRectangle(mousePos, pos, buttonWidth, buttonHeight))
        {
            alpha = 0.92f;
            color = glm::vec3(0.1f);
            isMouseClickHandled = true;
            if (g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                clickHandledUntilRelease = true;
                wasClicked = true;
            }
        }
        renderer->push(QuadRenderable(white,
                pos, buttonWidth, buttonHeight, color, alpha));
        renderer->push(TextRenderable(fontSmall, text, pos + glm::vec2(buttonWidth / 2, buttonHeight / 2),
                    glm::vec3(1.f), textAlpha, 1.f, HorizontalAlign::CENTER, VerticalAlign::CENTER));
        pos += spacing;

        return wasClicked;
    };
    auto slider = [&](glm::vec2& pos, glm::vec2 spacing, std::string&& text, f32 min, f32 max, f32& val) {
        f32 alpha = 0.9f;
        f32 textAlpha = 1.f;
        glm::vec3 color(0.f);
        bool wasClicked = false;
        u32 buttonWidth = height * 0.15f;
        u32 buttonHeight = height * 0.04f;
        if (pointInRectangle(mousePos, pos, buttonWidth, buttonHeight))
        {
            alpha = 0.92f;
            color = glm::vec3(0.1f);
            isMouseClickHandled = true;
            if (g_input.isMouseButtonDown(MOUSE_LEFT))
            {
                clickHandledUntilRelease = true;
                wasClicked = true;
                f32 t = (mousePos.x - pos.x) / buttonWidth;
                val = min + (max - min) * t;
            }
            // TODO: add support for mouse scrolling to change value
        }
        renderer->push(QuadRenderable(white,
                pos, buttonWidth, buttonHeight, color, alpha));
        renderer->push(QuadRenderable(white,
                pos, buttonWidth * ((val - min) / (max - min)), buttonHeight * 0.1f,
                val >= 0.f ? glm::vec3(0.0f, 0.f, 0.9f) : glm::vec3(0.9f, 0.f, 0.f), alpha));
        renderer->push(TextRenderable(fontSmall, std::move(text), pos + glm::vec2(buttonWidth / 2, buttonHeight / 2),
                    glm::vec3(1.f), textAlpha, 1.f, HorizontalAlign::CENTER, VerticalAlign::CENTER));
        pos += spacing;

        return wasClicked;
    };

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

    if (g_input.isKeyPressed(KEY_TAB))
    {
        editMode = EditMode(((u32)editMode + 1) % (u32)EditMode::MAX);
        scene->terrain->regenerateCollisionMesh(scene);
        entityDragAxis = DragAxis::NONE;
    }
    glm::vec2 guiOffset(height * 0.012f);
    f32 modeSelectionMaxY = 0.f;
    {
        const char* modeNames[] = { "Terrain", "Track", "Decoration" };
        const char* icons[] = { "terrain_icon", "track_icon", "decoration_icon" };
        glm::vec2 offset = guiOffset;
        u32 padding = height * 0.015f;
        u32 buttonHeight = height * 0.03f;
        f32 textHeight = font->getHeight();
        for (u32 i=0; i<(u32)EditMode::MAX; ++i)
        {
            u32 buttonWidth = height * 0.16f;
            f32 alpha = 0.9f;
            glm::vec3 color(0.f);
            if (i == (u32)editMode)
            {
                buttonWidth = height * 0.20f;
                alpha = 0.9f;
                color = glm::vec3(0.1f);
            }
            if (pointInRectangle(g_input.getMousePosition(),
                        offset + glm::vec2(0, (buttonHeight + padding * 2) * i),
                        buttonWidth + padding * 2, buttonHeight + padding * 2))
            {
                alpha = 0.92f;
                color = glm::vec3(0.1f);
                isMouseClickHandled = true;
                if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                {
                    editMode = EditMode(i);
                    scene->terrain->regenerateCollisionMesh(scene);
                }
            }
            renderer->push(QuadRenderable(white,
                        offset + glm::vec2(0, (buttonHeight + padding * 2) * i),
                        buttonWidth + padding * 2, buttonHeight + padding * 2,
                        color, alpha));
            u32 iconSize = buttonHeight;
            renderer->push(TextRenderable(font, modeNames[i],
                offset + glm::vec2(padding * 2 + iconSize,
                    ((padding * 2 + buttonHeight) - textHeight) / 2 + (buttonHeight + padding * 2) * i),
                glm::vec3(1), 1.f, 1.f));
            renderer->push(QuadRenderable(g_resources.getTexture(icons[i]),
                        offset + glm::vec2(padding, padding + (buttonHeight + padding * 2) * i),
                        iconSize, iconSize));
        }
        modeSelectionMaxY = offset.y + (buttonHeight + padding * 2) * (f32)EditMode::MAX;
        /*
        renderer->push(TextRenderable(fontSmall, "[TAB]",
                    offset + glm::vec2(height * 0.09f, modeSelectionMaxY),
                    glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::CENTER));
        */
    }

    glm::vec2 buttonOffset = guiOffset + glm::vec2(0, modeSelectionMaxY);
    glm::vec2 buttonSpacing(0.f, height * 0.045f);

    //if (editMode != EditMode::TERRAIN)
    if (editMode == EditMode::TRACK)
    {
        if (button(buttonOffset, buttonSpacing, gridSettings.show ? "Show Grid: ON" : "Show Grid: OFF"))
        {
            gridSettings.show = !gridSettings.show;
        }

        if (button(buttonOffset, buttonSpacing, gridSettings.snap ? "Snap to Grid: ON" : "Snap to Grid: OFF"))
        {
            gridSettings.snap = !gridSettings.snap;
        }
    }

    if (editMode == EditMode::TERRAIN)
    {
        const f32 step = 0.01f;
        glm::vec3 p = cam.position;
        while (p.z > scene->terrain->getZ(glm::vec2(p)))
        {
            p += rayDir * step;
        }

        if (g_input.isKeyPressed(KEY_SPACE))
        {
            terrainTool = TerrainTool(((u32)terrainTool + 1) % (u32)TerrainTool::MAX);
        }

        const char* toolNames[] = { "Raise / Lower", "Perturb", "Flatten", "Smooth", "Erode", "Match Track", "Paint" };
        // TODO: icons
        const char* icons[] = { "terrain_icon", "terrain_icon", "terrain_icon", "terrain_icon", "terrain_icon", "terrain_icon", "terrain_icon" };
        glm::vec2 offset = buttonOffset;
        u32 padding = height * 0.01f;
        u32 buttonHeight = height * 0.02f;
        f32 textHeight = fontSmall->getHeight();
        for (u32 i=0; i<(u32)TerrainTool::MAX; ++i)
        {
            u32 buttonWidth = height * 0.12f;
            f32 alpha = 0.9f;
            glm::vec3 color(0.f);
            if (i == (u32)terrainTool)
            {
                buttonWidth = height * 0.15f;
                alpha = 0.92f;
                color = glm::vec3(0.1f);
            }
            if (pointInRectangle(g_input.getMousePosition(),
                        offset + glm::vec2(0, (buttonHeight + padding * 2) * i),
                        buttonWidth + padding * 2, buttonHeight + padding * 2))
            {
                alpha = 0.92f;
                color = glm::vec3(0.1f);
                isMouseClickHandled = true;
                if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                {
                    terrainTool = TerrainTool(i);
                }
            }
            renderer->push(QuadRenderable(white,
                        offset + glm::vec2(0, (buttonHeight + padding * 2) * i),
                        buttonWidth + padding * 2, buttonHeight + padding * 2,
                        color, alpha));
            u32 iconSize = buttonHeight;
            renderer->push(TextRenderable(fontSmall, toolNames[i],
                offset + glm::vec2(padding * 2 + iconSize,
                    ((padding * 2 + buttonHeight) - textHeight) / 2 + (buttonHeight + padding * 2) * i),
                glm::vec3(1), 1.f, 1.f));
            renderer->push(QuadRenderable(g_resources.getTexture(icons[i]),
                        offset + glm::vec2(padding, padding + (buttonHeight + padding * 2) * i),
                        iconSize, iconSize));

            buttonOffset.y += buttonHeight + padding * 2 ;
        }

        buttonOffset.y += height * 0.01f;
        slider(buttonOffset, buttonSpacing, str("Brush Radius: ", std::fixed, std::setprecision(1), brushRadius), 2.f, 40.f, brushRadius);
        slider(buttonOffset, buttonSpacing, str("Brush Falloff: ", std::fixed, std::setprecision(1), brushFalloff), 0.2f, 10.f, brushFalloff);
        slider(buttonOffset, buttonSpacing, str("Brush Strength: ", std::fixed, std::setprecision(1), brushStrength), -30.f, 30.f, brushStrength);
        if (terrainTool == TerrainTool::PAINT)
        {
            const char* paintNames[] = { "Paint: Main", "Paint: Rock", "Paint: Offroad", "Paint: Garbage" };
            if (button(buttonOffset, buttonSpacing, paintNames[paintMaterialIndex]))
            {
                paintMaterialIndex = (paintMaterialIndex + 1) % 4;
            }
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
    }
    else if (editMode == EditMode::TRACK)
    {
        if (button(buttonOffset, buttonSpacing, "Connect Points [c]", scene->track->canConnect()) ||
                (g_input.isKeyPressed(KEY_C) && scene->track->canConnect()))
        {
            scene->track->connectPoints();
        }

        if (button(buttonOffset, buttonSpacing, "Subdivide [v]", scene->track->canSubdivide()) ||
                (g_input.isKeyPressed(KEY_V) && scene->track->canSubdivide()))
        {
            scene->track->subdividePoints();
        }

        if (button(buttonOffset, buttonSpacing, "Split [t]", scene->track->canSplit()) ||
                (g_input.isKeyPressed(KEY_V) && scene->track->canSubdivide()))
        {
            scene->track->split();
        }

        if (button(buttonOffset, buttonSpacing, "New Railing"))
        {
            placeMode = PlaceMode::NEW_RAILING;
        }

        if (button(buttonOffset, buttonSpacing, "New Offroad Area"))
        {
            placeMode = PlaceMode::NEW_OFFROAD;
        }

        if (button(buttonOffset, buttonSpacing, "New Track Marking"))
        {
            placeMode = PlaceMode::NEW_MARKING;
        }

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
                u32 itemSize = height * 0.08f;
                u32 iconSize = height * 0.08f;
                u32 gap = height * 0.015f;
                f32 totalWidth = itemSize * ARRAY_SIZE(prefabTrackItems) + gap * (ARRAY_SIZE(prefabTrackItems) - 2);
                f32 cx = g_game.windowWidth * 0.5f;
                f32 yoffset = height * 0.02f;

                for (u32 i=0; i<ARRAY_SIZE(prefabTrackItems); ++i)
                {
                    f32 alpha = 0.6f;
                    glm::vec3 color(0.f);
                    if (pointInRectangle(g_input.getMousePosition(),
                        { cx - totalWidth * 0.5f + ((itemSize + gap) * i), g_game.windowHeight - itemSize - yoffset},
                        itemSize, itemSize))
                    {
                        alpha = 0.9f;
                        color = glm::vec3(0.1f);
                        isMouseClickHandled = true;
                        if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                        {
                            clickHandledUntilRelease = true;
                            scene->track->extendTrack(i);
                        }
                    }

                    glm::vec2 bp( cx - totalWidth * 0.5f + ((itemSize + gap) * i),
                            g_game.windowHeight - itemSize - yoffset);
                    renderer->push(QuadRenderable(white,
                        bp, itemSize, itemSize, color, alpha));
                    renderer->push(QuadRenderable(g_resources.getTexture(prefabTrackItems[i].icon),
                        bp + glm::vec2((itemSize - iconSize)) * 0.5f, iconSize, iconSize));
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
        const char* transformModeNames[] = { "Translate [g]", "Rotate [r]", "Scale [f]" };
        if (button(buttonOffset, buttonSpacing, transformModeNames[(u32)transformMode]) > 0 ||
            g_input.isKeyPressed(KEY_SPACE))
        {
            transformMode = (TransformMode)(((u32)transformMode + 1) % (u32)TransformMode::MAX);
            entityDragAxis = DragAxis::NONE;
        }
        if (button(buttonOffset, buttonSpacing, "Duplicate [b]", selectedEntities.size() > 0)
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

        for (u32 i=0; i<(u32)entityTypes.size(); ++i)
        {
            auto& entityType = entityTypes[i];
            if (button(buttonOffset, buttonSpacing, entityType.name.c_str()))
            {
                selectedEntityTypeIndex = i;
            }
        }

        if (!entityDragAxis && g_input.isKeyPressed(KEY_DELETE))
        {
            for (PlaceableEntity* e : selectedEntities)
            {
                e->destroy();
            }
            selectedEntities.clear();
        }

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
