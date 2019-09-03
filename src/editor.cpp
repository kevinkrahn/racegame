#include "editor.h"
#include "game.h"
#include "input.h"
#include "renderer.h"
#include "scene.h"
#include "input.h"
#include "terrain.h"
#include "mesh_renderables.h"
#include "track.h"

void Editor::onStart(Scene* scene)
{

}

void Editor::onUpdate(Scene* scene, Renderer* renderer, f32 deltaTime)
{
    scene->terrain->setBrushSettings(0.f, 0.f, 0.f, {});

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
            scene->startRace(scene->track->getStart());
        }
    }
    else if (g_input.isKeyPressed(KEY_ESCAPE))
    {
        if (scene->isRaceInProgress)
        {
            scene->stopRace();
        }
    }

    if (scene->isRaceInProgress)
    {
        return;
    }

    renderer->setViewportCount(1);

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
        f32 alpha = 0.75f;
        f32 textAlpha = 1.f;
        glm::vec3 color(0.f);
        bool wasClicked = false;
        u32 buttonWidth = height * 0.15f;
        u32 buttonHeight = height * 0.04f;
        if (!enabled)
        {
            alpha = 0.5f;
            textAlpha = 0.75f;
        }
        if (enabled && pointInRectangle(mousePos, pos, buttonWidth, buttonHeight))
        {
            alpha = 0.9f;
            color = glm::vec3(0.3f);
            isMouseClickHandled = true;
            clickHandledUntilRelease = true;
            if (g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
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
        f32 alpha = 0.75f;
        f32 textAlpha = 1.f;
        glm::vec3 color(0.f);
        bool wasClicked = false;
        u32 buttonWidth = height * 0.15f;
        u32 buttonHeight = height * 0.04f;
        if (pointInRectangle(mousePos, pos, buttonWidth, buttonHeight))
        {
            alpha = 0.9f;
            color = glm::vec3(0.3f);
            if (g_input.isMouseButtonDown(MOUSE_LEFT))
            {
                isMouseClickHandled = true;
                clickHandledUntilRelease = true;
                wasClicked = true;
                f32 t = (mousePos.x - pos.x) / buttonWidth;
                val = min + (max - min) * t;
            }
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
    renderer->setViewportCamera(0, cameraFrom, cameraTarget, 10, 400.f);

    Camera const& cam = renderer->getCamera(0);
    glm::vec3 rayDir = screenToWorldRay(mousePos,
            glm::vec2(g_game.windowWidth, g_game.windowHeight), cam.view, cam.projection);

    if (g_input.isKeyPressed(KEY_TAB))
    {
        editMode = EditMode(((u32)editMode + 1) % (u32)EditMode::MAX);
        scene->terrain->regenerateCollisionMesh(scene);
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
            f32 alpha = 0.75f;
            glm::vec3 color(0.f);
            if (i == (u32)editMode)
            {
                buttonWidth = height * 0.20f;
                alpha = 0.9f;
                color = glm::vec3(0.3f);
            }
            if (pointInRectangle(g_input.getMousePosition(),
                        offset + glm::vec2(0, (buttonHeight + padding * 2) * i),
                        buttonWidth + padding * 2, buttonHeight + padding * 2))
            {
                alpha = 0.9f;
                color = glm::vec3(0.3f);
                isMouseClickHandled = true;
                if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                {
                    editMode = EditMode(i);
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

    if (editMode != EditMode::TERRAIN)
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

        const char* toolNames[] = { "Raise / Lower", "Perturb", "Flatten", "Smooth", "Erode", "Match Track" };
        // TODO: icons
        const char* icons[] = { "terrain_icon", "terrain_icon", "terrain_icon", "terrain_icon", "terrain_icon", "terrain_icon" };
        glm::vec2 offset = buttonOffset;
        u32 padding = height * 0.01f;
        u32 buttonHeight = height * 0.02f;
        f32 textHeight = fontSmall->getHeight();
        for (u32 i=0; i<(u32)TerrainTool::MAX; ++i)
        {
            u32 buttonWidth = height * 0.12f;
            f32 alpha = 0.75f;
            glm::vec3 color(0.f);
            if (i == (u32)terrainTool)
            {
                buttonWidth = height * 0.15f;
                alpha = 0.9f;
                color = glm::vec3(0.3f);
            }
            if (pointInRectangle(g_input.getMousePosition(),
                        offset + glm::vec2(0, (buttonHeight + padding * 2) * i),
                        buttonWidth + padding * 2, buttonHeight + padding * 2))
            {
                alpha = 0.9f;
                color = glm::vec3(0.3f);
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
                default:
                    break;
                }
            }
        }
    }
    else if (editMode == EditMode::TRACK)
    {
        if (button(buttonOffset, buttonSpacing, "Connect Points [C]", scene->track->canConnect()) ||
                (g_input.isKeyPressed(KEY_C) && scene->track->canConnect()))
        {
            scene->track->connectPoints();
        }

        if (button(buttonOffset, buttonSpacing, "Subdivide [V]", scene->track->canSubdivide()) ||
                (g_input.isKeyPressed(KEY_V) && scene->track->canSubdivide()))
        {
            scene->track->subdividePoints();
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
                scene->track->placeRailing(p);
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
                i32 pointIndex = scene->track->getSelectedPointIndex();
                Track::Point const& point = scene->track->getPoint(pointIndex);
                gridSettings.z = point.position.z + 0.15f;

                u32 itemSize = height * 0.08f;
                u32 iconSize = height * 0.08f;
                u32 gap = height * 0.015f;
                f32 totalWidth = itemSize * ARRAY_SIZE(prefabTrackItems) + gap * (ARRAY_SIZE(prefabTrackItems) - 2);
                f32 cx = g_game.windowWidth * 0.5f;
                f32 yoffset = height * 0.02f;

                for (u32 i=0; i<ARRAY_SIZE(prefabTrackItems); ++i)
                {
                    f32 alpha = 0.3f;
                    glm::vec3 color(0.f);
                    if (pointInRectangle(g_input.getMousePosition(),
                        { cx - totalWidth * 0.5f + ((itemSize + gap) * i), g_game.windowHeight - itemSize - yoffset},
                        itemSize, itemSize))
                    {
                        alpha = 0.75f;
                        color = glm::vec3(0.3f);
                        isMouseClickHandled = true;
                        if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                        {
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
