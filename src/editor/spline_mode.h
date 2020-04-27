#pragma once

#include "transform_gizmo.h"
#include "editor_mode.h"
#include "editor.h"
#include "../input.h"
#include "../renderer.h"
#include "../imgui.h"
#include "../scene.h"
#include "../spline.h"

class SplineMode : public EditorMode, public TransformGizmoHandler
{
    struct SplineMeshInfo
    {
        Model* model = nullptr;
        Texture icon;
        bool hasIcon = false;
    };

    std::vector<SplineMeshInfo> splineModels;
    u32 selectedSplineModel = 0;

    struct PointSelection
    {
        Spline* spline;
        u32 pointIndex;
    };

    struct HandleSelection
    {
        Spline* spline;
        u32 pointIndex;
        bool firstHandle;
    };

    std::vector<PointSelection> selectedPoints;
    std::vector<HandleSelection> selectedHandles;
    Spline* selectedSpline = nullptr;
    TransformGizmo transformGizmo;

    std::vector<Spline*> splineEntities;

    void clearSelection()
    {
        selectedPoints.clear();
        selectedHandles.clear();
    }

    bool raySphereIntersection(Camera const& cam, glm::vec3 const& rayDir,
            glm::vec3 const& spherePosition, f32 fixedSize)
    {
        f32 dist = glm::length(cam.position - spherePosition);
        f32 size = dist * fixedSize * glm::radians(cam.fov);
        if (::raySphereIntersection(cam.position, rayDir, spherePosition, size) > 0.f)
        {
            return true;
        }
        return false;
    }

    void connectSplines()
    {
        Spline* splineA = nullptr;
        Spline* splineB = nullptr;
        u32 pointIndexA = 0;
        u32 pointIndexB = 0;
        if (selectedPoints.size() != 2)
        {
            return;
        }
        for (auto& selection : selectedPoints)
        {
            if (!splineA)
            {
                splineA = selection.spline;
                pointIndexA = selection.pointIndex;
            }
            else
            {
                splineB = selection.spline;
                pointIndexB = selection.pointIndex;
                break;
            }
        }
        if (!splineA || !splineB)
        {
            return;
        }

        if (pointIndexA == splineA->points.size() - 1 &&
            pointIndexB == splineB->points.size() - 1)
        {
            for (auto it = splineB->points.rbegin(); it != splineB->points.rend(); ++it)
            {
                SplinePoint point = *it;
                std::swap(point.handleOffsetA, point.handleOffsetB);
                splineA->points.push_back(point);
            }
        }
        else if (pointIndexA == splineA->points.size() - 1 &&
            pointIndexB == 0)
        {
            for (auto it = splineB->points.begin(); it!=splineB->points.end(); ++it)
            {
                splineA->points.push_back(*it);
            }
        }
        else if (pointIndexA == 0 &&
            pointIndexB == splineB->points.size() - 1)
        {
            for (auto it = splineB->points.rbegin(); it != splineB->points.rend(); ++it)
            {
                splineA->points.insert(splineA->points.begin(), *it);
            }
        }
        else if (pointIndexA == 0 &&
            pointIndexB == 0)
        {
            for (auto it = splineB->points.begin(); it!=splineB->points.end(); ++it)
            {
                SplinePoint point = *it;
                std::swap(point.handleOffsetA, point.handleOffsetB);
                splineA->points.insert(splineA->points.begin(), point);
            }
        }
        else
        {
            return;
        }
        selectedPoints.clear();
        splineA->isDirty = true;
        splineB->destroy();
    }

