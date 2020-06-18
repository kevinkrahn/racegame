#pragma once

#include "misc.h"

#define DEBUG_INFO 0

class MotionGrid
{
public:
    static constexpr f32 CELL_SIZE = 2.f;

    enum CellType : u8
    {
        NONE         = 0,
        BLOCKED      = 1 << 0,
        OFFROAD      = 1 << 1,
        TRACK        = 1 << 2,
        VEHICLE      = 1 << 3,
        HAZARD       = 1 << 4,
        PICKUP_MONEY = 1 << 5,
        PICKUP_ARMOR = 1 << 6,
    };

    struct CellContents
    {
        f32 z;
        CellType staticCellType;
        CellType dynamicCellType;
        u32 generation;
    };

    struct Cell
    {
        SmallArray<CellContents> contents;
    };

    struct PathNode
    {
        Vec3 p;
        f32 f, g, h;
    };

private:
    f32 x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    i32 width, height;

    OwnedPtr<Cell[]> grid;

    u32 pathGeneration = 0;
    struct CellPathInfo
    {
        u32 pathGeneration[8] = { 0 };
    };
    OwnedPtr<CellPathInfo[]> pathFindingBuffer;

    struct Node
    {
        i32 x;
        i32 y;
        i32 z;
        f32 g = 0.f;
        f32 h = 0.f;
        f32 f = 0.f;
        Node* parent = nullptr;
#if DEBUG_INFO
        bool isBlocked = false;
#endif
    };
    Array<Node*> open;

#if DEBUG_INFO
    Array<Node> debugInfo;
#endif

public:
    MotionGrid()
    {
    }

    void build(class Scene* scene);

    void setCell(Vec3 p, CellType cellType, bool permanent=false);
    void setCells(Vec3 p, f32 radius, CellType cellType, bool permanent=false);

    i32 getCellLayerIndex(Vec3 const& p) const;

    void findPath(Vec3& from, Vec3& to, bool isBlockedAhead, Vec2 forward,
            Array<PathNode>& outPath);

    CellType getCellBleed(i32 x, i32 y, f32 z, CellType cellType);

    void debugDraw(class RenderWorld* rw);
};
