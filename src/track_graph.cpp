#include "track_graph.h"
#include "game.h"
#include "renderer.h"
#include "resources.h"
#include "debug_draw.h"
#include "font.h"
#include "track.h"
#include "input.h"

void TrackGraph::computeTravelTime(u32 toIndex, u32 fromIndex, u32 endIndex)
{
    Node& to = nodes[toIndex];
    Node& from = nodes[fromIndex];
    f32 travelTime = from.t + length(from.position - to.position);
    if (to.t <= travelTime)
    {
        return;
    }
    to.t = travelTime;
    if (toIndex == endIndex)
    {
        return;
    }
    for (u32 i=0; i<to.connections.size(); ++i)
    {
        u32 c = to.connections[i];
        if (c != fromIndex)
        {
            computeTravelTime(c, toIndex, endIndex);
        }
    }
}

void TrackGraph::computePath(u32 toIndex, u32 fromIndex, u32 pathIndex,
        Array<Array<u32>>& nodeIndexPaths)
{
    Array<u32>& path = nodeIndexPaths[pathIndex];
    // never visit the same node twice
    if (path.find(toIndex))
    {
        return;
    }
    Node& to = nodes[toIndex];
    nodeIndexPaths[pathIndex].push(toIndex);
    u32 branchCount = 0;
    Array<u32> pathToHere = nodeIndexPaths[pathIndex];
    for (u32 i=0; i<to.connections.size(); ++i)
    {
        u32 c = to.connections[i];
        if (c != fromIndex)
        {
            if (branchCount > 0)
            {
                pathIndex = (u32)nodeIndexPaths.size();
                nodeIndexPaths.push(pathToHere);
            }
            computePath(c, toIndex, pathIndex, nodeIndexPaths);
            ++branchCount;
        }
    }
}

u32 TrackGraph::addNode(Vec3 const& position)
{
    nodes.push({ position });
    return (u32)nodes.size() - 1;
}

void TrackGraph::addConnection(u32 fromIndex, u32 toIndex)
{
    nodes[fromIndex].connections.push(toIndex);
    nodes[toIndex].connections.push(fromIndex);
}

