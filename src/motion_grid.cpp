#include "motion_grid.h"
#include "scene.h"
#include "collision_flags.h"

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

    PxQueryFilterData filter;
    filter.flags |= PxQueryFlag::eSTATIC;
    filter.data = PxFilterData(COLLISION_FLAG_TRACK | COLLISION_FLAG_GROUND, 0, 0, 0);

    PxHitFlags hitFlags(PxHitFlag::eMESH_MULTIPLE |
                        PxHitFlag::ePOSITION |
                        PxHitFlag::eFACE_INDEX |
                        PxHitFlag::eASSUME_NO_INITIAL_OVERLAP);

    PxRaycastBuffer hit;

    // TODO: Speed this up with batched query
    u32 count = 0;
    for (i32 x = 0; x<width; ++x)
    {
        for (i32 y = 0; y<width; ++y)
        {
            f32 rx = x1 + x * width;
            f32 ry = y1 + y * width;

            if (scene->getPhysicsScene()->raycast(PxVec3(rx, ry, 5000.f), PxVec3(0, 0, -1),
                    10000.f, hit, hitFlags, filter))
            {
                PxMaterial* hitMaterial = hit.block.shape->getMaterialFromInternalFaceIndex(hit.block.faceIndex);
                if (hitMaterial == scene->trackMaterial)
                {
                    grid[y * width + x].contents[0].staticCellType = CellType::TRACK;
                }
                else if (hitMaterial == scene->offroadMaterial)
                {
                    grid[y * width + x].contents[0].staticCellType = CellType::OFFROAD;
                }
                else
                {
                    grid[y * width + x].contents[0].staticCellType = CellType::BLOCKED;
                }
            }
            ++count;
        }
    }

    print("Built motion grid: ", count, '\n');
}
