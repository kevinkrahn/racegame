#pragma once

#include "misc.h"
#include "math.h"
#include "entity.h"

struct GridSettings
{
    bool show = false;
    bool snap = false;
    f32 cellSize = 4.f;
    f32 z = 2.f;
};

namespace DragAxis
{
    enum
    {
        NONE = 0,
        X = 1 << 0,
        Y = 1 << 1,
        Z = 1 << 2,
        ALL = X | Y,
    };
}

class Editor
{
    f32 cameraDistance = 90.f;
    f32 zoomSpeed = 0.f;
    f32 cameraAngle = 0.f;
    f32 cameraRotateSpeed = 0.f;
    glm::vec3 cameraTarget = glm::vec3(0, 0, 0);
    glm::vec2 lastMousePosition;
    glm::vec3 cameraVelocity = glm::vec3(0, 0, 0);

    bool clickHandledUntilRelease = false;

    GridSettings gridSettings;

    f32 brushRadius = 8.f;
    f32 brushFalloff = 1.f;
    f32 brushStrength = 15.f;

    f32 brushStartZ = 0.f;

    enum struct TerrainTool
    {
        RAISE,
        PERTURB,
        FLATTEN,
        SMOOTH,
        ERODE,
        MATCH_TRACK,
        PAINT,
        MAX
    } terrainTool = TerrainTool::RAISE;
    u32 paintMaterialIndex = 2;

    enum struct EditMode
    {
        TERRAIN,
        TRACK,
        DECORATION,
        MAX
    } editMode = EditMode::TERRAIN;

    enum struct PlaceMode
    {
        NONE,
        NEW_RAILING,
        NEW_MARKING,
        MAX
    } placeMode = PlaceMode::NONE;

    enum struct TransformMode
    {
        TRANSLATE,
        ROTATE,
        SCALE,
        MAX
    } transformMode = TransformMode::TRANSLATE;

    u32 entityDragAxis = DragAxis::NONE;
    glm::vec3 entityDragOffset;
    glm::vec3 rotatePivot;
    u32 selectedEntityTypeIndex = 0;

    std::vector<PlaceableEntity*> selectedEntities;

public:
    glm::vec3 getCameraTarget() const { return cameraTarget; }
    void onUpdate(class Scene* scene, class Renderer* renderer, f32 deltaTime);
};
