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
    bool valid = false;

    void computeTravelTime(u32 toIndex, u32 fromIndex, u32 endIndex);
    void computePath(u32 toIndex, u32 fromIndex, u32 pathIndex,
            std::vector<std::vector<u32>>& nodeIndexPaths);

    std::vector<std::vector<Node*>> paths;
    void computePaths();

public:
    TrackGraph() {}

    void clear()
    {
        paths.clear();
        nodes.clear();
        startNode = nullptr;
        endNode = nullptr;
    }
    void addNode(glm::vec3 const& position);
    void addConnection(u32 fromIndex, u32 toIndex);
    void rebuild(glm::mat4 const& startTransform);

    Node* getStartNode() const { return startNode; }
    Node* getEndNode() const { return endNode; }

    void debugDraw(class DebugDraw* dbg, class Renderer* renderer) const;

    struct QueryResult
    {
        const Node* lastNode = nullptr;
        f32 currentLapDistance = 0.f;
        f32 lapDistanceLowMark = 0.f;
    };

    f32 findTrackProgressAtPoint(glm::vec3 const& p, f32 referenceValue) const;
    void findLapDistance(glm::vec3 const& p, QueryResult& queryResult, f32 maxSkippableDistance) const;
    bool isValid() const { return valid; }

    std::vector<std::vector<Node*>> const& getPaths() const { return paths; }
};