void TrackGraph::rebuild(Mat4 const& startTransform)
{
    valid = true;

    u32 startIndex = 0;
    f32 minDist = FLT_MAX;
    for (u32 i=0; i<nodes.size(); ++i)
    {
        Node const& node = nodes[i];
        f32 dist = lengthSquared(node.position - startTransform.position());
        if (dist < minDist)
        {
            minDist = dist;
            startIndex = i;
        }
    }

    if (minDist > square(50.f))
    {
        error("There is no track graph vertex close enough to the finish line.\n");
        valid = false;
    }

    nodes[startIndex].position = Vec3(Vec2(startTransform.position()), nodes[startIndex].position.z)
            + startTransform.xAxis() * 2.f;
    u32 endIndex = (u32)nodes.size();

    // copy the start node to make the end node
    Node newEndNode = nodes[startIndex];
    nodes.push(newEndNode);
    Node& end = nodes.back();
    end.connections.clear();

	Node& start = nodes[startIndex];
    for (auto c = start.connections.begin(); c != start.connections.end();)
    {
        Node& connection = nodes[*c];
        if (dot(normalize(connection.position - start.position), startTransform.xAxis()) <= 0.f)
        {
            for (auto o = connection.connections.begin(); o != connection.connections.end(); ++o)
            {
                if (*o == startIndex)
                {
                    connection.connections.erase(o);
                    connection.connections.push(endIndex);
                    break;
                }
            }
            end.connections.push(*c);
            start.connections.erase(c);
            continue;
        }
        ++c;
    }

    if (end.connections.empty())
    {
        error("Invalid track graph: cannot find any connections to end node.\n");
        valid = false;
    }

    end.t = 0.f;
    for (u32 c : end.connections)
    {
        computeTravelTime(c, endIndex, startIndex);
    }

    Array<Array<u32>> nodeIndexPaths;
    for (u32 c : start.connections)
    {
        nodeIndexPaths.push({ startIndex });
        computePath(c, startIndex, (u32)nodeIndexPaths.size() - 1, nodeIndexPaths);
    }

    Array<f32> pathLengths;
    for (auto it = nodeIndexPaths.begin(); it != nodeIndexPaths.end();)
    {
        // filter out paths that didn't reach the end
        if (it->back() != endIndex)
        {
            it = nodeIndexPaths.erase(it);
            continue;
        }

        // compute the length of the path
        f32 totalLength = 0.f;
        for (u32 i=1; i<it->size(); ++i)
        {
            totalLength +=
                length(nodes[(*it)[i]].position - nodes[(*it)[i-1]].position);
        }
        pathLengths.push(totalLength);

        ++it;
    }

    // sort paths based on length
    for (u32 i=1; i<(u32)nodeIndexPaths.size(); ++i)
    {
        for (u32 j=i; j>0 && pathLengths[j-1] > pathLengths[j]; --j)
        {
            swp(nodeIndexPaths[j], nodeIndexPaths[j-1]);
        }
    }

    // convert node indices to positions
    for (auto& indexPath : nodeIndexPaths)
    {
        Array<Node*> path;
        path.reserve(indexPath.size());
        for (u32 nodeIndex : indexPath)
        {
            path.push(&nodes[nodeIndex]);
        }
        paths.push(std::move(path));
    }

    // set node angles
    for (i32 j=(i32)nodeIndexPaths.size() - 1; j >= 0; --j)
    {
        for (u32 i=0; i<nodeIndexPaths[j].size(); ++i)
        {
            Vec3 fromPosition;
            Vec3 toPosition;
            if (i < nodeIndexPaths[j].size() - 1)
            {
                toPosition = nodes[nodeIndexPaths[j][i]].position;
                fromPosition = nodes[nodeIndexPaths[j][i+1]].position;
            }
            else
            {
                toPosition = nodes[nodeIndexPaths[j][i-1]].position;
                fromPosition = nodes[nodeIndexPaths[j][i]].position;
            }
            nodes[nodeIndexPaths[j][i]].angle = pointDirection(Vec2(fromPosition), Vec2(toPosition)) - PI * 0.5f;
            nodes[nodeIndexPaths[j][i]].direction = normalize(fromPosition - toPosition);
        }
    }

    end.angle = start.angle;
    end.direction = start.direction;

    println("Built track graph: %u nodes, %u paths", nodes.size(), paths.size());

    startNode = &nodes[startIndex];
    endNode = &nodes[endIndex];
}

i32 pathPointDrawCount = 0;
void TrackGraph::debugDraw(DebugDraw* dbg, Renderer* renderer) const
{
    if (g_input.isKeyPressed(KEY_O))
    {
        --pathPointDrawCount;
        if (pathPointDrawCount < 0)
        {
            pathPointDrawCount = 0;
        }
    }
    if (g_input.isKeyPressed(KEY_P))
    {
        ++pathPointDrawCount;
    }

    Mesh* arrowMesh = g_res.getModel("misc")->getMeshByName("world.Arrow");
    for (u32 i=0; i<nodes.size(); ++i)
    {
        Node const& c = nodes[i];

        drawSimple(renderer->getRenderWorld(), arrowMesh, &g_res.white,
                Mat4::translation(c.position) * Mat4::rotationZ(c.angle) * Mat4::scaling(Vec3(1.25f)));

        for (u32 connection : c.connections)
        {
            dbg->line(c.position, nodes[connection].position,
                    Vec4(1.f, 0.6f, 0.f, 1.f), Vec4(1.f, 0.6f, 0.f, 1.f));
        }

#if 0
        Vec4 p = g_game.renderer->getRenderWorld()->getCamera(0).viewProjection *
            Vec4(c.position + Vec3(0, 0, 1) * f32(i % 2) * 2.f, 1.f);
        p.x = (((p.x / p.w) + 1.f) / 2.f) * g_game.windowWidth;
        p.y = ((-1.f * (p.y / p.w) + 1.f) / 2.f) * g_game.windowHeight;
        renderer->push2D(TextRenderable(
                    &g_res.getFont("font", 23),
                    tstr(std::fixed, std::setprecision(1), c.t), { p.x, p.y },
                    Vec3(0.f, 0.f, 1.f), 1.f, 1.f, HorizontalAlign::CENTER));
#endif
    }

    Vec4 colors[] = {
        { 1, 0, 0, 1 },
        { 0, 1, 0, 1 },
        { 0, 0, 1, 1 },
        { 0, 1, 1, 1 },
        { 1, 0, 1, 1 },
    };
    for (u32 pathIndex=0; pathIndex < paths.size(); ++pathIndex)
    {
        for (u32 i=1; i<paths[pathIndex].size(); ++i)
        {
            if (i > (u32)pathPointDrawCount)
            {
                break;
            }
            Vec4 col = colors[pathIndex % ARRAY_SIZE(colors)];
            Vec3 zoffset(0, 0, pathIndex * 2);
            dbg->line(paths[pathIndex][i]->position + zoffset,
                    paths[pathIndex][i-1]->position + zoffset, col, col);
        }
    }
}

