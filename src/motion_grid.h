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

    struct PathNode
    {
        glm::vec3 p;
        f32 f, g, h;
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

    void setCell(glm::vec3 const& p, CellType cellType, bool permanent=false)
    {
        i32 x = (i32)((p.x - x1) / CELL_SIZE);
        i32 y = (i32)((p.y - y1) / CELL_SIZE);
        i32 z = 0;
        auto& contents = grid[y * width + x].contents;
        for (u32 i=0; i<contents.size(); ++i)
        {
            CellContents& cell = contents[i];
            if (glm::abs(cell.z - p.z) < 4.f)
            {
                z = (i32)i;
                break;
            }
        }
        if (grid[y * width + x].contents[z].staticCellType != CellType::BLOCKED)
        {
            if (permanent)
            {
                grid[y * width + x].contents[z].staticCellType = cellType;
            }
            else
            {
                grid[y * width + x].contents[z].dynamicCellType = cellType;
            }
        }
    }

    i32 getCellLayerIndex(glm::vec3 const& p) const;

    void findPath(glm::vec3& from, glm::vec3& to, std::vector<PathNode>& outPath);

    CellType getCellBleed(i32 x, i32 y, f32 z, CellType cellType);

    void debugDraw(class RenderWorld* rw);
};
