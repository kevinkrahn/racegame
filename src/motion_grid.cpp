#include "motion_grid.h"
#include "scene.h"
#include "collision_flags.h"
#include "game.h"

constexpr f32 D = 1.f;
constexpr f32 D2 = 1.41421356f * D;

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
    pathFindingBuffer.reset(new CellPathInfo[size]);

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
    const f32 overlapRadius = 4.f;

    u32 count = 0;
    u32 hitCount = 0;
    u32 highestLayerCount = 0;
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
                    if (hitMaterial == g_game.physx.materials.track)
                    {
                        // check for obstructions
                        bool obstructed = scene->getPhysicsScene()->overlap(PxSphereGeometry(overlapRadius),
                                PxTransform(PxVec3(rx, ry, hit.touches[i].position.z + overlapRadius), PxIdentity),
                                overlapHit, overlapFilter);
                        grid[y * width + x].contents.push_back({
                                hit.touches[i].position.z, obstructed ? CellType::BLOCKED : CellType::TRACK, CellType::NONE });
                        ++hitCount;
                    }
                }
                highestLayerCount = max(grid[y * width + x].contents.size(), highestLayerCount);
            }

            if (scene->terrain->isOffroadAt(rx, ry))
            {
                f32 tz = scene->terrain->getZ(Vec2(rx, ry));
                bool onSameLayerAsTrack = false;
                for (auto& layer : grid[y * width + x].contents)
                {
                    if (absolute(layer.z - tz) < 10.f)
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
                    contents.sort([](auto& a, auto& b) {
                        return a.z > b.z;
                    });
                }
            }

            ++count;
        }
    }

    print("Built motion grid. Cells checked: ", count, ", Cells used: ", hitCount,
            ", Layers: ", highestLayerCount, '\n');
}

void MotionGrid::setCell(Vec3 p, CellType cellType, bool permanent)
{
    p.x = clamp(p.x, x1, x2);
    p.y = clamp(p.y, y1, y2);

    i32 x = (i32)((p.x - x1) / CELL_SIZE);
    i32 y = (i32)((p.y - y1) / CELL_SIZE);
    i32 z = -1;
    auto& contents = grid[y * width + x].contents;
    for (u32 i=0; i<contents.size(); ++i)
    {
        CellContents& cell = contents[i];
        if (absolute(cell.z - p.z) < 4.f)
        {
            z = (i32)i;
            break;
        }
    }
    if (z == -1)
    {
        return;
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
            grid[y * width + x].contents[z].generation = (u32)g_game.frameCount;
        }
    }
}

void MotionGrid::setCells(Vec3 p, f32 radius, CellType cellType, bool permanent)
{
    p.x = clamp(p.x, x1, x2);
    p.y = clamp(p.y, y1, y2);
    Vec3 v1(p.x - radius, p.y - radius, p.z);
    Vec3 v2(p.x + radius, p.y + radius, p.z);
    i32 v1x = (i32)((v1.x - x1) / CELL_SIZE);
    i32 v1y = (i32)((v1.y - y1) / CELL_SIZE);
    i32 v2x = (i32)((v2.x - x1) / CELL_SIZE);
    i32 v2y = (i32)((v2.y - y1) / CELL_SIZE);
    for (i32 x = v1x; x <= v2x; ++x)
    {
        for (i32 y = v1y; y <= v2y; ++y)
        {
            i32 z = -1;
            auto& contents = grid[y * width + x].contents;
            for (u32 i=0; i<contents.size(); ++i)
            {
                CellContents& cell = contents[i];
                if (absolute(cell.z - p.z) < 4.f)
                {
                    z = (i32)i;
                    break;
                }
            }
            if (z == -1)
            {
                continue;
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
                    grid[y * width + x].contents[z].generation = (u32)g_game.frameCount;
                }
            }
        }
    }
}