    void subdividePoints()
    {
        if (selectedPoints.size() != 2
                || selectedPoints[0].spline != selectedPoints[1].spline)
        {
            return;
        }

        if (glm::abs((i32)selectedPoints[0].pointIndex - (i32)selectedPoints[1].pointIndex) != 1)
        {
            return;
        }

        Spline* spline = selectedPoints[0].spline;

        u32 pointIndexA = selectedPoints[0].pointIndex;
        u32 pointIndexB = selectedPoints[1].pointIndex;
        if (pointIndexB < pointIndexA)
        {
            std::swap(pointIndexA, pointIndexB);
        }
        SplinePoint& p1 = spline->points[pointIndexA];
        SplinePoint& p2 = spline->points[pointIndexB];
        glm::vec3 midPoint = pointOnBezierCurve(p1.position, p1.position + p1.handleOffsetB,
                p2.position + p2.handleOffsetA, p2.position, 0.5f);
        glm::vec3 handleA = pointOnBezierCurve(p1.position, p1.position + p1.handleOffsetB,
                p2.position + p2.handleOffsetA, p2.position, 0.4f) - midPoint;
        glm::vec3 handleB = pointOnBezierCurve(p1.position, p1.position + p1.handleOffsetB,
                p2.position + p2.handleOffsetA, p2.position, 0.6f) - midPoint;
        spline->points.insert(spline->points.begin() + pointIndexB, {
            midPoint,
            handleA,
            handleB
        });
        spline->isDirty = true;
    }

    void splitSpline(Scene* scene)
    {
        if (selectedPoints.empty())
        {
            return;
        }

        Spline* spline = selectedPoints[0].spline;
        if (selectedPoints.front().pointIndex == 0 ||
            selectedPoints.front().pointIndex == spline->points.size() - 1)
        {
            return;
        }
        Spline* newSpline = (Spline*)g_entities[5].create();
        newSpline->setPersistent(true);

        newSpline->points = std::vector<SplinePoint>(
                spline->points.begin() + selectedPoints.front().pointIndex,
                spline->points.end());
        newSpline->scale = spline->scale;
        newSpline->modelGuid = spline->modelGuid;
        spline->points.erase(
                spline->points.begin() + selectedPoints.front().pointIndex + 1,
                spline->points.end());
        selectedPoints.clear();

        spline->isDirty = true;
        newSpline->isDirty = true;

        scene->addEntity(newSpline);
    }

    void deletePoints()
    {
        if (selectedPoints.empty())
        {
            return;
        }

        for (auto& selection : selectedPoints)
        {
            selection.spline->points.erase(selection.spline->points.begin() + selection.pointIndex);
            if (selection.spline->points.empty())
            {
                selection.spline->destroy();
            }
        }
        selectedPoints.clear();
    }

    void matchZ(bool lowest)
    {
        if (selectedPoints.empty())
        {
            return;
        }

        f32 z = selectedPoints[0].spline->points[0].position.z;
        for (auto& selection : selectedPoints)
        {
            if (lowest)
            {
                if (selection.spline->points[selection.pointIndex].position.z < z)
                {
                    z = selection.spline->points[selection.pointIndex].position.z;
                }
            }
            else
            {
                if (selection.spline->points[selection.pointIndex].position.z > z)
                {
                    z = selection.spline->points[selection.pointIndex].position.z;
                }
            }
        }
        for (auto& selection : selectedPoints)
        {
            selection.spline->points[selection.pointIndex].position.z = z;
        }
    }

public:
    SplineMode() : EditorMode("Splines")
    {
        for (auto& model : g_res.models)
        {
            if (model.second->modelUsage == ModelUsage::SPLINE)
            {
                splineModels.push_back({ model.second.get() });
            }
        }
    }