void TrackGraph::findLapDistance(Vec3 const& p, QueryResult& queryResult, f32 maxSkippableDistance) const
{
    f32 minDistance = FLT_MAX;
    for (Node const& node : nodes)
    {
        for (u32 i=0; i<node.connections.size(); ++i)
        {
            u32 connectionNodeIndex = node.connections[i];
            const Node* nodeA = &node;
            const Node* nodeB = &nodes[connectionNodeIndex];
            if (nodeA->t > nodeB->t)
            {
                nodeA = nodeB;
                nodeB = &node;
            }

            Vec3 a = nodeA->position;
            Vec3 b = nodeB->position;

            Vec3 ap = p - a;
            Vec3 ab = b - a;
            f32 distanceAlongLine = clamp(dot(ap, ab) / lengthSquared(ab), 0.f, 1.f);
            f32 t = nodeA->t + distanceAlongLine * (nodeB->t - nodeA->t);

            if (queryResult.lapDistanceLowMark - t < maxSkippableDistance)
            {
                Vec3 result = a + distanceAlongLine * ab;
                f32 distance = lengthSquared(p - result);
                // prioritize points that don't loose progress
                if (queryResult.lapDistanceLowMark - t < -10.f)
                {
                    distance += 800.f;
                }
                if (minDistance > distance && distance < square(35))
                {
                    minDistance = distance;
                    queryResult.currentLapDistance = t;
                    queryResult.lastNode = nodeB;
                    queryResult.position = result;
                }
            }
        }
    }

    queryResult.lapDistanceLowMark = min(queryResult.lapDistanceLowMark, queryResult.currentLapDistance);
}

f32 TrackGraph::findTrackProgressAtPoint(Vec3 const& p, f32 referenceValue) const
{
    f32 minDistance = FLT_MAX;
    f32 currentDistance = 0.f;
    for (Node const& node : nodes)
    {
        for (u32 i=0; i<node.connections.size(); ++i)
        {
            u32 connectionNodeIndex = node.connections[i];
            const Node* nodeA = &node;
            const Node* nodeB = &nodes[connectionNodeIndex];
            if (nodeA->t > nodeB->t)
            {
                nodeA = nodeB;
                nodeB = &node;
            }

            Vec3 a = nodeA->position;
            Vec3 b = nodeB->position;

            Vec3 ap = p - a;
            Vec3 ab = b - a;
            f32 distanceAlongLine = clamp(dot(ap, ab) / lengthSquared(ab), 0.f, 1.f);
            f32 t = nodeA->t + distanceAlongLine * (nodeB->t - nodeA->t);

            if (referenceValue - t < 150.f)
            {
                Vec3 result = a + distanceAlongLine * ab;
                f32 distance = lengthSquared(p - result);
                // prioritize points that don't loose progress
                if (referenceValue - t < -10.f)
                {
                    distance += 800.f;
                }
                if (minDistance > distance && distance < square(35))
                {
                    minDistance = distance;
                    currentDistance = t;
                }
            }
        }
    }
    return currentDistance;
}
