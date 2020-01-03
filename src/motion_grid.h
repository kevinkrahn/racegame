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
        VEHICLE      = 1 << 2,
        HAZARD       = 1 << 3,
        TRACK        = 1 << 4,
        PICKUP_MONEY = 1 << 5,
        PICKUP_ARMOR = 1 << 6,
    };

    struct CellContents
    {
        f32 z = FLT_MAX;
        CellType staticCellType = CellType::NONE;
        CellType dynamicCellType = CellType::NONE;
    };

    struct Cell
    {
        CellContents contents[4];
    };

private:
    f32 x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    i32 width, height;
    static constexpr f32 CELL_SIZE = 2.f;
    static constexpr f32 AGENT_COLLISION_RADIUS = 1.f;

    std::unique_ptr<Cell[]> grid;

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
};
