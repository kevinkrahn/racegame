#include "track_graph.h"
#include "game.h"
#include "renderer.h"
#include "resources.h"
#include "debug_draw.h"
#include "font.h"
#include "track.h"
#include "mesh_renderables.h"
#include "input.h"
#include <iomanip>

void TrackGraph::computeTravelTime(u32 toIndex, u32 fromIndex, u32 endIndex)
{
    Node& to = nodes[toIndex];
    Node& from = nodes[fromIndex];
    f32 travelTime = from.t + glm::length(from.position - to.position);
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
        std::vector<std::vector<u32>>& nodeIndexPaths)
{
    std::vector<u32>& path = nodeIndexPaths[pathIndex];
    // never visit the same node twice
    if (std::find(path.begin(), path.end(), toIndex) != path.end())
    {
        return;
    }
    Node& to = nodes[toIndex];
    nodeIndexPaths[pathIndex].push_back(toIndex);
    u32 branchCount = 0;
    std::vector<u32> pathToHere = nodeIndexPaths[pathIndex];
    for (u32 i=0; i<to.connections.size(); ++i)
    {
        u32 c = to.connections[i];
        if (c != fromIndex)
        {
            if (branchCount > 0)
            {
                pathIndex = (u32)nodeIndexPaths.size();
                nodeIndexPaths.push_back(pathToHere);
            }
            computePath(c, toIndex, pathIndex, nodeIndexPaths);
            ++branchCount;
        }
    }
}

void TrackGraph::addNode(glm::vec3 const& position)
{
    nodes.push_back({ position });
}

void TrackGraph::addConnection(u32 fromIndex, u32 toIndex)
{
    nodes[fromIndex].connections.push_back(toIndex);
    nodes[toIndex].connections.push_back(fromIndex);
}

void TrackGraph::rebuild(glm::mat4 const& startTransform)
{
    valid = true;

    u32 startIndex = 0;
    f32 minDist = FLT_MAX;
    for (u32 i=0; i<nodes.size(); ++i)
    {
        Node const& node = nodes[i];
        f32 dist = glm::length2(node.position - translationOf(startTransform));
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

    nodes[startIndex].position = glm::vec3(glm::vec2(translationOf(startTransform)), nodes[startIndex].position.z) + xAxisOf(startTransform) * 2.f;
    u32 endIndex = (u32)nodes.size();

    // copy the start node to make the end node
    nodes.push_back(nodes[startIndex]);
    Node& end = nodes.back();
    end.connections.clear();

	Node& start = nodes[startIndex];
    for (auto c = start.connections.begin(); c != start.connections.end();)
    {
        Node& connection = nodes[*c];
        if (dot(normalize(connection.position - start.position), xAxisOf(startTransform)) <= 0.f)
        {
            for (auto o = connection.connections.begin(); o != connection.connections.end(); ++o)
            {
                if (*o == startIndex)
                {
                    connection.connections.erase(o);
                    connection.connections.push_back(endIndex);
                    break;
                }
            }
            end.connections.push_back(*c);
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

    std::vector<std::vector<u32>> nodeIndexPaths;
    for (u32 c : start.connections)
    {
        nodeIndexPaths.push_back({ startIndex });
        computePath(c, startIndex, (u32)nodeIndexPaths.size() - 1, nodeIndexPaths);
    }

    std::vector<f32> pathLengths;
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
                glm::length(nodes[(*it)[i]].position - nodes[(*it)[i-1]].position);
        }
        pathLengths.push_back(totalLength);

        ++it;
    }

    // sort paths based on length
    for (u32 i=1; i<(u32)nodeIndexPaths.size(); ++i)
    {
        for (u32 j=i; j>0 && pathLengths[j-1] > pathLengths[j]; --j)
        {
            std::swap(nodeIndexPaths[j], nodeIndexPaths[j-1]);
        }
    }

    // convert node indices to positions
    for (auto& indexPath : nodeIndexPaths)
    {
        std::vector<Node*> path;
        path.reserve(indexPath.size());
        for (u32 nodeIndex : indexPath)
        {
            path.push_back(&nodes[nodeIndex]);
        }
        paths.push_back(std::move(path));
    }

    // set node angles
    for (auto it = nodeIndexPaths.rbegin(); it != nodeIndexPaths.rend(); ++it)
    {
        for (u32 i=0; i<it->size(); ++i)
        {
            glm::vec3 fromPosition;
            glm::vec3 toPosition;
            if (i < it->size() - 1)
            {
                toPosition = nodes[(*it)[i]].position;
                fromPosition = nodes[(*it)[i+1]].position;
            }
            else
            {
                toPosition = nodes[(*it)[i-1]].position;
                fromPosition = nodes[(*it)[i]].position;
            }
            nodes[(*it)[i]].angle = pointDirection(glm::vec2(fromPosition), glm::vec2(toPosition)) - f32(M_PI) * 0.5f;
            nodes[(*it)[i]].direction = glm::normalize(fromPosition - toPosition);
        }
    }

    end.angle = start.angle;
    end.direction = start.direction;

    print("Built track graph: ", nodes.size(), " nodes, ", paths.size(), " paths\n");

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

    Mesh* arrowMesh = g_res.getMesh("world.Arrow");
    for (u32 i=0; i<nodes.size(); ++i)
    {
        Node const& c = nodes[i];

        renderer->getRenderWorld()->push(LitRenderable(arrowMesh,
                glm::translate(glm::mat4(1.f), c.position) *
                    glm::rotate(glm::mat4(1.f), c.angle, glm::vec3(0, 0, 1)) *
                    glm::scale(glm::mat4(1.f), glm::vec3(1.25f)), nullptr));

        for (u32 connection : c.connections)
        {
            dbg->line(c.position, nodes[connection].position,
                    glm::vec4(1.f, 0.6f, 0.f, 1.f), glm::vec4(1.f, 0.6f, 0.f, 1.f));
        }

#if 0
        glm::vec4 p = g_game.renderer->getRenderWorld()->getCamera(0).viewProjection *
            glm::vec4(c.position + glm::vec3(0, 0, 1) * f32(i % 2) * 2.f, 1.f);
        p.x = (((p.x / p.w) + 1.f) / 2.f) * g_game.windowWidth;
        p.y = ((-1.f * (p.y / p.w) + 1.f) / 2.f) * g_game.windowHeight;
        renderer->push2D(TextRenderable(
                    &g_res.getFont("font", 23),
                    tstr(std::fixed, std::setprecision(1), c.t), { p.x, p.y },
                    glm::vec3(0.f, 0.f, 1.f), 1.f, 1.f, HorizontalAlign::CENTER));
#endif
    }

    glm::vec4 colors[] = {
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
            glm::vec4 col = colors[pathIndex % ARRAY_SIZE(colors)];
            glm::vec3 zoffset(0, 0, pathIndex * 2);
            dbg->line(paths[pathIndex][i]->position + zoffset,
                    paths[pathIndex][i-1]->position + zoffset, col, col);
        }
    }
}

