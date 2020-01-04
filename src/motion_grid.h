#pragma once

#include "math.h"
#include "smallvec.h"
#include <vector>

class MotionGrid
{
public:
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
    };

    struct Cell
    {
        SmallVec<CellContents> contents;
    };

private:
    f32 x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    i32 width, height;
    static constexpr f32 CELL_SIZE = 2.f;

    std::unique_ptr<Cell[]> grid;

    u32 pathGeneration = 0;
    struct CellPathInfo
    {
        u32 pathGeneration[8] = { 0 };
    };
    std::unique_ptr<CellPathInfo[]> pathFindingBuffer;

    struct Node
    {
        i32 x;
        i32 y;
        i32 z;
        f32 g = 0.f;
        f32 h = 0.f;
        f32 f = 0.f;
        Node* parent = nullptr;
    };
    std::vector<Node*> open;

public:
    MotionGrid()
    {
    }

    void build(class Scene* scene);

    void setStaticCell(f32 x, f32 y, f32 z, CellType cellType)
    {
    }

    void setDynamicCell(f32 x, f32 y, f32 z, CellType cellType)
    {
    }

    std::vector<glm::vec3> findPath(glm::vec3 const& from, glm::vec3 const& to);

    CellType getCellBleed(i32 x, i32 y, f32 z, CellType cellType);

    void debugDraw(class RenderWorld* rw);
};