    void onUpdate(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
        bool isMouseClickHandled = ImGui::GetIO().WantCaptureMouse;

        RenderWorld* rw = renderer->getRenderWorld();
        glm::vec3 rayDir = scene->getEditorCamera().getMouseRay(rw);
        Camera const& cam = scene->getEditorCamera().getCamera();

        if (!selectedPoints.empty() || !selectedHandles.empty())
        {
            glm::vec3 minP(FLT_MAX);
            glm::vec3 maxP(-FLT_MAX);
            for (auto& selection : selectedPoints)
            {
                minP = glm::min(minP, selection.spline->points[selection.pointIndex].position);
                maxP = glm::max(maxP, selection.spline->points[selection.pointIndex].position);
            }
            for (auto& selection : selectedHandles)
            {
                if (selection.firstHandle)
                {
                    minP = glm::min(minP, selection.spline->points[selection.pointIndex].position
                            + selection.spline->points[selection.pointIndex].handleOffsetA);
                    maxP = glm::max(maxP, selection.spline->points[selection.pointIndex].position
                            + selection.spline->points[selection.pointIndex].handleOffsetA);
                }
                else
                {
                    minP = glm::min(minP, selection.spline->points[selection.pointIndex].position
                            + selection.spline->points[selection.pointIndex].handleOffsetB);
                    maxP = glm::max(maxP, selection.spline->points[selection.pointIndex].position
                            + selection.spline->points[selection.pointIndex].handleOffsetB);
                }
            }
            glm::vec3 gizmoPosition = minP + (maxP - minP) * 0.5f;
            if (transformGizmo.update(gizmoPosition, scene, rw,
                    deltaTime, glm::mat4(1.f), this, false))
            {
                isMouseClickHandled = true;
            }
        }

        splineEntities.clear();
        for (auto& e : scene->getEntities())
        {
            if (e->entityID == 5)
            {
                splineEntities.push_back((Spline*)e.get());
            }
        }

        // create new splines
        if (!isMouseClickHandled && g_input.isKeyDown(KEY_LCTRL) && g_input.isKeyDown(KEY_LSHIFT)
                && g_input.isMouseButtonPressed(MOUSE_LEFT))
        {
            isMouseClickHandled = true;
            PxRaycastBuffer hit;
            if (scene->raycastStatic(cam.position, rayDir, 10000.f, &hit))
            {
                glm::vec3 hitPoint = convert(hit.block.position);
                Spline* newSpline = (Spline*)g_entities[5].create();
                newSpline->modelGuid = splineModels[selectedSplineModel].model->guid;
                newSpline->setPersistent(true);
                glm::vec3 handleOffset(5, 0, 0);
                newSpline->points.push_back(
                        { hitPoint, -handleOffset, handleOffset });
                newSpline->points.push_back(
                        { hitPoint + glm::vec3(20, 0, 0), -handleOffset, handleOffset });
                newSpline->isDirty = true;
                scene->addEntity(newSpline);
            }
        }

        // select things
        if (!isMouseClickHandled && g_input.isMouseButtonPressed(MOUSE_LEFT))
        {
            selectedSpline = nullptr;

            if (!g_input.isKeyDown(KEY_LCTRL) && !g_input.isKeyDown(KEY_LSHIFT))
            {
                clearSelection();
            }
            bool hitPoint = false;

            // check if point was clicked on
            for (auto& spline : splineEntities)
            {
                for (u32 i=0; i<spline->points.size(); ++i)
                {
                    if (raySphereIntersection(cam, rayDir, spline->points[i].position, 0.015f))
                    {
                        bool alreadySelected = false;
                        for (u32 j=0; j<selectedPoints.size(); ++j)
                        {
                            if (selectedPoints[j].pointIndex == i && selectedPoints[j].spline == spline)
                            {
                                alreadySelected = true;
                                if (g_input.isKeyDown(KEY_LSHIFT))
                                {
                                    selectedPoints.erase(selectedPoints.begin() + j);
                                }
                                break;
                            }
                        }
                        if (!alreadySelected)
                        {
                            selectedPoints.push_back({ spline, i });
                        }
                        hitPoint = true;
                        break;
                    }
                }

                if (hitPoint)
                {
                    break;
                }
            }

            if (!hitPoint)
            {
                selectedPoints.clear();
            }

            // check if handles are clicked on
            bool hitHandle = false;
            for (auto& spline : splineEntities)
            {
                for (u32 i=0; i<spline->points.size(); ++i)
                {
                    // handle A
                    if (raySphereIntersection(cam, rayDir,
                            spline->points[i].position + spline->points[i].handleOffsetA, 0.012f))
                    {
                        bool alreadySelected = false;
                        for (u32 j=0; j<selectedPoints.size(); ++j)
                        {
                            if (selectedHandles[j].pointIndex == i
                                    && selectedHandles[j].spline == spline
                                    && selectedHandles[j].firstHandle)
                            {
                                alreadySelected = true;
                                if (g_input.isKeyDown(KEY_LSHIFT))
                                {
                                    selectedHandles.erase(selectedHandles.begin() + j);
                                }
                                break;
                            }
                        }
                        if (!alreadySelected)
                        {
                            selectedHandles.push_back({ spline, i, true });
                        }
                        hitHandle = true;
                        break;
                    }

                    // handle B
                    if (raySphereIntersection(cam, rayDir,
                            spline->points[i].position + spline->points[i].handleOffsetB, 0.012f))
                    {
                        bool alreadySelected = false;
                        for (u32 j=0; j<selectedPoints.size(); ++j)
                        {
                            if (selectedHandles[j].pointIndex == i
                                    && selectedHandles[j].spline == spline
                                    && !selectedHandles[j].firstHandle)
                            {
                                alreadySelected = true;
                                if (g_input.isKeyDown(KEY_LSHIFT))
                                {
                                    selectedHandles.erase(selectedHandles.begin() + j);
                                }
                                break;
                            }
                        }
                        if (!alreadySelected)
                        {
                            selectedHandles.push_back({ spline, i, false });
                        }
                        hitHandle = true;
                        break;
                    }
                }

                if (hitHandle)
                {
                    break;
                }
            }

            if (!hitHandle)
            {
                selectedHandles.clear();
            }

            if (!hitPoint && !hitHandle)
            {
                PxRaycastBuffer hit;
                if (scene->raycastStatic(cam.position, rayDir, 10000.f, &hit, COLLISION_FLAG_SPLINE))
                {
                    if (hit.block.actor)
                    {
                        ActorUserData* userData = (ActorUserData*)hit.block.actor->userData;
                        if (userData->entityType == ActorUserData::ENTITY)
                        {
                            selectedSpline = (Spline*)userData->entity;
                        }
                    }
                }
            }
            isMouseClickHandled = true;
        }

        if (selectedSpline)
        {
            selectedSpline->onRenderWireframe(rw, scene, deltaTime);
            showSelectedSplineProperties();
        }

        Mesh* sphereMesh = g_res.getMesh("world.Sphere");
        for (auto spline : splineEntities)
        {
            glm::vec4 orange = { 1.f, 0.5f, 0.f, 1.f };
            glm::vec4 red = { 1.f, 0.f, 0.f, 1.f };
            glm::vec4 white = { 1.f, 1.f, 1.f, 1.f };
            for (u32 i=0; i<spline->points.size(); ++i)
            {
                auto& point = spline->points[i];

                glm::mat4 transform = glm::translate(glm::mat4(1.f), point.position);
                bool isSelected = std::find_if(selectedPoints.begin(), selectedPoints.end(), [&](auto& p) {
                    return p.spline == spline && p.pointIndex == i;
                }) != selectedPoints.end();
                glm::vec3 color = isSelected ? white : red;
                rw->push(OverlayRenderable(sphereMesh, 0, transform, color));

                isSelected = std::find_if(selectedHandles.begin(), selectedHandles.end(), [&](auto& p) {
                    return p.spline == spline && p.pointIndex == i && p.firstHandle;
                }) != selectedHandles.end();
                color = isSelected ? white : orange;
                transform = glm::translate(glm::mat4(1.f), point.position + point.handleOffsetA) *
                            glm::scale(glm::mat4(1.f), glm::vec3(0.8f));
                rw->push(OverlayRenderable(sphereMesh, 0, transform, color));

                isSelected = std::find_if(selectedHandles.begin(), selectedHandles.end(), [&](auto& p) {
                    return p.spline == spline && p.pointIndex == i && !p.firstHandle;
                }) != selectedHandles.end();
                color = isSelected ? white : orange;
                transform = glm::translate(glm::mat4(1.f), point.position + point.handleOffsetB) *
                            glm::scale(glm::mat4(1.f), glm::vec3(0.8f));
                rw->push(OverlayRenderable(sphereMesh, 0, transform, color));

                glm::vec4 c(color, 1.f);
                scene->debugDraw.line(point.position, point.position + point.handleOffsetA, c, c);
                scene->debugDraw.line(point.position, point.position + point.handleOffsetB, c, c);
            }
        }
    }