void MotionGrid::debugDraw(class RenderWorld* rw)
{
    Mesh* mesh = g_res.getModel("misc")->getMeshByName("world.Sphere");
#if DEBUG_INFO
    for (auto& node : debugInfo)
    {
        f32 rx = x1 + node.x * CELL_SIZE;
        f32 ry = y1 + node.y * CELL_SIZE;
        Vec3 color(0.f, 0.f, 1.f);
        f32 rz = grid[node.y * width + node.x].contents[node.z].z;
        if (node.isBlocked)
        {
            rz += 2.f;
            color = Vec3(1.f, 0.f, 0.f);
        }
        drawSimple(rw, mesh, &g_res.white, glm::translate(Mat4(1.f), Vec3(rx, ry, rz)), color);
    }
#else
    Mat4 scale = glm::scale(Mat4(1.f), Vec3(0.4f));
    for (i32 x = 0; x<width; ++x)
    {
        for (i32 y = 0; y<height; ++y)
        {
            f32 rx = x1 + x * CELL_SIZE;
            f32 ry = y1 + y * CELL_SIZE;

            for (auto& cell : grid[y * width + x].contents)
            {
                auto cellType = cell.staticCellType;
                if (cellType != CellType::NONE)
                {
                    Vec3 color;
                    if (cellType == CellType::TRACK)
                    {
                        color = Vec3(0, 1, 0);
                    }
                    else if (cellType == CellType::OFFROAD)
                    {
                        color = Vec3(0, 0, 1);
                    }
                    else if (cellType == CellType::BLOCKED)
                    {
                        color = Vec3(1, 0, 0);
                    }

                    drawSimple(rw, mesh, &g_res.white, glm::translate(Mat4(1.f),
                            Vec3(rx, ry, cell.z)), color);
                }
                cellType = cell.dynamicCellType;
                if (cellType != CellType::NONE && cell.generation == (u32)g_game.frameCount)
                {
                    Vec3 color;
                    if (cellType == CellType::VEHICLE)
                    {
                        color = Vec3(0, 1, 1);
                    }
                    else if (cellType == CellType::TRACK)
                    {
                        color = Vec3(0, 1, 0);
                    }
                    else if (cellType == CellType::OFFROAD)
                    {
                        color = Vec3(0, 0, 1);
                    }
                    else if (cellType == CellType::BLOCKED)
                    {
                        color = Vec3(1, 0, 0);
                    }

                    drawSimple(rw, mesh, &g_res.white,
                        glm::translate(Mat4(1.f), Vec3(rx, ry, cell.z+4.f)) * scale, color);
                }
            }
        }
    }
#endif
}

i32 MotionGrid::getCellLayerIndex(Vec3 const& p) const
{
    i32 x = (i32)((p.x - x1) / CELL_SIZE);
    i32 y = (i32)((p.y - y1) / CELL_SIZE);
    auto& contents = grid[y * width + x].contents;
    for (u32 i=0; i<contents.size(); ++i)
    {
        CellContents& cell = contents[i];
        if (absolute(cell.z - p.z) < 4.f)
        {
            return (i32)i;
        }
    }
    return 0;
}

static f32 octileDistance(i32 x1, i32 y1, i32 x2, i32 y2)
{
    f32 dx = absolute(x1 - x2);
    f32 dy = absolute(y1 - y2);
    return D * (dx + dy) + (D2 - 2 * D) * min(dx, dy);
}

