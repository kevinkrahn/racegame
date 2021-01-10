#pragma once

#include "../math.h"
#include "../renderer.h"
#include "../input.h"
#include "../game.h"
#include "../imgui.h"
#include "../scene.h"
#include "editor_camera.h"

class TransformGizmoHandler
{
public:
    virtual ~TransformGizmoHandler() {}
    virtual void gizmoDrag(Vec3 const& dragCenter, Vec3 const& dragDest,
            Vec3 const& dragDestZ, Vec3 dragOffset, i32 dragAxis) = 0;
    virtual void gizmoRotate(f32 angleDiff, Vec3 const& rotatePivot, i32 dragAxis) = 0;
    virtual void gizmoScale(f32 scaleFactor, i32 dragAxis, Vec3 const& scaleCenter,
            Vec3 const& dragOffset, Vec3 const& hitPos, Vec3& hitPosZ) = 0;
};

namespace DragAxis
{
    enum
    {
        NONE = 0,
        X = 1 << 0,
        Y = 1 << 1,
        Z = 1 << 2,
        MIDDLE = X | Y,
    };
}

enum struct TransformMode : i32
{
    TRANSLATE,
    ROTATE,
    SCALE,
    MAX
};

class TransformGizmo
{
    TransformMode transformMode = TransformMode::TRANSLATE;
    i32 entityDragAxis = DragAxis::NONE;
    Vec3 entityDragOffset;
    Vec3 rotatePivot;

public:
    TransformMode getTransformMode() const { return transformMode; }
    void setTransformMode(TransformMode mode) { transformMode = mode; }

    void reset()
    {
        entityDragAxis = DragAxis::NONE;
    }

