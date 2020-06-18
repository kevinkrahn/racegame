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

    Array<SplineMeshInfo> splineModels;
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

    Array<PointSelection> selectedPoints;
    Array<HandleSelection> selectedHandles;
    Spline* selectedSpline = nullptr;
    TransformGizmo transformGizmo;

    Array<Spline*> splineEntities;

    void clearSelection()
    {
        selectedPoints.clear();
        selectedHandles.clear();
    }

    bool raySphereIntersection(Camera const& cam, Vec3 const& rayDir,
            Vec3 const& spherePosition, f32 fixedSize)
    {
        f32 dist = length(cam.position - spherePosition);
        f32 size = dist * fixedSize * radians(cam.fov);
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
        if (splineA == splineB)
        {
            return;
        }

        if (pointIndexA == splineA->points.size() - 1 &&
            pointIndexB == splineB->points.size() - 1)
        {
            for (i32 i=(i32)splineB->points.size() - 1; i >= 0; --i)
            {
                SplinePoint point = splineB->points[i];
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
            for (i32 i=(i32)splineB->points.size() - 1; i >= 0; --i)
            {
                splineA->points.insert(splineA->points.begin(), splineB->points[i]);
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

        if (absolute((i32)selectedPoints[0].pointIndex - (i32)selectedPoints[1].pointIndex) != 1)
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
        Vec3 midPoint = pointOnBezierCurve(p1.position, p1.position + p1.handleOffsetB,
                p2.position + p2.handleOffsetA, p2.position, 0.5f);
        Vec3 handleA = pointOnBezierCurve(p1.position, p1.position + p1.handleOffsetB,
                p2.position + p2.handleOffsetA, p2.position, 0.4f) - midPoint;
        Vec3 handleB = pointOnBezierCurve(p1.position, p1.position + p1.handleOffsetB,
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

        newSpline->points = Array<SplinePoint>(
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
        g_res.iterateResourceType(ResourceType::MODEL, [&](Resource* res){
            Model* model = (Model*)res;
            if (model->modelUsage == ModelUsage::SPLINE)
            {
                splineModels.push_back({ model });
            }
        });
    }

    void onUpdate(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
        bool isMouseClickHandled = ImGui::GetIO().WantCaptureMouse;
        bool isKeyboardHandled = ImGui::GetIO().WantCaptureKeyboard;

        RenderWorld* rw = renderer->getRenderWorld();
        Vec3 rayDir = scene->getEditorCamera().getMouseRay(rw);
        Camera const& cam = scene->getEditorCamera().getCamera();

        if (!selectedPoints.empty() || !selectedHandles.empty())
        {
            Vec3 minP(FLT_MAX);
            Vec3 maxP(-FLT_MAX);
            for (auto& selection : selectedPoints)
            {
                minP = min(minP, selection.spline->points[selection.pointIndex].position);
                maxP = max(maxP, selection.spline->points[selection.pointIndex].position);
            }
            for (auto& selection : selectedHandles)
            {
                if (selection.firstHandle)
                {
                    minP = min(minP, selection.spline->points[selection.pointIndex].position
                            + selection.spline->points[selection.pointIndex].handleOffsetA);
                    maxP = max(maxP, selection.spline->points[selection.pointIndex].position
                            + selection.spline->points[selection.pointIndex].handleOffsetA);
                }
                else
                {
                    minP = min(minP, selection.spline->points[selection.pointIndex].position
                            + selection.spline->points[selection.pointIndex].handleOffsetB);
                    maxP = max(maxP, selection.spline->points[selection.pointIndex].position
                            + selection.spline->points[selection.pointIndex].handleOffsetB);
                }
            }
            Vec3 gizmoPosition = minP + (maxP - minP) * 0.5f;
            if (transformGizmo.update(gizmoPosition, scene, rw,
                    deltaTime, Mat4(1.f), this, false))
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
                Vec3 hitPoint = convert(hit.block.position);
                Spline* newSpline = (Spline*)g_entities[5].create();
                newSpline->modelGuid = splineModels[selectedSplineModel].model->guid;
                newSpline->setPersistent(true);
                Vec3 handleOffset(5, 0, 0);
                newSpline->points.push_back(
                        { hitPoint, -handleOffset, handleOffset });
                newSpline->points.push_back(
                        { hitPoint + Vec3(20, 0, 0), -handleOffset, handleOffset });
                newSpline->isDirty = true;
                scene->addEntity(newSpline);
            }
        }

        // add point to spline
        if (!isKeyboardHandled && selectedPoints.size() == 1 && g_input.isKeyPressed(KEY_E))
        {
            isMouseClickHandled = true;
            PxRaycastBuffer hit;
            if (scene->raycastStatic(cam.position, rayDir, 10000.f, &hit))
            {
                Vec3 hitPoint = convert(hit.block.position);
                Spline* spline = selectedPoints.front().spline;
                auto& point = spline->points[selectedPoints.front().pointIndex];
                Vec3 handleP = Vec3(normalize(
                            Vec2(hitPoint) - Vec2(point.position)) * 4.f, 0.f);
                u32 newSelectIndex = 0;
                if (selectedPoints.front().pointIndex == spline->points.size() - 1)
                {
                    newSelectIndex = (u32)spline->points.size();
                    spline->points.push_back({ hitPoint, -handleP, handleP });
                }
                else
                {
                    spline->points.insert(
                            selectedPoints.front().spline->points.begin(), { hitPoint, handleP, -handleP });
                }
                selectedPoints.clear();
                selectedPoints.push_back({ spline, newSelectIndex });
                spline->isDirty = true;
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
            selectedSpline->onRenderOutline(rw, scene, deltaTime);
            showSelectedSplineProperties();
        }

        Mesh* sphereMesh = g_res.getModel("misc")->getMeshByName("world.Sphere");
        for (auto spline : splineEntities)
        {
            Vec4 orange = { 1.f, 0.5f, 0.f, 1.f };
            Vec4 red = { 1.f, 0.f, 0.f, 1.f };
            Vec4 white = { 1.f, 1.f, 1.f, 1.f };
            for (u32 i=0; i<spline->points.size(); ++i)
            {
                auto& point = spline->points[i];

                Mat4 transform = glm::translate(Mat4(1.f), point.position);
                bool isSelected = !!selectedPoints.find([&](auto& p) {
                    return p.spline == spline && p.pointIndex == i; });
                Vec3 color = isSelected ? white : red;
                drawOverlay(rw, sphereMesh, transform, color);

                isSelected = !!selectedHandles.find([&](auto& p) {
                    return p.spline == spline && p.pointIndex == i && p.firstHandle; });
                color = isSelected ? white : orange;
                transform = glm::translate(Mat4(1.f), point.position + point.handleOffsetA) *
                            glm::scale(Mat4(1.f), Vec3(0.8f));
                drawOverlay(rw, sphereMesh, transform, color);

                isSelected = !!selectedHandles.find([&](auto& p) {
                    return p.spline == spline && p.pointIndex == i && !p.firstHandle; });
                color = isSelected ? white : orange;
                transform = glm::translate(Mat4(1.f), point.position + point.handleOffsetB) *
                            glm::scale(Mat4(1.f), Vec3(0.8f));
                drawOverlay(rw, sphereMesh, transform, color);

                Vec4 c(color, 1.f);
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

            if (ImGui::DragFloat("Scale", &selectedSpline->scale, 0.01f, 0.1f, 10.f))
            {
                selectedSpline->isDirty = true;
                selectedSpline->scale = max(selectedSpline->scale, 0.01f);
            }

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
                bool isButtonSelected = selectedSplineModel == i;
                if (isButtonSelected)
                {
                    const u32 selectedColor = 0x992299EE;
                    ImGui::PushStyleColor(ImGuiCol_Button, selectedColor);
                }
                if (ImGui::ImageButton((void*)(uintptr_t)splineModels[i].icon.handle,
                            ImVec2(iconSize, iconSize), {1,1}, {0,0}))
                {
                    selectedSplineModel = i;
                }
                if (isButtonSelected)
                {
                    ImGui::PopStyleColor();
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
                renderWorld.setClearColor(true, Vec4(0.15f, 0.15f, 0.15f, 1.f));
                Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("world.Quad");
                drawSimple(&renderWorld, quadMesh, &g_res.white,
                            glm::translate(Mat4(1.f), Vec3(0, 0, -0.01f)) *
                            glm::scale(Mat4(1.f), Vec3(120.f)), Vec3(0.15f));
                renderWorld.addDirectionalLight(Vec3(-0.5f, 0.2f, -1.f), Vec3(1.5f));
                renderWorld.setViewportCount(1);
                renderWorld.updateWorldTime(30.f);
                renderWorld.setViewportCamera(0, Vec3(7.f, 7.f, 7.f),
                        Vec3(0.f, 0.f, 1.f), 3.f, 150.f, 35.f);
                for (auto& obj : splineModels[i].model->objects)
                {
                    if (obj.isVisible)
                    {
                        g_res.getMaterial(obj.materialGuid)->draw(&renderWorld, obj.getTransform(),
                                &splineModels[i].model->meshes[obj.meshIndex]);
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

    void gizmoDrag(Vec3 const& dragCenter, Vec3 const& dragDest, Vec3 const& dragDestZ,
            Vec3 dragOffset, i32 dragAxis) override
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

            Vec3 pointP = selection.spline->points[selection.pointIndex].position;
            Vec3& sourceP = selection.firstHandle ?
                selection.spline->points[selection.pointIndex].handleOffsetA :
                selection.spline->points[selection.pointIndex].handleOffsetB;
            Vec3& oppositeHandleP = selection.firstHandle ?
                selection.spline->points[selection.pointIndex].handleOffsetB :
                selection.spline->points[selection.pointIndex].handleOffsetA;
            Vec3 p = pointP + sourceP;

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

    void gizmoRotate(f32 angleDiff, Vec3 const& rotatePivot, i32 dragAxis) override
    {
        Vec3 rotationAxis(1, 0, 0);
        if (dragAxis & DragAxis::X)
        {
            rotationAxis = Vec3(1, 0, 0);
        }
        else if (dragAxis & DragAxis::Y)
        {
            rotationAxis = Vec3(0, 1, 0);
        }
        else if (dragAxis & DragAxis::Z)
        {
            rotationAxis = Vec3(0, 0, 1);
        }
        Mat4 transform = glm::rotate(Mat4(1.f), angleDiff, rotationAxis) *
            glm::translate(Mat4(1.f), -rotatePivot);
        for (auto& selection : selectedPoints)
        {
            selection.spline->isDirty = true;

            selection.spline->points[selection.pointIndex].position =
                Vec3(transform * Vec4(selection.spline->points[selection.pointIndex].position, 1.f)) + rotatePivot;
            // TODO: rotate attached handles
        }

        // TODO: rotate selected handles
    }

    void gizmoScale(f32 scaleFactor, i32 entityDragAxis, Vec3 const& scaleCenter,
            Vec3 const& dragOffset, Vec3 const& hitPos, Vec3& hitPosZ) override
    {
        // TODO: Scale along the axis chosen, not all of them
        if (selectedPoints.size() > 1)
        {
            Mat4 transform = glm::scale(Mat4(1.f), Vec3(scaleFactor))
                * glm::translate(Mat4(1.f), -scaleCenter);
            for (auto& selection : selectedPoints)
            {
                selection.spline->isDirty = true;

                Mat4 t = transform
                    * glm::translate(Mat4(1.f), selection.spline->points[selection.pointIndex].position);
                selection.spline->points[selection.pointIndex].position = translationOf(t) + scaleCenter;
                // TODO: scale attached handles
            }
        }

        // TODO: scale selected handles
    }
};
