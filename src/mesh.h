#pragma once

#include "misc.h"
#include "math.h"
#include "smallvec.h"
#include "bounding_box.h"
#include "gl.h"
#include "datafile.h"
#include <vector>

enum struct VertexAttributeType
{
    FLOAT1,
    FLOAT2,
    FLOAT3,
    FLOAT4
};

struct VertexAttribute
{
    u32 binding;
    VertexAttributeType type;
};

struct Mesh
{
    std::string name;
    std::vector<f32> vertices;
    std::vector<u32> indices;
    u32 numVertices;
    u32 numIndices;
    u32 numColors;
    u32 numTexCoords;
    BoundingBox aabb;

    u32 stride = 0;
    SmallVec<VertexAttribute> vertexFormat;

    // TODO: remove this property when all meshes have been reimported
    bool hasTangents = false;

    void serialize(Serializer& s)
    {
        s.field(name);
        s.field(vertices);
        s.field(indices);
        s.field(numVertices);
        s.field(numIndices);
        s.field(numColors);
        s.field(numTexCoords);
        s.field(aabb);
        s.field(hasTangents);

        if (s.deserialize)
        {
            calculateVertexFormat();
            createVAO();
        }
    }

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
    void calculateVertexFormat();

    bool intersect(glm::mat4 const& transform, BoundingBox bb, std::vector<u32>& output) const;
    void createVAO();

    PxTriangleMesh* collisionMesh = nullptr;
    PxConvexMesh* convexCollisionMesh = nullptr;

    PxTriangleMesh* getCollisionMesh();
    PxConvexMesh* getConvexCollisionMesh();

    void destroy();
};

inline std::ostream& operator << (std::ostream& lhs, Mesh const& rhs)
{
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec4 tangent;
        glm::vec2 uv;
        glm::vec3 color;
    };
    for (u32 i=0; i<rhs.numVertices; ++i)
    {
        Vertex v = *((Vertex*)(((u8*)rhs.vertices.data()) + i * rhs.stride));
        lhs << "POS:    " << v.pos << '\n';
        lhs << "NORMAL: " << v.normal << '\n';
        lhs << "TANGENT: " << v.tangent << '\n';
        lhs << "COLOR:  " << v.color << '\n';
        lhs << "UV:     " << v.uv << '\n';
    }
    return lhs;
}