    bool update(Vec3 const& p, Scene* scene, RenderWorld* rw, f32 deltaTime,
            Mat4 const& orientation, TransformGizmoHandler* handler, bool drawCenter=true)
    {
        bool isMouseClickHandled = ImGui::GetIO().WantCaptureMouse;
        bool isKeyboardHandled = ImGui::GetIO().WantCaptureKeyboard;

        f32 rot = (f32)M_PI * 0.5f;
        Vec2 windowSize(g_game.windowWidth, g_game.windowHeight);
        Mat4 viewProj = rw->getCamera(0).viewProjection;

        Vec3 rayDir = scene->getEditorCamera().getMouseRay(rw);
        Camera const& cam = scene->getEditorCamera().getCamera();

        if (g_input.isKeyPressed(KEY_ESCAPE))
        {
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

        if (!isKeyboardHandled && g_input.isKeyPressed(KEY_SPACE))
        {
            transformMode = (TransformMode)(((u32)transformMode + 1) % (u32)TransformMode::MAX);
        }

        if (!isKeyboardHandled && g_input.isKeyPressed(KEY_G))
        {
            transformMode = TransformMode::TRANSLATE;
            entityDragAxis = DragAxis::MIDDLE;
        }

        if (!isKeyboardHandled && g_input.isKeyPressed(KEY_R))
        {
            transformMode = TransformMode::ROTATE;
            entityDragAxis = DragAxis::Z;
            rotatePivot = p;
            Vec2 mousePos = g_input.getMousePosition();
            entityDragOffset.x = pointDirection(mousePos, project(rotatePivot, viewProj) * windowSize);
        }

        if (!isKeyboardHandled && g_input.isKeyPressed(KEY_F))
        {
            transformMode = TransformMode::SCALE;
            entityDragAxis = DragAxis::MIDDLE;
        }

        Vec3 xCol = Vec3(0.95f, 0, 0);
        Vec3 xColHighlight = Vec3(1, 0.1f, 0.1f);
        Vec3 yCol = Vec3(0, 0.85f, 0);
        Vec3 yColHighlight = Vec3(0.2f, 1, 0.2f);
        Vec3 zCol = Vec3(0, 0, 0.95f);
        Vec3 zColHighlight = Vec3(0.1f, 0.1f, 1.f);
        Vec3 centerCol = Vec3(0.8f, 0.8f, 0.8f);

        if (transformMode == TransformMode::TRANSLATE)
        {
            f32 xyT = rayPlaneIntersection(cam.position, rayDir, Vec3(0, 0, 1), p);
            Vec3 hitPos = cam.position + rayDir * xyT;
            f32 zT = rayPlaneIntersection(cam.position, rayDir,
                    normalize(Vec3(Vec2(-rayDir), 0.f)), p);
            Vec3 hitPosZ = cam.position + rayDir * zT;

            if (!isMouseClickHandled)
            {
                f32 offset = 4.5f;
                Vec2 size(g_game.windowWidth, g_game.windowHeight);
                Vec2 xHandle = projectScale(p, Vec3(offset, 0, 0), viewProj) * size;
                Vec2 yHandle = projectScale(p, Vec3(0, offset, 0), viewProj) * size;
                Vec2 zHandle = projectScale(p, Vec3(0, 0, offset), viewProj) * size;
                Vec2 center = project(p, viewProj) * size;

                f32 radius = 18.f;
                Vec2 mousePos = g_input.getMousePosition();

                if (length(xHandle - mousePos) < radius)
                {
                    xCol = xColHighlight;
                    if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                    {
                        entityDragAxis = DragAxis::X;
                        isMouseClickHandled = true;
                        entityDragOffset = p - hitPos;
                    }
                }

                if (length(yHandle - mousePos) < radius)
                {
                    yCol = yColHighlight;
                    if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                    {
                        entityDragAxis = DragAxis::Y;
                        isMouseClickHandled = true;
                        entityDragOffset = p - hitPos;
                    }
                }

                if (length(zHandle - mousePos) < radius)
                {
                    zCol = zColHighlight;
                    if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                    {
                        entityDragAxis = DragAxis::Z;
                        isMouseClickHandled = true;
                        entityDragOffset = p - hitPosZ;
                    }
                }

                bool shortcut = !isKeyboardHandled && g_input.isKeyPressed(KEY_G);
                if (length(center - mousePos) < radius || shortcut)
                {
                    centerCol = Vec3(1.f);
                    if (g_input.isMouseButtonPressed(MOUSE_LEFT) || shortcut)
                    {
                        entityDragAxis = DragAxis::MIDDLE;
                        isMouseClickHandled = true;
                        entityDragOffset = p - hitPos;
                    }
                }
            }

            if (entityDragAxis)
            {
                bool valid = false;

                if (entityDragAxis & DragAxis::X)
                {
                    xCol = xColHighlight;
                    if (xyT < 300.f)
                    {
                        valid = true;
                    }
                }

                if (entityDragAxis & DragAxis::Y)
                {
                    yCol = yColHighlight;
                    if (xyT < 300.f)
                    {
                        valid = true;
                    }
                }

                if (entityDragAxis & DragAxis::Z)
                {
                    zCol = zColHighlight;
                    if (zT < 300.f)
                    {
                        valid = true;
                    }
                }

                if (valid)
                {
                    handler->gizmoDrag(p, hitPos, hitPosZ, entityDragOffset, entityDragAxis);
                }
            }

            Mesh* arrowMesh = g_res.getModel("misc")->getMeshByName("TranslateArrow");
            Mesh* centerMesh = g_res.getModel("misc")->getMeshByName("Sphere");
            if (drawCenter)
            {
                drawOverlay(rw, centerMesh, Mat4::translation(p), centerCol, -1);
            }

            drawOverlay(rw, arrowMesh, Mat4::translation(p), xCol);
            if (entityDragAxis & DragAxis::X)
            {
                scene->debugDraw.line(
                        p - Vec3(100.f, 0.f, 0.f), p + Vec3(100.f, 0.f, 0.f),
                        Vec4(1, 0, 0, 1), Vec4(1, 0, 0, 1));
            }

            drawOverlay(rw, arrowMesh, Mat4::translation(p) * Mat4::rotationZ(rot), yCol);
            if (entityDragAxis & DragAxis::Y)
            {
                scene->debugDraw.line(
                        p - Vec3(0.f, 100.f, 0.f), p + Vec3(0.f, 100.f, 0.f),
                        Vec4(0, 1, 0, 1), Vec4(0, 1, 0, 1));
            }

            drawOverlay(rw, arrowMesh, Mat4::translation(p) * Mat4::rotationY(rot), zCol);
            if (entityDragAxis & DragAxis::Z)
            {
                scene->debugDraw.line(
                        p - Vec3(0.f, 0.f, 100.f), p + Vec3(0.f, 0.f, 100.f),
                        Vec4(0, 0, 1, 1), Vec4(0, 0, 1, 1));
            }
        }
        if (transformMode == TransformMode::ROTATE)
        {
            f32 dist = length(cam.position - p);
            f32 fixedSize = 0.085f;
            f32 size = dist * fixedSize * radians(cam.fov);
            Vec2 mousePos = g_input.getMousePosition();

            f32 sphereT = raySphereIntersection(cam.position, rayDir, p, size);
            if (sphereT > 0.f)
            {
                Vec2 centerPos = project(p, viewProj) * windowSize;
                Vec3 sphereHitPos = cam.position + rayDir * sphereT;
                Vec3 localHitPos = normalize(sphereHitPos - p);

                if (!isMouseClickHandled)
                {
                    if (absolute(localHitPos.x) < 0.2f)
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
                    else if (absolute(localHitPos.y) < 0.2f)
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
                    else if (absolute(localHitPos.z) < 0.2f)
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
                Vec2 centerPos = project(rotatePivot, viewProj) * windowSize;
                f32 angle = pointDirection(mousePos, centerPos);
                f32 angleDiff = angleDifference(angle, entityDragOffset.x);
                entityDragOffset.x = angle;
                handler->gizmoRotate(angleDiff, rotatePivot, entityDragAxis);
            }
            else
            {
                Mesh* arrowMesh = g_res.getModel("misc")->getMeshByName("RotateArrow");
                Mesh* sphereMesh = g_res.getModel("misc")->getMeshByName("Sphere");
                drawOverlay(rw, sphereMesh,
                        Mat4::translation(p) * Mat4::scaling(Vec3(4.4f)),
                        Vec3(1.f), -1, true);

                drawOverlay(rw, arrowMesh, Mat4::translation(p) * Mat4::rotationY(rot), xCol);
                if (entityDragAxis & DragAxis::X)
                {
                    scene->debugDraw.line(
                            p - Vec3(100.f, 0.f, 0.f), p + Vec3(100.f, 0.f, 0.f),
                            Vec4(1, 0, 0, 1), Vec4(1, 0, 0, 1));
                }

                drawOverlay(rw, arrowMesh, Mat4::translation(p) * Mat4::rotationX(rot), yCol);
                if (entityDragAxis & DragAxis::Y)
                {
                    scene->debugDraw.line(
                            p - Vec3(0.f, 100.f, 0.f), p + Vec3(0.f, 100.f, 0.f),
                            Vec4(0, 1, 0, 1), Vec4(0, 1, 0, 1));
                }

                drawOverlay(rw, arrowMesh, Mat4::translation(p), zCol);
                if (entityDragAxis & DragAxis::Z)
                {
                    scene->debugDraw.line(
                            p - Vec3(0.f, 0.f, 100.f), p + Vec3(0.f, 0.f, 100.f),
                            Vec4(0, 0, 1, 1), Vec4(0, 0, 1, 1));
                }
            }
        }
        if (transformMode == TransformMode::SCALE)
        {
            f32 t = rayPlaneIntersection(cam.position, rayDir, Vec3(0, 0, 1), p);
            Vec3 hitPos = cam.position + rayDir * t;
            t = rayPlaneIntersection(cam.position, rayDir,
                    normalize(Vec3(Vec2(-rayDir), 0.f)), p);
            Vec3 hitPosZ = cam.position + rayDir * t;

            f32 offset = 4.5f;
            Vec3 xAxis = orientation.xAxis() * offset;
            Vec3 yAxis = orientation.yAxis() * offset;
            Vec3 zAxis = orientation.zAxis() * offset;

            if (!isMouseClickHandled)
            {
                Vec2 size(g_game.windowWidth, g_game.windowHeight);
                Vec2 xHandle = projectScale(p, xAxis, viewProj) * size;
                Vec2 yHandle = projectScale(p, yAxis, viewProj) * size;
                Vec2 zHandle = projectScale(p, zAxis, viewProj) * size;
                Vec2 center = project(p, viewProj) * size;

                f32 radius = 18.f;
                Vec2 mousePos = g_input.getMousePosition();

                if (length(xHandle - mousePos) < radius)
                {
                    xCol = xColHighlight;
                    if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                    {
                        entityDragAxis = DragAxis::X;
                        isMouseClickHandled = true;
                        entityDragOffset = p - hitPos;
                    }
                }

                if (length(yHandle - mousePos) < radius)
                {
                    yCol = yColHighlight;
                    if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                    {
                        entityDragAxis = DragAxis::Y;
                        isMouseClickHandled = true;
                        entityDragOffset = p - hitPos;
                    }
                }

                if (length(zHandle - mousePos) < radius)
                {
                    zCol = zColHighlight;
                    if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                    {
                        entityDragAxis = DragAxis::Z;
                        isMouseClickHandled = true;
                        entityDragOffset = p - hitPosZ;
                    }
                }

                bool shortcut = !isKeyboardHandled && g_input.isKeyPressed(KEY_F);
                if (length(center - mousePos) < radius || shortcut)
                {
                    centerCol = Vec3(1.f);
                    if (g_input.isMouseButtonPressed(MOUSE_LEFT) || shortcut)
                    {
                        entityDragAxis = DragAxis::MIDDLE;
                        isMouseClickHandled = true;
                        entityDragOffset = p - hitPos;
                    }
                }
            }

            if (entityDragAxis)
            {
                f32 startDistance = length(entityDragOffset);
                f32 scaleFactor = length(p - hitPos) / startDistance;
                handler->gizmoScale(scaleFactor, entityDragAxis, p, entityDragOffset, hitPos, hitPosZ);
                if ((entityDragAxis & DragAxis::MIDDLE) == DragAxis::MIDDLE)
                {
                    xCol = xColHighlight;
                    yCol = yColHighlight;
                    zCol = zColHighlight;
                    entityDragOffset = p - hitPos;
                }
                else if (entityDragAxis & DragAxis::X)
                {
                    xCol = xColHighlight;
                    entityDragOffset = p - hitPos;
                }
                else if (entityDragAxis & DragAxis::Y)
                {
                    yCol = yColHighlight;
                    entityDragOffset = p - hitPos;
                }
                else if (entityDragAxis & DragAxis::Z)
                {
                    zCol = zColHighlight;
                    entityDragOffset = p - hitPosZ;
                }
            }

            Mesh* arrowMesh = g_res.getModel("misc")->getMeshByName("ScaleArrow");
            Mesh* centerMesh = g_res.getModel("misc")->getMeshByName("UnitCube");
            if (drawCenter)
            {
                drawOverlay(rw, centerMesh, Mat4::translation(p) * orientation, centerCol, -1);
            }

            drawOverlay(rw, arrowMesh, Mat4::translation(p) * orientation, xCol);
            if (entityDragAxis & DragAxis::X)
            {
                scene->debugDraw.line(
                        p - Vec3(100.f, 0.f, 0.f), p + Vec3(100.f, 0.f, 0.f),
                        Vec4(1, 0, 0, 1), Vec4(1, 0, 0, 1));
            }

            drawOverlay(rw, arrowMesh, Mat4::translation(p) * orientation * Mat4::rotationZ(rot), yCol);
            if (entityDragAxis & DragAxis::Y)
            {
                scene->debugDraw.line(
                        p - Vec3(0.f, 100.f, 0.f), p + Vec3(0.f, 100.f, 0.f),
                        Vec4(0, 1, 0, 1), Vec4(0, 1, 0, 1));
            }

            drawOverlay(rw, arrowMesh, Mat4::translation(p) * orientation * Mat4::rotationY(rot), zCol);
            if (entityDragAxis & DragAxis::Z)
            {
                scene->debugDraw.line(
                        p - Vec3(0.f, 0.f, 100.f), p + Vec3(0.f, 0.f, 100.f),
                        Vec4(0, 0, 1, 1), Vec4(0, 0, 1, 1));
            }
        }

        return isMouseClickHandled;
    }
};
