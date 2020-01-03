#include "motion_grid.h"
#include "scene.h"
#include "collision_flags.h"
#include "mesh_renderables.h"

void MotionGrid::build(Scene* scene)
{
    f32 x1 = scene->terrain->x1;
    f32 y1 = scene->terrain->y1;
    f32 x2 = scene->terrain->x2;
    f32 y2 = scene->terrain->y2;

    this->x1 = snap(x1, CELL_SIZE);
    this->y1 = snap(y1, CELL_SIZE);
    this->x2 = snap(x2, CELL_SIZE);
    this->y2 = snap(y2, CELL_SIZE);
    width = (i32)((this->x2 - this->x1) / CELL_SIZE);
    height = (i32)((this->y2 - this->y1) / CELL_SIZE);
	i32 size = width * height;
    grid.reset(new Cell[size]);

    PxRaycastHit hitBuffer[8];
    PxRaycastBuffer hit(hitBuffer, ARRAY_SIZE(hitBuffer));
    PxQueryFilterData filter;
    filter.flags |= PxQueryFlag::eSTATIC;
    filter.data = PxFilterData(COLLISION_FLAG_TRACK, 0, 0, 0);
    PxHitFlags hitFlags(PxHitFlag::eMESH_MULTIPLE |
                        PxHitFlag::ePOSITION |
                        PxHitFlag::eFACE_INDEX |
                        PxHitFlag::eASSUME_NO_INITIAL_OVERLAP);

    PxOverlapHit overlapHitBuffer[8];
    PxOverlapBuffer overlapHit(overlapHitBuffer, ARRAY_SIZE(overlapHitBuffer));
    PxQueryFilterData overlapFilter;
    overlapFilter.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eANY_HIT;
    overlapFilter.data = PxFilterData(COLLISION_FLAG_OBJECT, 0, 0, 0);
    const f32 overlapRadius = 2.f;

    u32 count = 0;
    u32 hitCount = 0;
    for (i32 x = 0; x<width; ++x)
    {
        for (i32 y = 0; y<height; ++y)
        {
            f32 rx = x1 + x * CELL_SIZE;
            f32 ry = y1 + y * CELL_SIZE;

            if (scene->getPhysicsScene()->raycast(PxVec3(rx, ry, 5000.f), PxVec3(0, 0, -1),
                    10000.f, hit, hitFlags, filter))
            {
                // create layer for each track hit
                for (u32 i=0; i<hit.nbTouches; ++i)
                {
                    PxMaterial* hitMaterial =
                        hit.touches[i].shape->getMaterialFromInternalFaceIndex(hit.touches[i].faceIndex);
                    if (hitMaterial == scene->trackMaterial)
                    {
                        // check for obstructions
                        bool obstructed = scene->getPhysicsScene()->overlap(PxSphereGeometry(overlapRadius),
                                PxTransform(PxVec3(rx, ry, hit.touches[i].position.z + overlapRadius), PxIdentity),
                                overlapHit, overlapFilter);
                        grid[y * width + x].contents.push_back({
                                hit.touches[i].position.z, obstructed ? CellType::BLOCKED : CellType::TRACK });
                        ++hitCount;
                    }
                }
            }

            if (scene->terrain->isOffroadAt(rx, ry))
            {
                f32 tz = scene->terrain->getZ(glm::vec2(rx, ry));
                bool onSameLayerAsTrack = false;
                for (auto& layer : grid[y * width + x].contents)
                {
                    if (glm::abs(layer.z - tz) < 10.f)
                    {
                        onSameLayerAsTrack = true;
                        break;
                    }
                }
                if (!onSameLayerAsTrack)
                {
                    // check for obstructions
                    bool obstructed = scene->getPhysicsScene()->overlap(PxSphereGeometry(overlapRadius),
                            PxTransform(PxVec3(rx, ry, tz + overlapRadius), PxIdentity),
                            overlapHit, overlapFilter);
                    auto& contents = grid[y * width + x].contents;
                    contents.push_back({ tz, obstructed ? CellType::BLOCKED : CellType::OFFROAD });
                    std::sort(contents.begin(), contents.end(), [](auto& a, auto& b) {
                        return a.z > b.z;
                    });
                }
            }

            ++count;
        }
    }

    /*
    struct CellAdjustment
    {
        i32 index;
        u8 layerIndex;
        CellType cellType;
    };
    std::vector<CellAdjustment> cellAdjustments;
    cellAdjustments.reserve(1000);
    for (i32 x = 0; x<width; ++x)
    {
        for (i32 y = 0; y<width; ++y)
        {
            auto& contents = grid[y * width + x].contents;
            for (u32 layer = 0; layer < contents.size(); ++layer)
            {
                f32 z = contents[layer].z;
                CellType cellType = contents[layer].staticCellType;
                cellType = getCellBleed(x + 1, y, z, cellType);
                cellType = getCellBleed(x - 1, y, z, cellType);
                cellType = getCellBleed(x, y + 1, z, cellType);
                cellType = getCellBleed(x, y - 1, z, cellType);
                cellType = getCellBleed(x + 1, y + 1, z, cellType);
                cellType = getCellBleed(x + 1, y - 1, z, cellType);
                cellType = getCellBleed(x - 1, y + 1, z, cellType);
                cellType = getCellBleed(x - 1, y - 1, z, cellType);
                cellAdjustments.push_back({ y * width + x, (u8)layer, cellType });
            }
        }
    }

    for (auto& c : cellAdjustments)
    {
        grid[c.index].contents[c.layerIndex].staticCellType = c.cellType;
    }
    */

    print("Built motion grid: ", count, " (", hitCount, ")\n");
}

MotionGrid::CellType MotionGrid::getCellBleed(i32 x, i32 y, f32 z, CellType cellType)
{
    for (auto& layer : grid[y * width + x].contents)
    {
        if (glm::abs(z - layer.z) < 10.f)
        {
            CellType otherCellType = layer.staticCellType;
            return otherCellType < cellType ? otherCellType : cellType;
        }
    }
    return CellType::BLOCKED;
}

void MotionGrid::debugDraw(class RenderWorld* rw)
{
    Mesh* mesh = g_res.getMesh("world.Sphere");
    for (i32 x = 0; x<width; ++x)
    {
        for (i32 y = 0; y<height; ++y)
        {
            f32 rx = x1 + x * CELL_SIZE;
            f32 ry = y1 + y * CELL_SIZE;

            for (auto& cell : grid[y * width + x].contents)
            {
                if (cell.staticCellType != CellType::NONE)
                {
                    glm::vec3 color;
                    if (cell.staticCellType == CellType::TRACK)
                    {
                        color = glm::vec3(0, 1, 0);
                    }
                    else if (cell.staticCellType == CellType::OFFROAD)
                    {
                        color = glm::vec3(0, 0, 1);
                    }
                    else if (cell.staticCellType == CellType::BLOCKED)
                    {
                        color = glm::vec3(1, 0, 0);
                    }

                    rw->push(LitRenderable(mesh, glm::translate(glm::mat4(1.f), glm::vec3(rx, ry, cell.z)),
                                nullptr, color));
                }
            }
        }
    }
}