    void showSelectedSplineProperties()
    {
        if (ImGui::Begin("Spline Mesh Properties"))
        {
            Model* model = g_res.getModel(selectedSpline->modelGuid);
            ImGui::Text(model->name.c_str());
            ImGui::End();
        }
    }

    void showSplineModelIcons()
    {
        static RenderWorld renderWorld;
        static i32 lastSplineModelRenderered = -1;
        u32 count = (u32)splineModels.size();
        u32 iconSize = 64;

        if (lastSplineModelRenderered != -1)
        {
            splineModels[lastSplineModelRenderered].icon = renderWorld.releaseTexture();
            splineModels[lastSplineModelRenderered].hasIcon = true;
            lastSplineModelRenderered = -1;
        }
        for (u32 i=0; i<count; ++i)
        {
            if (splineModels[i].hasIcon)
            {
                if (i % 4 != 0)
                {
                    ImGui::SameLine();
                }
                ImGui::PushID(i);
                if (ImGui::ImageButton((void*)(uintptr_t)splineModels[i].icon.handle,
                            ImVec2(iconSize, iconSize), {1,1}, {0,0}))
                {
                    selectedSplineModel = i;
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                    ImGui::TextUnformatted(splineModels[i].model->name.c_str());
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
                ImGui::PopID();
            }
            else if (lastSplineModelRenderered == -1)
            {
                renderWorld.setName("Spline Model Icon");
                renderWorld.setSize(iconSize*2, iconSize*2);
                renderWorld.setClearColor(true, glm::vec4(0.15f, 0.15f, 0.15f, 1.f));
                Mesh* quadMesh = g_res.getMesh("world.Quad");
                renderWorld.push(LitRenderable(quadMesh,
                            glm::translate(glm::mat4(1.f), glm::vec3(0, 0, -0.01f)) *
                            glm::scale(glm::mat4(1.f), glm::vec3(120.f)), nullptr, glm::vec3(0.15f)));
                renderWorld.addDirectionalLight(glm::vec3(-0.5f, 0.2f, -1.f), glm::vec3(1.5f));
                renderWorld.setViewportCount(1);
                renderWorld.updateWorldTime(30.f);
                renderWorld.setViewportCamera(0, glm::vec3(7.f, 7.f, 7.f),
                        glm::vec3(0.f, 0.f, 1.f), 3.f, 150.f, 35.f);
                for (auto& obj : splineModels[i].model->objects)
                {
                    if (obj.isVisible)
                    {
                        renderWorld.push(LitMaterialRenderable(
                                    &splineModels[i].model->meshes[obj.meshIndex], obj.getTransform(),
                                    g_res.getMaterial(obj.materialGuid)));
                    }
                }
                g_game.renderer->addRenderWorld(&renderWorld);
                lastSplineModelRenderered = (i32)i;
            }
        }
    }

    void onEditorTabGui(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
        bool isKeyboardHandled = ImGui::GetIO().WantCaptureKeyboard;

        ImGui::Spacing();

        auto buttonSize = ImVec2(ImGui::GetWindowWidth() * 0.65f, 0);
        if (ImGui::Button("Connect Splines [c]", buttonSize)
                || (!isKeyboardHandled && g_input.isKeyPressed(KEY_C)))
        {
            connectSplines();
        }

        if (ImGui::Button("Subdivide [n]", buttonSize)
                || (!isKeyboardHandled && g_input.isKeyPressed(KEY_N)))
        {
            subdividePoints();
        }

        if (ImGui::Button("Split Spline [t]", buttonSize)
                || (!isKeyboardHandled && g_input.isKeyPressed(KEY_T)))
        {
            splitSpline(scene);
        }

        if (ImGui::Button("Delete Points [DELETE]", buttonSize)
                || (!isKeyboardHandled && g_input.isKeyPressed(KEY_T)))
        {
            deletePoints();
        }

        if (ImGui::Button("Match Highest Z", buttonSize))
        {
            matchZ(false);
        }

        if (ImGui::Button("Match Lowest Z", buttonSize))
        {
            matchZ(true);
        }

        ImGui::Gap();

        showSplineModelIcons();
    }

    void onSwitchTo(Scene* scene) override
    {
        transformGizmo.reset();
    }

    void onSwitchFrom(Scene* scene) override
    {
        transformGizmo.reset();
    }

    void gizmoDrag(glm::vec3 const& dragCenter, glm::vec3 const& dragDest, glm::vec3 const& dragDestZ,
            glm::vec3 dragOffset, i32 dragAxis) override
    {
        for (auto& selection : selectedPoints)
        {
            selection.spline->isDirty = true;

            if (dragAxis & DragAxis::X)
            {
                f32 diff = dragCenter.x - selection.spline->points[selection.pointIndex].position.x;
                selection.spline->points[selection.pointIndex].position.x = dragDest.x + dragOffset.x - diff;
            }

            if (dragAxis & DragAxis::Y)
            {
                f32 diff = dragCenter.y - selection.spline->points[selection.pointIndex].position.y;
                selection.spline->points[selection.pointIndex].position.y = dragDest.y + dragOffset.y - diff;
            }

            if (dragAxis & DragAxis::Z)
            {
                f32 diff = dragCenter.z - selection.spline->points[selection.pointIndex].position.z;
                selection.spline->points[selection.pointIndex].position.z = dragDestZ.z + dragOffset.z - diff;
            }
        }
        for (auto& selection : selectedHandles)
        {
            selection.spline->isDirty = true;

            glm::vec3 pointP = selection.spline->points[selection.pointIndex].position;
            glm::vec3& sourceP = selection.firstHandle ?
                selection.spline->points[selection.pointIndex].handleOffsetA :
                selection.spline->points[selection.pointIndex].handleOffsetB;
            glm::vec3 oppositeHandleP = selection.firstHandle ?
                selection.spline->points[selection.pointIndex].handleOffsetB :
                selection.spline->points[selection.pointIndex].handleOffsetA;
            glm::vec3 p = pointP + sourceP;

            if (dragAxis & DragAxis::X)
            {
                f32 diff = dragCenter.x - p.x;
                p.x = dragDest.x + dragOffset.x - diff;
            }

            if (dragAxis & DragAxis::Y)
            {
                f32 diff = dragCenter.y - p.y;
                p.y = dragDest.y + dragOffset.y - diff;
            }

            if (dragAxis & DragAxis::Z)
            {
                f32 diff = dragCenter.z - p.z;
                p.z = dragDestZ.z + dragOffset.z - diff;
            }

            sourceP = p - pointP;
            oppositeHandleP = -sourceP;
        }
    }

    void gizmoRotate(f32 angleDiff, glm::vec3 const& rotatePivot, i32 dragAxis) override
    {
        glm::vec3 rotationAxis(1, 0, 0);
        if (dragAxis & DragAxis::X)
        {
            rotationAxis = glm::vec3(1, 0, 0);
        }
        else if (dragAxis & DragAxis::Y)
        {
            rotationAxis = glm::vec3(0, 1, 0);
        }
        else if (dragAxis & DragAxis::Z)
        {
            rotationAxis = glm::vec3(0, 0, 1);
        }
        glm::mat4 transform = glm::rotate(glm::mat4(1.f), angleDiff, rotationAxis) *
            glm::translate(glm::mat4(1.f), -rotatePivot);
        for (auto& selection : selectedPoints)
        {
            selection.spline->isDirty = true;

            selection.spline->points[selection.pointIndex].position =
                glm::vec3(transform * glm::vec4(selection.spline->points[selection.pointIndex].position, 1.f)) + rotatePivot;
            // TODO: rotate attached handles
        }

        // TODO: rotate selected handles
    }

    void gizmoScale(f32 scaleFactor, i32 entityDragAxis, glm::vec3 const& scaleCenter,
            glm::vec3 const& dragOffset, glm::vec3 const& hitPos, glm::vec3& hitPosZ) override
    {
        // TODO: Scale along the axis chosen, not all of them
        if (selectedPoints.size() > 1)
        {
            glm::mat4 transform = glm::scale(glm::mat4(1.f), glm::vec3(scaleFactor))
                * glm::translate(glm::mat4(1.f), -scaleCenter);
            for (auto& selection : selectedPoints)
            {
                selection.spline->isDirty = true;

                glm::mat4 t = transform
                    * glm::translate(glm::mat4(1.f), selection.spline->points[selection.pointIndex].position);
                selection.spline->points[selection.pointIndex].position = translationOf(t) + scaleCenter;
                // TODO: scale attached handles
            }
        }

        // TODO: scale selected handles
    }
};
