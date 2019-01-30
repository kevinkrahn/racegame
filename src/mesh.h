#pragma once

#include "misc.h"
#include "math.h"
#include "smallvec.h"
#include "bounding_box.h"
#include <vector>

struct Mesh
{
    std::vector<f32> vertices;
    std::vector<u32> indices;
    u32 numVertices;
    u32 numIndices;
    u32 numColors;
    u32 numTexCoords;
    u32 elementSize;
    u32 stride;
    u32 renderHandle;
    BoundingBox aabb;

    struct OctreeNode
    {
        BoundingBox aabb;
        SmallVec<std::unique_ptr<OctreeNode>, 8> children;
        std::vector<u32> triangleIndices;

        void subdivide(Mesh const& mesh);
        void debugDraw(glm::mat4 const& transform, glm::vec4 const& col=glm::vec4(1.f));
    };

    std::unique_ptr<OctreeNode> octree = nullptr;
    void buildOctree();
};
