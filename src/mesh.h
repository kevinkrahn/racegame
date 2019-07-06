#pragma once

#include "misc.h"
#include "math.h"
#include "smallvec.h"
#include "bounding_box.h"
#include "gl.h"
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
    BoundingBox aabb;

    enum VertexAttribute
    {
        FLOAT1,
        FLOAT2,
        FLOAT3,
        FLOAT4
    };

    SmallVec<float> vertexFormat = {
        VertexAttribute::FLOAT3,
        VertexAttribute::FLOAT3,
        VertexAttribute::FLOAT3,
        VertexAttribute::FLOAT2,
    };

    GLuint vao = 0, vbo = 0, ebo = 0;

    struct OctreeNode
    {
        BoundingBox aabb;
        std::vector<OctreeNode> children;
        std::vector<u32> triangleIndices;

        void subdivide(Mesh const& mesh);
        void debugDraw(class DebugDraw* dbg, glm::mat4 const& transform, glm::vec4 const& col=glm::vec4(1.f));
        bool intersect(Mesh const& mesh, glm::mat4 const& transform, BoundingBox const& bb, std::vector<u32>& output) const;
    };

    std::unique_ptr<OctreeNode> octree = nullptr;
    void buildOctree();

    bool intersect(glm::mat4 const& transform, BoundingBox bb, std::vector<u32>& output) const;
    void createVAO();
};