void MotionGrid::findPath(Vec3& from, Vec3& to, bool isBlocking, Vec2 forward,
        Array<PathNode>& outPath)
{
    outPath.clear();

    from.x = clamp(from.x, x1, x2);
    from.y = clamp(from.y, y1, y2);
    to.x = clamp(to.x, x1, x2);
    to.y = clamp(to.y, y1, y2);

    // TODO: if the start or end cell is blocked, search the area for a valid cell
    Node* startNode = g_game.tempMem.bump<Node>();
    *startNode = { (i32)((from.x - x1) / CELL_SIZE), (i32)((from.y - y1) / CELL_SIZE), getCellLayerIndex(from) };
    Node endNode = { (i32)((to.x - x1) / CELL_SIZE), (i32)((to.y - y1) / CELL_SIZE), getCellLayerIndex(to) };

#if DEBUG_INFO
    debugInfo.clear();
#endif

    open.clear();
    open.push_back(startNode);

    const u32 MAX_ITERATIONS = 800;
    u32 gen = (u32)g_game.frameCount;

    ++pathGeneration;
    u32 iterations = 0;
    while (!open.empty())
    {
        auto currentNodeIt = open.begin();
        for (auto it = open.begin()+1; it != open.end(); ++it)
        {
            if ((*it)->f < (*currentNodeIt)->f)
            {
                currentNodeIt = it;
            }
        }

        Node* currentNode = *currentNodeIt;

#if DEBUG_INFO
        debugInfo.push_back(*currentNode);
#endif

        open.erase(currentNodeIt);
        pathFindingBuffer[currentNode->y * width + currentNode->x].pathGeneration[currentNode->z] = pathGeneration;

        ++iterations;
        if ((currentNode->x == endNode.x && currentNode->y == endNode.y && currentNode->z == endNode.z)
                || iterations > MAX_ITERATIONS)
        {
            // if we have hit max iterations, then get as close to the goal as possible
            if (iterations > MAX_ITERATIONS)
            {
                currentNodeIt = open.begin();
                for (auto it = open.begin()+1; it != open.end(); ++it)
                {
                    if ((*it)->h < (*currentNodeIt)->h)
                    {
                        currentNodeIt = it;
                    }
                }
            }
            Node* current = currentNode;
            while (current)
            {
                outPath.push_back(PathNode{
                    {
                        x1 + current->x * CELL_SIZE,
                        y1 + current->y * CELL_SIZE,
                        grid[current->y * width + current->x].contents[current->z].z
                    },
                    current->f,
                    current->g,
                    current->h
                });
                current = current->parent;
            }
            std::reverse(outPath.begin(), outPath.end());
            return;
        }

        Vec2i offsets[] = {
            { 0, -1 },
            { 0, 1 },
            { -1, 0 },
            { 1, 0 },
            { -1, -1 },
            { -1, 1 },
            { 1, -1 },
            { 1, 1 },
        };
        f32 costs[] = { D, D, D, D, D2, D2, D2, D2 };
        for (u32 i=0; i<ARRAY_SIZE(offsets); ++i)
        {
            auto offset = offsets[i];
            i32 tx = currentNode->x + offset.x;
            i32 ty = currentNode->y + offset.y;
            i32 tz = 0;

            if (tx < 0 || tx >= width || ty < 0 || ty >= height)
            {
                continue;
            }

            CellContents* targetCell = nullptr;
            auto& contents = grid[ty * width + tx].contents;
            for (u32 i=0; i<contents.size(); ++i)
            {
                CellContents& cell = contents[i];
                if (cell.generation != gen)
                {
                    cell.dynamicCellType = CellType::NONE;
                }
                if (grid[currentNode->y * width + currentNode->x].contents.empty())
                {
                    break;
                }
                if (absolute(cell.z - grid[currentNode->y * width + currentNode->x].contents[currentNode->z].z) < 4.f)
                {
                    if (cell.staticCellType > CellType::BLOCKED &&
                        cell.dynamicCellType != CellType::BLOCKED)
                    {
                        targetCell = &cell;
                        tz = (i32)i;
                        break;
                    }
                }
            }

            if (!targetCell)
            {
                continue;
            }

            if (pathFindingBuffer[ty * width + tx].pathGeneration[tz] == pathGeneration)
            {
                continue;
            }

            Node newNode = {
                tx, ty, tz,
                0.f, 0.f, 0.f,
                currentNode,
            };
            f32 costMultiplier = 1.f;
            if (targetCell->staticCellType == CellType::OFFROAD)
            {
                costMultiplier = 1.75f;
            }
            Vec2 currentCellWorldPosition(x1 + currentNode->x * CELL_SIZE, y1 + currentNode->y * CELL_SIZE);
            Vec2 targetCellWorldPosition(x1 + newNode.x * CELL_SIZE, y1 + newNode.y * CELL_SIZE);
            Vec2 diff = targetCellWorldPosition - Vec2(from);
            f32 len = length(diff);
            if (isBlocking && len < 20.f && dot(diff / len, forward) > 0.9f)
            {
                costMultiplier = 3.f;
#if DEBUG_INFO
                Node debugInfoNode = newNode;
                debugInfoNode.isBlocked = true;
                debugInfo.push_back(debugInfoNode);
#endif
            }
            newNode.g = currentNode->g + costs[i] * costMultiplier;
            newNode.h = octileDistance(currentNode->x, currentNode->y, endNode.x, endNode.y) * 1.05f;
            newNode.f = newNode.g + newNode.h;

            bool isOnOpen = false;
            for (Node* openNode : open)
            {
                if (openNode->x == tx && openNode->y == ty && openNode->z == tz && newNode.g > openNode->g)
                {
                    isOnOpen = true;
                    break;
                }
            }

            if (!isOnOpen)
            {
                Node* node = g_game.tempMem.bump<Node>();
                *node = newNode;
                open.push_back(node);
            }
        }
    }
}
