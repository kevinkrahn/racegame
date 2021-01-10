#pragma once

#include "transform_gizmo.h"
#include "editor_mode.h"
#include "../input.h"
#include "../renderer.h"
#include "../imgui.h"
#include "../scene.h"
#include "../game.h"

class PathMode : public EditorMode, public TransformGizmoHandler
{
    Array<u16> selectedPoints;
    u32 selectedPathIndex = 0;
    i32 selectedGraphPathIndex = -1;
    TransformGizmo transformGizmo;
    Scene* scene = nullptr;

    void moveUp(Scene* scene)
    {
        if (selectedPathIndex <= 0)
        {
            return;
        }
        swap(scene->getPaths()[selectedPathIndex], scene->getPaths()[selectedPathIndex-1]);
    }

    void moveDown(Scene* scene)
    {
        if (selectedPathIndex >= scene->getPaths().size())
        {
            return;
        }
        swap(scene->getPaths()[selectedPathIndex], scene->getPaths()[selectedPathIndex+1]);
    }

    void subdividePath(Scene* scene)
    {
        if (selectedPoints.size() != 2)
        {
            return;
        }
        u32 pointIndexA = selectedPoints[0];
        u32 pointIndexB = selectedPoints[1];
        if (pointIndexB < pointIndexA)
        {
            swap(pointIndexA, pointIndexB);
        }
        auto& path = scene->getPaths()[selectedPathIndex];
        RacingLine::Point newPoint;
        newPoint.position =
            (path.points[pointIndexA].position + path.points[pointIndexB].position) * 0.5f;
        newPoint.targetSpeed =
            (path.points[pointIndexA].targetSpeed + path.points[pointIndexB].targetSpeed) * 0.5f;
        path.points.insert(path.points.begin() + pointIndexB, newPoint);
    }

    void deletePoints(Scene* scene)
    {
        if (scene->getPaths().empty())
        {
            return;
        }
        if ((i32)scene->getPaths()[selectedPathIndex].points.size() - (i32)selectedPoints.size() < 3)
        {
            return;
        }
        auto& path = scene->getPaths()[selectedPathIndex].points;
        for (u16 pointIndex : selectedPoints)
        {
            path[pointIndex].trackProgress = FLT_MAX;
        }
        for (auto it = path.begin(); it != path.end();)
        {
            if (it->trackProgress == FLT_MAX)
            {
                it = path.erase(it);
            }
            else
            {
                ++it;
            }
        }
        selectedPoints.clear();
    }

    void addNewPath(Scene* scene)
    {
        if (scene->getTrackGraph().getPaths().empty())
        {
            return;
        }
        auto& path = scene->getTrackGraph().getPaths()[selectedGraphPathIndex];
        RacingLine racingLine;
        racingLine.points.reserve(path.size());
        for (TrackGraph::Node* p : path)
        {
            RacingLine::Point point;
            point.position = p->position;
            racingLine.points.push(point);
        }
        scene->getPaths().push(move(racingLine));
    }

    void deletePath(Scene* scene)
    {
        if (scene->getPaths().size() > 0)
        {
            scene->getPaths().erase(scene->getPaths().begin() + selectedPathIndex);
            selectedPathIndex = 0;
            selectedPoints.clear();
        }
    }

    void duplicatePath(Scene* scene)
    {
        if (scene->getPaths().size() > 0)
        {
            scene->getPaths().push(scene->getPaths()[selectedPathIndex]);
            ++selectedPathIndex;
        }
    }

    void generatePaths(Scene* scene)
    {
        scene->getPaths().clear();
        scene->getPaths().reserve(scene->getTrackGraph().getPaths().size());
        for (auto& path : scene->getTrackGraph().getPaths())
        {
            RacingLine racingLine;
            racingLine.points.reserve(path.size());
            for (TrackGraph::Node* p : path)
            {
                RacingLine::Point point;
                point.position = p->position;
                racingLine.points.push(point);
            }
            scene->getPaths().push(move(racingLine));
        }
        selectedPoints.clear();
        selectedPathIndex = 0;
    }

public:
    PathMode() : EditorMode("Paths") {}

