#pragma once

#include "editor_mode.h"
#include "../input.h"
#include "../renderer.h"
#include "../imgui.h"
#include "../scene.h"
#include "../game.h"
#include "../mesh_renderables.h"

struct EntityItem
{
    Texture icon;
    bool hasIcon;
    u32 variationIndex;
    u32 entityIndex;
    EditorCategory category;
};
std::vector<EntityItem> editorEntityItems;

class DecorationMode : public EditorMode
{
    enum struct TransformMode : i32
    {
        TRANSLATE,
        ROTATE,
        SCALE,
        MAX
    } transformMode = TransformMode::TRANSLATE;
    i32 entityDragAxis = DragAxis::NONE;
    glm::vec3 entityDragOffset;
    glm::vec3 rotatePivot;

    std::vector<PlaceableEntity*> selectedEntities;

    void showEntityIcons()
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
                    if (ImGui::ImageButton((void*)(uintptr_t)editorEntityItems[itemIndex].icon.handle,
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

    std::vector<DataFile::Value> serializedTransientEntities;
    i32 selectedEntityTypeIndex = 0;

public:
    DecorationMode() : EditorMode("Props")
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

    void onUpdate(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
        if (g_input.isKeyPressed(KEY_ESCAPE))
        {
            entityDragAxis = DragAxis::NONE;
        }

        if (selectedEntities.size() > 0)
        {
            ImGui::Begin("Entity Properties");
            ImGui::Text("%s", selectedEntities.front()->getName());
            ImGui::Gap();
            selectedEntities.front()->showDetails(scene);
            ImGui::End();
        }

        bool isMouseClickHandled = ImGui::GetIO().WantCaptureMouse;
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

        RenderWorld* rw = renderer->getRenderWorld();
        glm::vec3 rayDir = scene->getEditorCamera().getMouseRay(rw);
        Camera const& cam = scene->getEditorCamera().getCamera();

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
                glm::vec2 mousePos = g_input.getMousePosition();
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

    void onEditorTabGui(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
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

        auto buttonSize = ImVec2(ImGui::GetWindowWidth() * 0.65f, 0);
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
    }

    void onBeginTest(Scene* scene) override
    {
        serializedTransientEntities = scene->serializeTransientEntities();
        entityDragAxis = DragAxis::NONE;
    }

    void onEndTest(Scene* scene) override
    {
        scene->deserializeTransientEntities(serializedTransientEntities);
    }

    void onSwitchFrom(Scene* scene) override
    {
        entityDragAxis = DragAxis::NONE;
    }
};
