#pragma once

#include "misc.h"
#include "math.h"
#include "bounding_box.h"
#include "gl.h"
#include "datafile.h"

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
    Array<f32> vertices;
    Array<u32> indices;

    // TODO: remove these and only store vertexFormat
    u32 numVertices;
    u32 numIndices;
    u32 numColors;
    u32 numTexCoords;
    BoundingBox aabb;

    u32 stride = 0;
    SmallArray<VertexAttribute> vertexFormat;

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
        Array<OctreeNode> children;
        Array<u32> triangleIndices;

        void subdivide(Mesh const& mesh);
        void debugDraw(class DebugDraw* dbg, Mat4 const& transform, Vec4 const& col=Vec4(1.f));
        bool intersect(Mesh const& mesh, Mat4 const& transform, BoundingBox const& bb, Array<u32>& output) const;
    };

    OwnedPtr<OctreeNode> octree = nullptr;
    void buildOctree();
    void calculateVertexFormat();
    void computeBoundingBox();

    bool intersect(Mat4 const& transform, BoundingBox bb, Array<u32>& output) const;
    void createVAO();

    PxTriangleMesh* collisionMesh = nullptr;
    PxConvexMesh* convexCollisionMesh = nullptr;

    PxTriangleMesh* getCollisionMesh();
    PxConvexMesh* getConvexCollisionMesh();

    void destroy();
};

#if 0
inline std::ostream& operator << (std::ostream& lhs, Mesh const& rhs)
{
    struct Vertex
    {
        Vec3 pos;
        Vec3 normal;
        Vec4 tangent;
        Vec2 uv;
        Vec3 color;
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
#endif
