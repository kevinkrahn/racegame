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
    virtual void gizmoDrag(glm::vec3 const& dragCenter, glm::vec3 const& dragDest,
            glm::vec3 const& dragDestZ, glm::vec3 dragOffset, i32 dragAxis) = 0;
    virtual void gizmoRotate(f32 angleDiff, glm::vec3 const& rotatePivot, i32 dragAxis) = 0;
    virtual void gizmoScale(f32 scaleFactor, i32 dragAxis, glm::vec3 const& scaleCenter,
            glm::vec3 const& dragOffset, glm::vec3 const& hitPos, glm::vec3& hitPosZ) = 0;
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
    glm::vec3 entityDragOffset;
    glm::vec3 rotatePivot;

public:
    TransformMode getTransformMode() const { return transformMode; }
    void setTransformMode(TransformMode mode) { transformMode = mode; }

    void reset()
    {
        entityDragAxis = DragAxis::NONE;
    }

    bool update(glm::vec3 const& p, Scene* scene, RenderWorld* rw, f32 deltaTime,
            glm::mat4 const& orientation, TransformGizmoHandler* handler, bool drawCenter=true)
    {
        bool isMouseClickHandled = ImGui::GetIO().WantCaptureMouse;
        bool isKeyboardHandled = ImGui::GetIO().WantCaptureKeyboard;

        f32 rot = (f32)M_PI * 0.5f;
        glm::vec2 windowSize(g_game.windowWidth, g_game.windowHeight);
        glm::mat4 viewProj = rw->getCamera(0).viewProjection;

        glm::vec3 rayDir = scene->getEditorCamera().getMouseRay(rw);
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
            glm::vec2 mousePos = g_input.getMousePosition();
            entityDragOffset.x = pointDirection(mousePos, project(rotatePivot, viewProj) * windowSize);
        }

        if (!isKeyboardHandled && g_input.isKeyPressed(KEY_F))
        {
            transformMode = TransformMode::SCALE;
            entityDragAxis = DragAxis::MIDDLE;
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
            f32 xyT = rayPlaneIntersection(cam.position, rayDir, glm::vec3(0, 0, 1), p);
            glm::vec3 hitPos = cam.position + rayDir * xyT;
            f32 zT = rayPlaneIntersection(cam.position, rayDir,
                    glm::normalize(glm::vec3(glm::vec2(-rayDir), 0.f)), p);
            glm::vec3 hitPosZ = cam.position + rayDir * zT;

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

                bool shortcut = !isKeyboardHandled && g_input.isKeyPressed(KEY_G);
                if (glm::length(center - mousePos) < radius || shortcut)
                {
                    centerCol = glm::vec3(1.f);
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

            Mesh* arrowMesh = g_res.getModel("misc")->getMeshByName("world.TranslateArrow");
            Mesh* centerMesh = g_res.getModel("misc")->getMeshByName("world.Sphere");
            if (drawCenter)
            {
                drawOverlay(rw, centerMesh,
                        glm::translate(glm::mat4(1.f), p), centerCol, -1);
            }

            drawOverlay(rw, arrowMesh,
                    glm::translate(glm::mat4(1.f), p), xCol);
            if (entityDragAxis & DragAxis::X)
            {
                scene->debugDraw.line(
                        p - glm::vec3(100.f, 0.f, 0.f), p + glm::vec3(100.f, 0.f, 0.f),
                        glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1));
            }

            drawOverlay(rw, arrowMesh,
                    glm::translate(glm::mat4(1.f), p) *
                    glm::rotate(glm::mat4(1.f), rot, glm::vec3(0, 0, 1)), yCol);
            if (entityDragAxis & DragAxis::Y)
            {
                scene->debugDraw.line(
                        p - glm::vec3(0.f, 100.f, 0.f), p + glm::vec3(0.f, 100.f, 0.f),
                        glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1));
            }

            drawOverlay(rw, arrowMesh,
                    glm::translate(glm::mat4(1.f), p) *
                    glm::rotate(glm::mat4(1.f), -rot, glm::vec3(0, 1, 0)), zCol);
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
                entityDragOffset.x = angle;
                handler->gizmoRotate(angleDiff, rotatePivot, entityDragAxis);
            }
            else
            {
                Mesh* arrowMesh = g_res.getModel("misc")->getMeshByName("world.RotateArrow");
                Mesh* sphereMesh = g_res.getModel("misc")->getMeshByName("world.Sphere");
                drawOverlay(rw, sphereMesh,
                        glm::translate(glm::mat4(1.f), p) * glm::scale(glm::mat4(1.f), glm::vec3(4.4f)),
                        glm::vec3(1.f), true);

                drawOverlay(rw, arrowMesh,
                        glm::translate(glm::mat4(1.f), p) *
                        glm::rotate(glm::mat4(1.f), rot, glm::vec3(0, 1, 0)), xCol);
                if (entityDragAxis & DragAxis::X)
                {
                    scene->debugDraw.line(
                            p - glm::vec3(100.f, 0.f, 0.f), p + glm::vec3(100.f, 0.f, 0.f),
                            glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1));
                }

                drawOverlay(rw, arrowMesh,
                        glm::translate(glm::mat4(1.f), p) *
                        glm::rotate(glm::mat4(1.f), rot, glm::vec3(1, 0, 0)), yCol);
                if (entityDragAxis & DragAxis::Y)
                {
                    scene->debugDraw.line(
                            p - glm::vec3(0.f, 100.f, 0.f), p + glm::vec3(0.f, 100.f, 0.f),
                            glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1));
                }

                drawOverlay(rw, arrowMesh, glm::translate(glm::mat4(1.f), p), zCol);
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

                bool shortcut = !isKeyboardHandled && g_input.isKeyPressed(KEY_F);
                if (glm::length(center - mousePos) < radius || shortcut)
                {
                    centerCol = glm::vec3(1.f);
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
                f32 startDistance = glm::length(entityDragOffset);
                f32 scaleFactor = glm::length(p - hitPos) / startDistance;
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

            Mesh* arrowMesh = g_res.getModel("misc")->getMeshByName("world.ScaleArrow");
            Mesh* centerMesh = g_res.getModel("misc")->getMeshByName("world.UnitCube");
            if (drawCenter)
            {
                drawOverlay(rw, centerMesh,
                        glm::translate(glm::mat4(1.f), p) * orientation, centerCol, -1);
            }

            drawOverlay(rw, arrowMesh,
                    glm::translate(glm::mat4(1.f), p) * orientation, xCol);
            if (entityDragAxis & DragAxis::X)
            {
                scene->debugDraw.line(
                        p - glm::vec3(100.f, 0.f, 0.f), p + glm::vec3(100.f, 0.f, 0.f),
                        glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1));
            }

            drawOverlay(rw, arrowMesh,
                    glm::translate(glm::mat4(1.f), p) *
                    orientation *
                    glm::rotate(glm::mat4(1.f), rot, glm::vec3(0, 0, 1)), yCol);
            if (entityDragAxis & DragAxis::Y)
            {
                scene->debugDraw.line(
                        p - glm::vec3(0.f, 100.f, 0.f), p + glm::vec3(0.f, 100.f, 0.f),
                        glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1));
            }

            drawOverlay(rw, arrowMesh,
                    glm::translate(glm::mat4(1.f), p) *
                    orientation *
                    glm::rotate(glm::mat4(1.f), -rot, glm::vec3(0, 1, 0)), zCol);
            if (entityDragAxis & DragAxis::Z)
            {
                scene->debugDraw.line(
                        p - glm::vec3(0.f, 0.f, 100.f), p + glm::vec3(0.f, 0.f, 100.f),
                        glm::vec4(0, 0, 1, 1), glm::vec4(0, 0, 1, 1));
            }
        }

        return isMouseClickHandled;
    }
};