    void onUpdate(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
        this->scene = scene;
        if (scene->getPaths().size() > 0)
        {
            bool isMouseHandled = ImGui::GetIO().WantCaptureMouse;

            RacingLine& path = scene->getPaths()[selectedPathIndex];

            if (selectedPoints.size() > 0)
            {
                ImGui::Begin("Path Point");
                ImGui::Text("%i Point(s) Selected", selectedPoints.size());
                ImGui::Gap();
                ImGui::InputFloat3("Position", (f32*)&path.points[selectedPoints.back()].position);
                f32 speed = path.points[selectedPoints.back()].targetSpeed;
                ImGui::InputFloat("Speed", &speed, 0.1f, 0.f, "%.1f");
                speed = clamp(speed, 0.f, 1.f);
                for (auto& index : selectedPoints)
                {
                    path.points[index].targetSpeed = speed;
                }
                ImGui::End();

                Vec3 minP(FLT_MAX);
                Vec3 maxP(-FLT_MAX);
                for (auto& index : selectedPoints)
                {
                    minP = min(minP, path.points[index].position);
                    maxP = max(maxP, path.points[index].position);
                }
                Vec3 gizmoPosition = minP + (maxP - minP) * 0.5f;
                isMouseHandled = transformGizmo.update(gizmoPosition, scene, renderer->getRenderWorld(),
                        deltaTime, Mat4(1.f), this, false);
            }

            Vec3 offset(0, 0, 0.05f);
            Vec4 color(1, 1, 0, 1);
            Mesh* sphere = g_res.getModel("misc")->getMeshByName("Sphere");
            Vec3 rayDir = scene->getEditorCamera().getMouseRay(renderer->getRenderWorld());
            Camera const& cam = scene->getEditorCamera().getCamera();
            for (u32 i=0; i<path.points.size()-1; ++i)
            {
                scene->debugDraw.line(path.points[i].position + offset,
                                path.points[i+1].position + offset, color, color);
            }
            scene->debugDraw.line(path.points.back().position + offset,
                                path.points.front().position + offset, color, color);

            RenderWorld* rw = renderer->getRenderWorld();
            for (u16 si : selectedPoints)
            {
                drawOverlay(rw, sphere,
                            Mat4::translation(path.points[si].position) *
                            Mat4::scaling(Vec3(1.08f)), { 1, 1, 1 }, -2);
            }
            for (u32 i=0; i<path.points.size(); ++i)
            {
                drawOverlay(rw, sphere,
                            Mat4::translation(path.points[i].position) *
                            Mat4::scaling(Vec3(1.f)), { 1, 0, 0 }, -2);
            }

            if (!isMouseHandled && g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                if (!g_input.isKeyDown(KEY_LCTRL) && !g_input.isKeyDown(KEY_LSHIFT))
                {
                    selectedPoints.clear();
                }
                bool hit = false;
                for (u32 i=0; i<path.points.size(); ++i)
                {
                    f32 dist = length(cam.position - path.points[i].position);
                    f32 fixedSize = 0.02f;
                    f32 size = dist * fixedSize * radians(cam.fov);
                    if (!isMouseHandled &&
                        raySphereIntersection(cam.position, rayDir, path.points[i].position, size) > 0.f)
                    {
                        bool alreadySelected = false;
                        for (u32 j=0; j<selectedPoints.size(); ++j)
                        {
                            if ((u32)selectedPoints[j] == i)
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
                            selectedPoints.push((u16)i);
                        }
                        hit = true;
                        break;
                    }
                }
                if (!hit)
                {
                    selectedPoints.clear();
                }
            }
        }
    }

