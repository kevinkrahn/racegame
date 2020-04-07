#pragma once

#include "math.h"
#include "smallvec.h"
#include <vector>

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
#if DEBUG_INFO
        bool isBlocked = false;
#endif
    };
    std::vector<Node*> open;

#if DEBUG_INFO
    std::vector<Node> debugInfo;
#endif

public:
    MotionGrid()
    {
    }

    void build(class Scene* scene);

    void setCell(glm::vec3 p, CellType cellType, bool permanent=false);
    void setCells(glm::vec3 p, f32 radius, CellType cellType, bool permanent=false);

    i32 getCellLayerIndex(glm::vec3 const& p) const;

    void findPath(glm::vec3& from, glm::vec3& to, bool isBlockedAhead, glm::vec2 forward,
            std::vector<PathNode>& outPath);

    CellType getCellBleed(i32 x, i32 y, f32 z, CellType cellType);

    void debugDraw(class RenderWorld* rw);
};
