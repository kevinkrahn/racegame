#pragma once

#include "math.h"
#include "smallvec.h"
#include <vector>

class NodeMap
{
public:
    enum CellType : u8
    {
        BLOCKED,
        OFFROAD,
        VEHICLE,
        HAZARD,
        TRACK
    };

    struct StaticCell
    {
        i16 minZ;
        i16 maxZ;
        CellType cellType;
    };

    struct DynamicCell
    {
        u32 generation;
        bool low;
        CellType cellType;
    };
    u32 generation = 1;

private:
    static constexpr u32 CELL_SIZE = 4;
    static constexpr u32 GRID_SIZE = 256;

    StaticCell staticGrid[GRID_SIZE][GRID_SIZE];
    DynamicCell dynamicGrid[GRID_SIZE][GRID_SIZE];

public:
    NodeMap()
    {
        memset(staticGrid, 0, sizeof(staticGrid));
        memset(dynamicGrid, 0, sizeof(dynamicGrid));
    }

    void build(class Track* track, class Terrain* terrain);

    void update() { ++generation; }
    u32 getGeneration() const { return generation; }

    void setStaticCell(f32 x, f32 y, f32 z, CellType cellType)
    {
    }

    void setDynamicCell(f32 x, f32 y, f32 z, CellType cellType)
    {
    }
};
