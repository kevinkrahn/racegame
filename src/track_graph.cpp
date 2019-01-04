#include "track_graph.h"
#include "game.h"

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
        Node& from = nodes[index];
        for (u32 i=0; i<from.connections.size(); ++i)
        {
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

TrackGraph::TrackGraph(glm::mat4 const& startTransform, Mesh const& mesh, glm::mat4 const& transform)
{
    if (mesh.elementSize != 2)
    {
        FATAL_ERROR("Track graph mesh must be an edge mesh.");
    }

    u32 startIndex;
    f32 minDist = FLT_MAX;
    for (u32 i=0; i<mesh.numVertices; ++i)
    {
        const f32* data = mesh.vertices.data();
        glm::vec3 position = transform * glm::vec4(data[i*3], data[i*3+1], data[i*3+2], 1.f);
        f32 dist = glm::length2(position - translationOf(startTransform));
        if (dist < minDist)
        {
            minDist = dist;
            startIndex = i;
        }
        nodes.push_back({ position });
    }

    if (minDist > square(50.f))
    {
        FATAL_ERROR("There is no track graph vertex close enough to the finish line.");
    }

    u32 size = mesh.indices.size() / 2;
    for (u32 i=0; i<size; ++i)
    {
        u32 index1 = mesh.indices[i * 2];
        u32 index2 = mesh.indices[i * 2 + 1];
        nodes[index1].connections.push_back(index2);
        nodes[index2].connections.push_back(index1);
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

        node.angle = pointDirection(glm::vec2(b->position), glm::vec2(a->position)) - M_PI * 0.5f;
        node.direction = glm::normalize(b->position - a->position);
    }

    subdivide();

    startNode = &nodes[startIndex];
    endNode = &nodes[endIndex];
}

void TrackGraph::debugDraw() const
{
    u32 arrow = game.resources.getMesh("world.Arrow").renderHandle;
    for (u32 i=0; i<nodes.size(); ++i)
    {
        Node const& c = nodes[i];

        game.renderer.drawMesh(arrow,
                glm::translate(glm::mat4(1.f), c.position) *
                    glm::rotate(glm::mat4(1.f), c.angle, glm::vec3(0, 0, 1)) *
                    glm::scale(glm::mat4(1.f), glm::vec3(1.25f)));

        for (u32 connection : c.connections)
        {
            game.renderer.drawLine(c.position, nodes[connection].position,
                    glm::vec4(1.f, 0.6f, 0.f, 1.f), glm::vec4(1.f, 0.6f, 0.f, 1.f));
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
                if (minDistance > distance && distance < square(35))
                {
                    minDistance = distance;
                    queryResult.currentLapDistance = t;
                    queryResult.lastNode = nodeA;
                }
            }
        }
    }

    queryResult.lapDistanceLowMark = std::min(queryResult.lapDistanceLowMark, queryResult.currentLapDistance);
}