void TrackGraph::findLapDistance(glm::vec3 const& p, QueryResult& queryResult, f32 maxSkippableDistance) const
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

            glm::vec3 a = nodeA->position;
            glm::vec3 b = nodeB->position;

            glm::vec3 ap = p - a;
            glm::vec3 ab = b - a;
            f32 distanceAlongLine = glm::clamp(glm::dot(ap, ab) / glm::length2(ab), 0.f, 1.f);
            f32 t = nodeA->t + distanceAlongLine * (nodeB->t - nodeA->t);

            if (queryResult.lapDistanceLowMark - t < maxSkippableDistance)
            {
                glm::vec3 result = a + distanceAlongLine * ab;
                f32 distance = glm::length2(p - result);
                // prioritize points that don't loose progress
                if (queryResult.lapDistanceLowMark - t < 0.f)
                {
                    distance += 800.f;
                }
                if (minDistance > distance && distance < square(35))
                {
                    minDistance = distance;
                    queryResult.currentLapDistance = t;
                    queryResult.lastNode = nodeA;
                }
            }
        }
    }

    queryResult.lapDistanceLowMark = glm::min(queryResult.lapDistanceLowMark, queryResult.currentLapDistance);
}

f32 TrackGraph::findTrackProgressAtPoint(glm::vec3 const& p, f32 referenceValue) const
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

            glm::vec3 a = nodeA->position;
            glm::vec3 b = nodeB->position;

            glm::vec3 ap = p - a;
            glm::vec3 ab = b - a;
            f32 distanceAlongLine = glm::clamp(glm::dot(ap, ab) / glm::length2(ab), 0.f, 1.f);
            f32 t = nodeA->t + distanceAlongLine * (nodeB->t - nodeA->t);

            if (referenceValue - t < 150.f)
            {
                glm::vec3 result = a + distanceAlongLine * ab;
                f32 distance = glm::length2(p - result);
                // prioritize points that don't loose progress
                if (referenceValue - t < 0.f)
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
