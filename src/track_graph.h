#pragma once

#include "math.h"
#include "resources.h"

class TrackGraph
{
public:
    struct Node
    {
        Vec3 position = {};
        f32 t = FLT_MAX;
        Vec3 direction = {};
        f32 angle = 0.f;
        SmallArray<u32, 4> connections;
    };

private:
    Array<Node> nodes;
    Node* startNode = nullptr;
    Node* endNode = nullptr;
    bool valid = false;

    void computeTravelTime(u32 toIndex, u32 fromIndex, u32 endIndex);
    void computePath(u32 toIndex, u32 fromIndex, u32 pathIndex,
            Array<Array<u32>>& nodeIndexPaths);

    Array<Array<Node*>> paths;
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
    u32 addNode(Vec3 const& position);
    void addConnection(u32 fromIndex, u32 toIndex);
    void rebuild(Mat4 const& startTransform);

    Node* getStartNode() const { return startNode; }
    Node* getEndNode() const { return endNode; }
    Node* getNode(u32 index) { return &nodes[index]; }
    u32 getNodeCount() const { return (u32)nodes.size(); }
    u32 getStartNodeIndex() const { return startNode - nodes.data(); }
    u32 getEndNodeIndex() const { return (u32)nodes.size() - 1; }

    void debugDraw(class DebugDraw* dbg, class Renderer* renderer) const;

    struct QueryResult
    {
        const Node* lastNode = nullptr;
        f32 currentLapDistance = 0.f;
        f32 lapDistanceLowMark = 0.f;
        Vec3 position = { 0, 0, 0 };
    };

    f32 findTrackProgressAtPoint(Vec3 const& p, f32 referenceValue) const;
    void findLapDistance(Vec3 const& p, QueryResult& queryResult, f32 maxSkippableDistance) const;
    bool isValid() const { return valid; }

    Array<Array<Node*>> const& getPaths() const { return paths; }
};
