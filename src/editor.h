#pragma once

#include "misc.h"
#include "math.h"
#include "entity.h"
#include "texture.h"
#include "renderer.h"

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
    std::vector<DataFile::Value> serializedTransientEntities;

    bool clickHandledUntilRelease = false;

    GridSettings gridSettings;

    f32 brushRadius = 8.f;
    f32 brushFalloff = 1.f;
    f32 brushStrength = 15.f;

    f32 brushStartZ = 0.f;

    enum struct TerrainTool : i32
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
    i32 paintMaterialIndex = 2;

    enum struct EditMode : i32
    {
        TERRAIN,
        TRACK,
        DECORATION,
        PATHS,
        MAX
    } editMode = EditMode::TRACK;

    enum struct PlaceMode
    {
        NONE,
        NEW_SPLINE,
        MAX
    } placeMode = PlaceMode::NONE;

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
    i32 selectedEntityTypeIndex = 0;
    i32 selectedSplineTypeIndex = 0;

    bool canUseTerrainTool = false;

    std::vector<PlaceableEntity*> selectedEntities;

    void showEntityIcons();

public:
    Editor();
    void onUpdate(class Scene* scene, class Renderer* renderer, f32 deltaTime);
};

std::string chooseFile(const char* defaultSelection, bool open);
