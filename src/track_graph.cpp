#include "track_graph.h"
#include "game.h"
#include "renderer.h"
#include "resources.h"
#include "debug_draw.h"
#include "font.h"
#include "track.h"
#include "mesh_renderables.h"
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

    for (u32 c : to.connections)
    {
        if (c != fromIndex)
        {
            computeTravelTime(c, toIndex, endIndex);
        }
    }
}

void TrackGraph::subdivide()
{
    const f32 maxDistance = square(60.f);
    for (u32 index=0; index<nodes.size(); ++index)
    {
        for (u32 i=0; i<nodes[index].connections.size(); ++i)
        {
            Node& from = nodes[index];
            u32 next = from.connections[i];
            Node& to = nodes[next];
            if (glm::length2(from.position - to.position) > maxDistance)
            {
                Node middle = {
                    glm::lerp(from.position, to.position, 0.5f),
                    glm::lerp(from.t, to.t, 0.5f),
                    from.t > to.t ? from.direction : to.direction,
                    from.t > to.t ? from.angle : to.angle
                };
                middle.connections.push_back(next);
                middle.connections.push_back(index);

                for (u32 j=0; j<to.connections.size(); ++j)
                {
                    if (to.connections[j] == index)
                    {
                        to.connections[j] = nodes.size();
                        break;
                    }
                }

                from.connections[i] = nodes.size();
                nodes.push_back(middle);
            }
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
    u32 startIndex;
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
    }

    Node& start = nodes[startIndex];
    start.position = glm::vec3(glm::vec2(translationOf(startTransform)), start.position.z) + xAxisOf(startTransform) * 2.f;
    u32 endIndex = nodes.size();

    // copy the start node to make the end node
    nodes.push_back(start);
    Node& end = nodes.back();
    end.connections.clear();

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
    }

    end.t = 0.f;
    for (u32 c : end.connections)
    {
        computeTravelTime(c, endIndex, startIndex);
    }

    // compute checkpoint angles
    for (Node& node : nodes)
    {
        if (node.connections.empty())
        {
            continue;
        }

        Node* min = &nodes[node.connections.front()];
        for (auto const& connection : node.connections)
        {
            if (nodes[connection].t < min->t)
            {
                min = &nodes[connection];
            }
        }

        Node* a = min;
        Node* b = &node;
        if (a->t < b->t)
        {
            b = a;
            a = &node;
        }

        node.angle = pointDirection(glm::vec2(b->position), glm::vec2(a->position)) - f32(M_PI) * 0.5f;
        node.direction = glm::normalize(b->position - a->position);
    }

    //subdivide();

    startNode = &nodes[startIndex];
    endNode = &nodes[endIndex];
}

void TrackGraph::debugDraw(DebugDraw* dbg, Renderer* renderer) const
{
    Mesh* arrowMesh = g_resources.getMesh("world.Arrow");
    for (u32 i=0; i<nodes.size(); ++i)
    {
        Node const& c = nodes[i];

        renderer->push(LitRenderable(arrowMesh,
                glm::translate(glm::mat4(1.f), c.position) *
                    glm::rotate(glm::mat4(1.f), c.angle, glm::vec3(0, 0, 1)) *
                    glm::scale(glm::mat4(1.f), glm::vec3(1.25f)), nullptr));

        for (u32 connection : c.connections)
        {
            dbg->line(c.position, nodes[connection].position,
                    glm::vec4(1.f, 0.6f, 0.f, 1.f), glm::vec4(1.f, 0.6f, 0.f, 1.f));
        }

        glm::vec4 p = g_game.renderer->getCamera(0).viewProjection *
            glm::vec4(c.position + glm::vec3(0, 0, 1) * f32(i % 2) * 2.f, 1.f);
        p.x = (((p.x / p.w) + 1.f) / 2.f) * g_game.windowWidth;
        p.y = ((-1.f * (p.y / p.w) + 1.f) / 2.f) * g_game.windowHeight;
        /*
        g_resources.getFont("font", 23).drawText(str(std::fixed, std::setw(1), c.t).c_str(), { p.x, p.y },
                glm::vec3(0.f, 0.f, 1.f), 1.f, 1.f, HorizontalAlign::CENTER);
                */
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
