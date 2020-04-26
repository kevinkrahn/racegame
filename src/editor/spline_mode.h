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

public:
    SplineMode() : EditorMode("Splines") {}

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
            isMouseClickHandled = transformGizmo.update(gizmoPosition, scene, rw,
                    deltaTime, glm::mat4(1.f), this, false);
        }

        splineEntities.clear();
        for (auto& e : scene->getEntities())
        {
            if (e->entityID == 5)
            {
                splineEntities.push_back((Spline*)e.get());
            }
        }

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
                    if (raySphereIntersection(cam, rayDir, spline->points[i].position, 0.02f))
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
                            spline->points[i].position + spline->points[i].handleOffsetA, 0.0125f))
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
                            spline->points[i].position + spline->points[i].handleOffsetB, 0.0125f))
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

    void onEditorTabGui(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
        bool isKeyboardHandled = ImGui::GetIO().WantCaptureKeyboard;

        ImGui::Spacing();

        auto buttonSize = ImVec2(ImGui::GetWindowWidth() * 0.65f, 0);
        if (ImGui::Button("Connect Splines [c]", buttonSize)
                || (!isKeyboardHandled && g_input.isKeyPressed(KEY_C)))
        {
        }

        if (ImGui::Button("Subdivide [n]", buttonSize)
                || (!isKeyboardHandled && g_input.isKeyPressed(KEY_N)))
        {
        }

        if (ImGui::Button("Split [t]", buttonSize)
                || (!isKeyboardHandled && g_input.isKeyPressed(KEY_T)))
        {
        }

        if (ImGui::Button("Match Highest Z", buttonSize))
        {
        }

        if (ImGui::Button("Match Lowest Z", buttonSize))
        {
        }
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
