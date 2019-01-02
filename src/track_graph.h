#pragma once

#include "math.h"
#include "resources.h"
#include "smallvec.h"

class TrackGraph
{
public:
    struct Node
    {
        glm::vec3 position = {};
        f32 t = FLT_MAX;
        glm::vec3 direction = {};
        f32 angle = 0.f;
        SmallVec<u32, 4> connections;
    };

private:
    std::vector<Node> nodes;
    Node* startNode = nullptr;
    Node* endNode = nullptr;

    void computeTravelTime(u32 toIndex, u32 fromIndex, u32 endIndex);
    void subdivide();

public:
    TrackGraph() {}
    TrackGraph(glm::mat4 const& startTransform, Mesh const& mesh, glm::mat4 const& transform);

    Node* getStartNode() const { return startNode; }
    Node* getEndNode() const { return endNode; }

    void debugDraw() const;

    struct QueryResult
    {
        const Node* lastNode = nullptr;
        f32 currentLapDistance = 0.f;
        f32 lapDistanceLowMark = 0.f;
    };

    QueryResult findLapDistance(glm::vec3 const& p, f32 currentLapDistance,
            f32 lapDistanceLowMark, f32 maxSkippableDistance) const;
};