    void onEditorTabGui(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
        ImGui::Spacing();

        if (ImGui::ListBoxHeader("AI Paths"))
        {
            for (u32 i=0; i<scene->getPaths().size(); ++i)
            {
                if (ImGui::Selectable(tmpStr("Path %u", i + 1), i == selectedPathIndex))
                {
                    selectedPathIndex = i;
                    selectedPoints.clear();
                }
            }
            ImGui::ListBoxFooter();
            ImGui::HelpMarker("Paths that the AI drivers will follow. The most optimal paths should be first in the list.");
        }

        auto buttonSize = ImVec2(ImGui::GetWindowWidth() * 0.65f, 0);

        if (ImGui::Button("Move Up", { buttonSize.x / 2 - 4, 0 }))
        {
            moveUp(scene);
        }

        ImGui::SameLine();
        if (ImGui::Button("Move Down", { buttonSize.x / 2 - 4, 0 }))
        {
            moveUp(scene);
        }

        if (ImGui::BeginCombo("Source Path",
                selectedGraphPathIndex >= 0 ? tmpStr("Graph Path %i", selectedGraphPathIndex + 1) : "None"))
        {
            for (i32 i=0; i<(i32)scene->getTrackGraph().getPaths().size(); ++i)
            {
                if (ImGui::Selectable(tmpStr("Graph Path %i", i + 1)))
                {
                    selectedGraphPathIndex = i;
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::Button("New Path From Graph", buttonSize))
        {
            addNewPath(scene);
        }

        if (ImGui::Button("Delete Path", buttonSize))
        {
            deletePath(scene);
        }

        if (ImGui::Button("Duplicate Path", buttonSize))
        {
            duplicatePath(scene);
        }

        if (ImGui::Button("Generate Paths", buttonSize))
        {
            if (scene->getPaths().size() > 0)
            {
                ImGui::OpenPopup("Confirm");
            }
            else
            {
                generatePaths(scene);
            }
        }

        if (ImGui::BeginPopupModal("Confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("This will delete all existing paths. Do you want to continue?");
            ImGui::Gap();
            if (ImGui::Button("Yes", {120, 0}))
            {
                ImGui::CloseCurrentPopup();
                generatePaths(scene);
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", {120, 0}))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Gap();

        if (ImGui::Button("Subdivide [n]", buttonSize) || g_input.isKeyPressed(KEY_N))
        {
            subdividePath(scene);
        }

        if (ImGui::Button("Delete Point(s) [DELETE]", buttonSize) || g_input.isKeyPressed(KEY_DELETE))
        {
            deletePoints(scene);
        }
    }

    void onSwitchTo(Scene* scene) override
    {
        scene->track->buildTrackGraph(&scene->getTrackGraph(), scene->getStart());
        if (scene->getTrackGraph().getPaths().size() > 0)
        {
            selectedGraphPathIndex = 0;
        }
        transformGizmo.reset();
    }

    void onSwitchFrom(Scene* scene) override
    {
        transformGizmo.reset();
    }

    void gizmoDrag(Vec3 const& dragCenter, Vec3 const& dragDest, Vec3 const& dragDestZ,
            Vec3 dragOffset, i32 dragAxis) override
    {
        auto& path = scene->getPaths()[selectedPathIndex];
        for (u16 pointIndex : selectedPoints)
        {
            if (dragAxis & DragAxis::X)
            {
                f32 diff = dragCenter.x - path.points[pointIndex].position.x;
                path.points[pointIndex].position.x = dragDest.x + dragOffset.x - diff;
            }

            if (dragAxis & DragAxis::Y)
            {
                f32 diff = dragCenter.y - path.points[pointIndex].position.y;
                path.points[pointIndex].position.y = dragDest.y + dragOffset.y - diff;
            }

            if (dragAxis & DragAxis::Z)
            {
                f32 diff = dragCenter.z - path.points[pointIndex].position.z;
                path.points[pointIndex].position.z = dragDestZ.z + dragOffset.z - diff;
            }
        }
    }

    void gizmoRotate(f32 angleDiff, Vec3 const& rotatePivot, i32 dragAxis) override
    {
        auto& path = scene->getPaths()[selectedPathIndex];
        Mat4 rot;
        if (dragAxis & DragAxis::X)
        {
            rot = Mat4::rotationX(angleDiff);
        }
        else if (dragAxis & DragAxis::Y)
        {
            rot = Mat4::rotationY(angleDiff);
        }
        else if (dragAxis & DragAxis::Z)
        {
            rot = Mat4::rotationZ(angleDiff);
        }
        Mat4 transform = rot * Mat4::translation(-rotatePivot);
        for (u16 pointIndex : selectedPoints)
        {
            path.points[pointIndex].position =
                Vec3(transform * Vec4(path.points[pointIndex].position, 1.f)) + rotatePivot;
        }
    }

    void gizmoScale(f32 scaleFactor, i32 entityDragAxis, Vec3 const& scaleCenter,
            Vec3 const& dragOffset, Vec3 const& hitPos, Vec3& hitPosZ) override
    {
        // TODO: Scale along the axis chosen, not all of them
        if (selectedPoints.size() > 1)
        {
            auto& path = scene->getPaths()[selectedPathIndex];
            Mat4 transform = Mat4::scaling(Vec3(scaleFactor)) * Mat4::translation(-scaleCenter);
            for (u16 pointIndex : selectedPoints)
            {
                Mat4 t = transform * Mat4::translation(path.points[pointIndex].position);
                path.points[pointIndex].position = t.position() + scaleCenter;
            }
        }
    }
};
