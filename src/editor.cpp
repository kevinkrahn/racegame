#include "editor.h"
#include "game.h"
#include "input.h"
#include "renderer.h"
#include "scene.h"
#include "input.h"
#include "terrain.h"
#include "mesh_renderables.h"
#include "track.h"
#include "imgui.h"

struct EntityItem
{
    Texture icon;
    bool hasIcon;
    u32 variationIndex;
    u32 entityIndex;
    EditorCategory category;
};
std::vector<EntityItem> editorEntityItems;

Editor::Editor()
{
    if (editorEntityItems.empty())
    {
        for (u32 entityIndex = 0; entityIndex < (u32)g_entities.size(); ++entityIndex)
        {
            auto& e = g_entities[entityIndex];
            if (e.isPlaceableInEditor)
            {
                std::unique_ptr<PlaceableEntity> ptr((PlaceableEntity*)e.create());
                u32 variationCount = ptr->getVariationCount();
                for (u32 i=0; i<variationCount; ++i)
                {
                    editorEntityItems.push_back({
                            {}, false, i, entityIndex, ptr->getEditorCategory(i) });
                }
            }
        }
    }
}

void Editor::onUpdate(Scene* scene, Renderer* renderer, f32 deltaTime)
{
    scene->terrain->setBrushSettings(1.f, 1.f, 1.f, { 0, 0, 1000000 });

    if (g_input.isKeyPressed(KEY_ESCAPE))
    {
        if (scene->isRaceInProgress)
        {
            scene->stopRace();
            scene->deserializeTransientEntities(serializedTransientEntities);
        }
        entityDragAxis = DragAxis::NONE;
    }

    if (scene->isRaceInProgress)
    {
        return;
    }

    bool isMouseClickHandled = ImGui::GetIO().WantCaptureMouse;
    bool isKeyboardHandled = ImGui::GetIO().WantCaptureKeyboard;

    Texture* white = &g_res.textures->white;
    EditMode previousEditMode = editMode;

    if (isKeyboardHandled && g_input.isKeyPressed(KEY_TAB))
    {
        editMode = EditMode(((u32)editMode + 1) % (u32)EditMode::MAX);
    }

    ImGui::Begin("Editor");
    if (ImGui::Button("New"))
    {
        g_game.changeScene(nullptr);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load"))
    {
        std::string filename = chooseFile("tracks/track1.dat", true);
        if (!filename.empty())
        {
            g_game.changeScene(filename.c_str());
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
        std::string filename = scene->filename;
        if (filename.empty())
        {
            filename = chooseFile("tracks/untitled.dat", false);
        }
        if (!filename.empty())
        {
            print("Saving scene to file: ", scene->filename, '\n');
            DataFile::save(scene->serialize(), filename.c_str());
            scene->filename = filename;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save As"))
    {
        std::string filename = chooseFile("tracks/untitled.dat", false);
        if (!filename.empty())
        {
            print("Saving scene to file: ", scene->filename, '\n');
            DataFile::save(scene->serialize(), filename.c_str());
            scene->filename = filename;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Test Drive") || g_input.isKeyPressed(KEY_F5))
    {
        serializedTransientEntities = scene->serializeTransientEntities();
        g_game.state.drivers.clear();
        g_game.state.drivers.push_back(Driver(true, true, true, 0, 0));
        auto conf = g_game.state.drivers.back().getVehicleConfig();
        conf->frontWeaponIndices[0] = 1;
        conf->frontWeaponUpgradeLevel[0] = 5;
        conf->rearWeaponIndices[0] = 5;
        conf->rearWeaponUpgradeLevel[0] = 5;
        conf->specialAbilityIndex = 11;
        scene->terrain->regenerateCollisionMesh(scene);
        scene->startRace();
        entityDragAxis = DragAxis::NONE;
    }

    ImGui::Gap();
    ImGui::InputText("Track Name", &scene->name);
    ImGui::InputTextMultiline("Notes", &scene->notes, {0, 100});
    ImGui::Checkbox("Show Grid", &gridSettings.show);
    ImGui::Checkbox("Snap to Grid", &gridSettings.snap);
    ImGui::InputFloat("Grid Size", &gridSettings.cellSize, 0.1f, 0.f, "%.1f");
    gridSettings.cellSize = clamp(gridSettings.cellSize, 0.1f, 20.f);

    ImGui::Gap();
    if (ImGui::BeginTabBar("Edit Mode"))
    {
        if (ImGui::BeginTabItem("Terrain Mode"))
        {
            editMode = EditMode::TERRAIN;

            ImGui::Spacing();
            if (ImGui::BeginCombo("Terrain Type", scene->terrain->surfaceMaterials[(i32)scene->terrain->terrainType].name))
            {
                for (i32 i=0; i<(i32)ARRAY_SIZE(scene->terrain->surfaceMaterials); ++i)
                {
                    if (ImGui::Selectable(scene->terrain->surfaceMaterials[i].name))
                    {
                        scene->terrain->terrainType = (Terrain::TerrainType)i;
                    }
                }
                ImGui::EndCombo();
            }

            glm::vec2 terrainMin(scene->terrain->x1, scene->terrain->y1);
            glm::vec2 terrainMax(scene->terrain->x2, scene->terrain->y2);
            if (ImGui::InputFloat2("Terrain Min", (f32*)&terrainMin))
            {
                terrainMin = glm::min(terrainMin, terrainMax - 10.f);
                terrainMin = glm::max(terrainMin, -400.f);
                scene->terrain->resize(terrainMin.x, terrainMin.y, terrainMax.x, terrainMax.y);
            }
            if (ImGui::InputFloat2("Terrain Max", (f32*)&terrainMax))
            {
                terrainMax = glm::max(terrainMax, terrainMin + 10.f);
                terrainMax = glm::min(terrainMin, 400.f);
                scene->terrain->resize(terrainMin.x, terrainMin.y, terrainMax.x, terrainMax.y);
            }

            ImGui::Gap();
            ImGui::ListBoxHeader("Terrain Tool", {0, 145});
            const char* toolNames[] = { "Raise / Lower", "Perturb", "Flatten", "Smooth", "Erode", "Match Track", "Paint" };
            for (i32 i=0; i<(i32)TerrainTool::MAX; ++i)
            {
                if (ImGui::Selectable(toolNames[i], i == (i32)terrainTool))
                {
                    terrainTool = (TerrainTool)i;
                }
            }
            ImGui::ListBoxFooter();

            ImGui::Gap();
            ImGui::SliderFloat("Brush Radius", &brushRadius, 2.f, 40.f);
            ImGui::SliderFloat("Brush Falloff", &brushFalloff, 0.2f, 10.f);
            ImGui::SliderFloat("Brush Strength", &brushStrength, -30.f, 30.f);

            if (terrainTool == TerrainTool::PAINT)
            {
                ImGui::ListBoxHeader("Paint Material", {0, 85});
                auto& m = scene->terrain->getSurfaceMaterial();
                for (i32 i=0; i<4; ++i)
                {
                    if (ImGui::Selectable(m.textureNames[i], i == paintMaterialIndex))
                    {
                        paintMaterialIndex = i;
                    }
                }
                ImGui::ListBoxFooter();
            }

            ImGui::EndTabItem();
        }

        auto buttonSize = ImVec2(ImGui::GetWindowWidth() * 0.65f, 0);
        if (ImGui::BeginTabItem("Track Mode"))
        {
            editMode = EditMode::TRACK;

            ImGui::Spacing();
            if (ImGui::Button("Connect Points [c]", buttonSize) || g_input.isKeyPressed(KEY_C))
            {
                scene->track->connectPoints();
            }

            if (ImGui::Button("Connect Railings [c]", buttonSize) || g_input.isKeyPressed(KEY_C))
            {
                scene->track->connectRailings();
            }

            if (ImGui::Button("Subdivide [n]", buttonSize) || g_input.isKeyPressed(KEY_N))
            {
                scene->track->subdividePoints();
            }

            if (ImGui::Button("Split [t]", buttonSize) || g_input.isKeyPressed(KEY_T))
            {
                scene->track->split();
            }

            if (ImGui::Button("Match Highest Z", buttonSize))
            {
                scene->track->matchZ(false);
            }

            if (ImGui::Button("Match Lowest Z", buttonSize))
            {
                scene->track->matchZ(true);
            }

            ImGui::Gap();
            ImGui::ListBoxHeader("Spline Type", {0,85});
            for (i32 i=0; i<(i32)ARRAY_SIZE(Track::railingMeshTypes); ++i)
            {
                if (ImGui::Selectable(scene->track->railingMeshTypes[i].name, i == selectedSplineTypeIndex))
                {
                    selectedSplineTypeIndex = i;
                }
            }
            ImGui::ListBoxFooter();

            if (ImGui::Button("New Spline", buttonSize))
            {
                placeMode = PlaceMode::NEW_SPLINE;
            }

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Decoration Mode"))
        {
            editMode = EditMode::DECORATION;

            ImGui::Spacing();

            TransformMode previousTransformMode = transformMode;

            if (g_input.isKeyPressed(KEY_SPACE))
            {
                transformMode = (TransformMode)(((u32)transformMode + 1) % (u32)TransformMode::MAX);
            }

            ImGui::ListBoxHeader("Transform Mode", {0,70});
            if (ImGui::Selectable("Translate [g]", transformMode == TransformMode::TRANSLATE))
            {
                transformMode = TransformMode::TRANSLATE;
            }
            if (ImGui::Selectable("Rotate [r]", transformMode == TransformMode::ROTATE))
            {
                transformMode = TransformMode::ROTATE;
            }
            if (ImGui::Selectable("Scale [f]", transformMode == TransformMode::SCALE))
            {
                transformMode = TransformMode::SCALE;
            }
            ImGui::ListBoxFooter();

            if (transformMode != previousTransformMode)
            {
                entityDragAxis = DragAxis::NONE;
            }

            ImGui::Gap();

            if (ImGui::Button("Duplicate [b]", buttonSize) > 0 || g_input.isKeyPressed(KEY_B))
            {
                std::vector<PlaceableEntity*> newEntities;
                for (auto e : selectedEntities)
                {
                    auto data = e->serialize();
                    PlaceableEntity* newEntity = (PlaceableEntity*)scene->deserializeEntity(data);
                    newEntity->setPersistent(true);
                    newEntities.push_back(newEntity);
                }
                selectedEntities.clear();
                for (auto e : newEntities)
                {
                    selectedEntities.push_back(e);
                }
            }

            if (ImGui::Button("Delete [DELETE]", buttonSize) || g_input.isKeyPressed(KEY_DELETE))
            {
                for (PlaceableEntity* e : selectedEntities)
                {
                    e->destroy();
                }
                selectedEntities.clear();
            }

            ImGui::Gap();

            ImGui::BeginChild("Entites");
            showEntityIcons();
            ImGui::EndChild();

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();

    if (selectedEntities.size() > 0)
    {
        ImGui::Begin("Entity Properties");
        ImGui::Text("%s", selectedEntities.front()->getName());
        ImGui::Gap();
        selectedEntities.front()->showDetails(scene);
        ImGui::End();
    }

    if (!isKeyboardHandled)
    {
        f32 right = (f32)g_input.isKeyDown(KEY_D) - (f32)g_input.isKeyDown(KEY_A);
        f32 up = (f32)g_input.isKeyDown(KEY_S) - (f32)g_input.isKeyDown(KEY_W);
        glm::vec3 moveDir = (right != 0.f || up != 0.f) ? glm::normalize(glm::vec3(right, up, 0.f)) : glm::vec3(0, 0, 0);
        glm::vec3 forward(lengthdir(cameraAngle, 1.f), 0.f);
        glm::vec3 sideways(lengthdir(cameraAngle + PI / 2.f, 1.f), 0.f);

        cameraVelocity += (((forward * moveDir.y) + (sideways * moveDir.x)) * (deltaTime * (120.f + cameraDistance * 1.5f)));
    }
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

    if (!isKeyboardHandled && g_input.isKeyPressed(KEY_SPACE))
    {
        terrainTool = TerrainTool(((u32)terrainTool + 1) % (u32)TerrainTool::MAX);
    }

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
        zoomSpeed = g_input.getMouseScroll() * 1.1f;
    }
    cameraDistance = clamp(cameraDistance - zoomSpeed, 10.f, 200.f);
    zoomSpeed = smoothMove(zoomSpeed, 0.f, 10.f, deltaTime);

    RenderWorld* rw = renderer->getRenderWorld();
    glm::vec3 cameraFrom = cameraTarget + glm::normalize(glm::vec3(lengthdir(cameraAngle, 1.f), 1.25f)) * cameraDistance;
    rw->setViewportCamera(0, cameraFrom, cameraTarget, 5.f, 400.f);

    Camera const& cam = rw->getCamera(0);
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
                if (placeMode == PlaceMode::NEW_SPLINE)
                {
                    scene->track->placeSpline(p, selectedSplineTypeIndex);
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
                f32 totalWidth = (f32)(itemSize * ARRAY_SIZE(Track::prefabTrackItems) + gap * (ARRAY_SIZE(Track::prefabTrackItems) - 2));
                f32 cx = g_game.windowWidth * 0.5f;
                f32 yoffset = height * 0.02f;

                for (u32 i=0; i<ARRAY_SIZE(Track::prefabTrackItems); ++i)
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

                    glm::vec2 bp(cx - totalWidth * 0.5f + ((itemSize + gap) * i),
                            g_game.windowHeight - itemSize - yoffset);
                    renderer->push2D(QuadRenderable(white,
                        bp, (f32)itemSize, (f32)itemSize, color, alpha));
                    renderer->push2D(QuadRenderable(&scene->track->prefabTrackItems[i].icon,
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
            glm::mat4 viewProj = rw->getCamera(0).viewProjection;

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

                Mesh* arrowMesh = g_res.getMesh("world.TranslateArrow");
                Mesh* centerMesh = g_res.getMesh("world.Sphere");
                rw->push(OverlayRenderable(centerMesh, 0,
                        glm::translate(glm::mat4(1.f), p), centerCol, -1));

                rw->push(OverlayRenderable(arrowMesh, 0,
                        glm::translate(glm::mat4(1.f), p), xCol));
                if (entityDragAxis & DragAxis::X)
                {
                    scene->debugDraw.line(
                            p - glm::vec3(100.f, 0.f, 0.f), p + glm::vec3(100.f, 0.f, 0.f),
                            glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1));
                }

                rw->push(OverlayRenderable(arrowMesh, 0,
                        glm::translate(glm::mat4(1.f), p) *
                        glm::rotate(glm::mat4(1.f), rot, glm::vec3(0, 0, 1)), yCol));
                if (entityDragAxis & DragAxis::Y)
                {
                    scene->debugDraw.line(
                            p - glm::vec3(0.f, 100.f, 0.f), p + glm::vec3(0.f, 100.f, 0.f),
                            glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1));
                }

                rw->push(OverlayRenderable(arrowMesh, 0,
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
                    Mesh* arrowMesh = g_res.getMesh("world.RotateArrow");
                    Mesh* sphereMesh = g_res.getMesh("world.Sphere");
                    rw->push(OverlayRenderable(sphereMesh, 0,
                            glm::translate(glm::mat4(1.f), p) * glm::scale(glm::mat4(1.f), glm::vec3(4.4f))
                            , {0,0,0}, -1, true));

                    rw->push(OverlayRenderable(arrowMesh, 0,
                            glm::translate(glm::mat4(1.f), p) *
                            glm::rotate(glm::mat4(1.f), rot, glm::vec3(0, 1, 0)), xCol));
                    if (entityDragAxis & DragAxis::X)
                    {
                        scene->debugDraw.line(
                                p - glm::vec3(100.f, 0.f, 0.f), p + glm::vec3(100.f, 0.f, 0.f),
                                glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1));
                    }

                    rw->push(OverlayRenderable(arrowMesh, 0,
                            glm::translate(glm::mat4(1.f), p) *
                            glm::rotate(glm::mat4(1.f), rot, glm::vec3(1, 0, 0)), yCol));
                    if (entityDragAxis & DragAxis::Y)
                    {
                        scene->debugDraw.line(
                                p - glm::vec3(0.f, 100.f, 0.f), p + glm::vec3(0.f, 100.f, 0.f),
                                glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1));
                    }

                    rw->push(OverlayRenderable(arrowMesh, 0,
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

                Mesh* arrowMesh = g_res.getMesh("world.ScaleArrow");
                Mesh* centerMesh = g_res.getMesh("world.UnitCube");
                rw->push(OverlayRenderable(centerMesh, 0,
                        glm::translate(glm::mat4(1.f), p) * orientation, centerCol, -1));

                rw->push(OverlayRenderable(arrowMesh, 0,
                        glm::translate(glm::mat4(1.f), p) * orientation, xCol));
                if (entityDragAxis & DragAxis::X)
                {
                    scene->debugDraw.line(
                            p - glm::vec3(100.f, 0.f, 0.f), p + glm::vec3(100.f, 0.f, 0.f),
                            glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1));
                }

                rw->push(OverlayRenderable(arrowMesh, 0,
                        glm::translate(glm::mat4(1.f), p) *
                        orientation *
                        glm::rotate(glm::mat4(1.f), rot, glm::vec3(0, 0, 1)), yCol));
                if (entityDragAxis & DragAxis::Y)
                {
                    scene->debugDraw.line(
                            p - glm::vec3(0.f, 100.f, 0.f), p + glm::vec3(0.f, 100.f, 0.f),
                            glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1));
                }

                rw->push(OverlayRenderable(arrowMesh, 0,
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
                            (PlaceableEntity*)g_entities[editorEntityItems[selectedEntityTypeIndex].entityIndex].create();
                        newEntity->setVariationIndex(editorEntityItems[selectedEntityTypeIndex].variationIndex);
                        newEntity->position = hitPoint;
                        newEntity->updateTransform(scene);
                        newEntity->setPersistent(true);
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
            e->onEditModeRender(rw, scene, isSelected);
        }
    }

    if (gridSettings.show && editMode != EditMode::TERRAIN)
    {
        i32 count = (i32)(40.f / gridSettings.cellSize);
        f32 gridSize = count * gridSettings.cellSize;
        glm::vec4 color = { 1.f, 1.f, 1.f, 0.2f };
        for (i32 i=-count; i<=count; i++)
        {
            f32 x = i * gridSettings.cellSize;
            f32 y = i * gridSettings.cellSize;
            scene->debugDraw.line(
                { snap(cameraTarget.x + x, gridSettings.cellSize), snap(cameraTarget.y - gridSize, gridSettings.cellSize), gridSettings.z },
                { snap(cameraTarget.x + x, gridSettings.cellSize), snap(cameraTarget.y + gridSize, gridSettings.cellSize), gridSettings.z },
                color, color);
            scene->debugDraw.line(
                { snap(cameraTarget.x - gridSize, gridSettings.cellSize), snap(cameraTarget.y + y, gridSettings.cellSize), gridSettings.z },
                { snap(cameraTarget.x + gridSize, gridSettings.cellSize), snap(cameraTarget.y + y, gridSettings.cellSize), gridSettings.z },
                color, color);
        }
    }
}

std::string chooseFile(const char* defaultSelection, bool open)
{
#if _WIN32
    char szFile[260];

    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All\0*.*\0Tracks\0*.dat\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    if (open)
    {
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    }

    if (open)
    {
        if (GetOpenFileName(&ofn) == TRUE)
        {
            return std::string(szFile);
        }
        else
        {
            return {};
        }
    }
    else
    {
        if (GetSaveFileName(&ofn) == TRUE)
        {
            return std::string(szFile);
        }
        else
        {
            return {};
        }
    }
#else
    char filename[1024] = { 0 };
    std::string cmd = "zenity";
    if (open)
    {
        cmd += " --title 'Open Track' --file-selection --filename ";
        cmd += defaultSelection;
    }
    else
    {
        cmd += " --title 'Save Track' --file-selection --save --confirm-overwrite --filename ";
        cmd += defaultSelection;
    }
    FILE *f = popen(cmd.c_str(), "r");
    if (!f || !fgets(filename, sizeof(filename) - 1, f))
    {
        error("Unable to create file dialog\n");
        return {};
    }
    pclose(f);
    std::string file(filename);
    if (!file.empty())
    {
        file.pop_back();
    }
    return file;
#endif
}

void Editor::showEntityIcons()
{
    u32 count = (u32)editorEntityItems.size();
    u32 iconSize = 48;

    static RenderWorld renderWorld;
    static i32 lastEntityIconRendered = -1;

    if (lastEntityIconRendered != -1)
    {
        editorEntityItems[lastEntityIconRendered].icon = renderWorld.releaseTexture();
        editorEntityItems[lastEntityIconRendered].hasIcon = true;
        lastEntityIconRendered = -1;
    }
    for (u32 categoryIndex=0; categoryIndex<ARRAY_SIZE(editorCategoryNames); ++categoryIndex)
    {
        if (!ImGui::CollapsingHeader(editorCategoryNames[categoryIndex]))
        {
            continue;
        }

        u32 buttonCount = 0;
        for (u32 i=0, itemIndex = 0; itemIndex<count; ++itemIndex)
        {
            if ((u32)editorEntityItems[itemIndex].category != categoryIndex)
            {
                continue;
            }

            if (editorEntityItems[itemIndex].hasIcon)
            {
                if (buttonCount % 5 != 0)
                {
                    ImGui::SameLine();
                }
                ImGui::PushID(itemIndex);
                if (ImGui::ImageButton((void*)(u64)editorEntityItems[itemIndex].icon.handle,
                            ImVec2(iconSize, iconSize), {1,1}, {0,0}))
                {
                    selectedEntityTypeIndex = (i32)itemIndex;
                }
                ImGui::PopID();
                ++buttonCount;
            }
            else if (lastEntityIconRendered == -1)
            {
                renderWorld.setName("Entity Icon");
                renderWorld.setSize(128, 128);
                Mesh* quadMesh = g_res.getMesh("world.Quad");
                renderWorld.push(LitRenderable(quadMesh,
                            glm::scale(glm::mat4(1.f), glm::vec3(120.f)), nullptr, glm::vec3(0.15f)));
                renderWorld.addDirectionalLight(glm::vec3(-0.5f, 0.2f, -1.f), glm::vec3(1.5f));
                renderWorld.setViewportCount(1);
                renderWorld.updateWorldTime(30.f);
                renderWorld.setViewportCamera(0, glm::vec3(8.f, 8.f, 10.f),
                        glm::vec3(0.f, 0.f, 1.f), 1.f, 200.f, 40.f);
                static std::unique_ptr<PlaceableEntity> ptr;
                ptr.reset((PlaceableEntity*)
                        g_entities[editorEntityItems[itemIndex].entityIndex].create());
                ptr->setVariationIndex(editorEntityItems[itemIndex].variationIndex);
                ptr->onPreview(&renderWorld);
                g_game.renderer->addRenderWorld(&renderWorld);
                lastEntityIconRendered = (i32)itemIndex;
            }
            ++i;
        }
    }
}
