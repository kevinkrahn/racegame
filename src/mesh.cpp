#include "mesh.h"
#include "debug_draw.h"
#include "game.h"

void Mesh::buildOctree()
{
    assert(elementSize == 3);
    if (octree) return;
    octree = std::make_unique<OctreeNode>();
    octree->aabb = aabb;
    octree->triangleIndices = indices;
    octree->subdivide(*this);
}

void Mesh::OctreeNode::debugDraw(DebugDraw* dbg, glm::mat4 const& transform, glm::vec4 const& col)
{
    if (triangleIndices.size() > 0 || children.size() > 0)
    {
        dbg->boundingBox(aabb, transform, col);
    }
    else
    {
        dbg->boundingBox(aabb, transform, glm::vec4(1, 0, 0, 1));
    }
    for (auto& child : children)
    {
        child.debugDraw(dbg, transform, glm::vec4(glm::vec3(col) * 0.7f, 1.f));
    }
}

void Mesh::OctreeNode::subdivide(Mesh const& mesh)
{
    u32 crossCount[3] = {};
    glm::vec3 center = (aabb.min + aabb.max) * 0.5f;
    for (u32 i=0; i<triangleIndices.size(); i+=3)
    {
        u32 j = triangleIndices[i+0] * mesh.stride / sizeof(f32);
        glm::vec3 v0(mesh.vertices[j+0], mesh.vertices[j+1], mesh.vertices[j+2]);
        j = triangleIndices[i+1] * mesh.stride / sizeof(f32);
        glm::vec3 v1(mesh.vertices[j+0], mesh.vertices[j+1], mesh.vertices[j+2]);
        j = triangleIndices[i+2] * mesh.stride / sizeof(f32);
        glm::vec3 v2(mesh.vertices[j+0], mesh.vertices[j+1], mesh.vertices[j+2]);

        glm::vec3 min = glm::min(glm::min(v0, v1), v2);
        glm::vec3 max = glm::max(glm::max(v0, v1), v2);
        for (u32 i=0; i<3; ++i)
        {
            if (min[i] <= center[i] && max[i] >= center[i])
            {
                ++crossCount[i];
            }
        }
    }

    SmallVec<BoundingBox> childBoxes = {
        { { aabb.min, aabb.max } }
    };
    SmallVec<BoundingBox> splits;

    const f32 MIN_SIZE = 3.f;

    u32 triCount = triangleIndices.size() / 3;
    for (u32 i=0; i<3; ++i)
    {
        // don't subdivide if the number of triangles that straddle this axis division exceeds the threshold
        if (crossCount[i] > triCount / 2)
        {
            continue;
        }

        splits.clear();
        for (auto& bb : childBoxes)
        {
            f32 diff = (bb.max[i] - bb.min[i]) * 0.5f;
            if (diff * 2.f > MIN_SIZE)
            {
                glm::vec3 dim = bb.max - bb.min;
                dim[i] *= 0.5f;
                glm::vec3 off(0.f);
                off[i] = diff;

                splits.push_back({ bb.min, bb.min + dim});
                splits.push_back({ bb.min + off, bb.min + off + dim });
            }
            else
            {
                splits.push_back({ bb.min, bb.max });
            }
        }
        childBoxes = std::move(splits);
    }
    if (childBoxes.size() == 1)
    {
        return;
    }

    children.resize(childBoxes.size());
    for (u32 i=0; i<childBoxes.size(); ++i)
    {
        children[i].aabb = childBoxes[i];
        childBoxes[i] = { glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX) };
    }

    std::vector<u32> indices(std::move(triangleIndices));
    triangleIndices = std::vector<u32>();
    for (u32 i=0; i<indices.size(); i+=3)
    {
        u32 j = indices[i+0] * mesh.stride / sizeof(f32);
        glm::vec3 v0(mesh.vertices[j+0], mesh.vertices[j+1], mesh.vertices[j+2]);
        j = indices[i+1] * mesh.stride / sizeof(f32);
        glm::vec3 v1(mesh.vertices[j+0], mesh.vertices[j+1], mesh.vertices[j+2]);
        j = indices[i+2] * mesh.stride / sizeof(f32);
        glm::vec3 v2(mesh.vertices[j+0], mesh.vertices[j+1], mesh.vertices[j+2]);

        u32 hitCount = 0;
        u32 hitIndex = 0;
        for (u32 i=0; i<children.size(); ++i)
        {
            BoundingBox const& bb = children[i].aabb;
            if (bb.intersectsTriangle(v0, v1, v2))
            {
                hitIndex = i;
                ++hitCount;
                if (hitCount > 1) break;
            }
        }

        if (hitCount == 0)
        {
            continue;
        }
        else if (hitCount > 1)
        {
            triangleIndices.push_back(indices[i+0]);
            triangleIndices.push_back(indices[i+1]);
            triangleIndices.push_back(indices[i+2]);
        }
        else
        {
            children[hitIndex].triangleIndices.push_back(indices[i+0]);
            children[hitIndex].triangleIndices.push_back(indices[i+1]);
            children[hitIndex].triangleIndices.push_back(indices[i+2]);
            childBoxes[hitIndex].min = glm::min(childBoxes[hitIndex].min, glm::min(glm::min(v0, v1), v2));
            childBoxes[hitIndex].max = glm::max(childBoxes[hitIndex].max, glm::max(glm::max(v0, v1), v2));
        }
    }

    for (u32 i=0; i<children.size(); ++i)
    {
        children[i].aabb = childBoxes[i];
        if (children[i].triangleIndices.size() > 6)
        {
            children[i].subdivide(mesh);
        }
    }

    // remove empty children
    for (auto it = children.begin(); it != children.end();)
    {
        it = it->triangleIndices.size() == 0 ? children.erase(it) : it + 1;
    }
}

bool Mesh::OctreeNode::intersect(Mesh const& mesh, glm::mat4 const& transform, BoundingBox const& bb, std::vector<u32>& output) const
{
    if (aabb.intersects(bb))
    {
        for (u32 index : triangleIndices)
        {
            output.push_back(index);
        }
        for(auto const& child : children)
        {
            child.intersect(mesh, transform, bb, output);
        }
        return true;
    }
    return false;
}

bool Mesh::intersect(glm::mat4 const& transform, BoundingBox bb, std::vector<u32>& output) const
{
    bb = bb.transform(glm::inverse(transform));
    if (octree)
    {
        return octree->intersect(*this, transform, bb, output);
    }
    else
    {
        if (aabb.intersects(bb))
        {
            for (auto index : indices)
            {
                output.push_back(index);
            }
            return true;
        }
        return false;
    }
}

void Mesh::createVAO()
{
    assert(vertexFormat.size() > 0);

    destroy();

    glCreateVertexArrays(1, &vao);

    glCreateBuffers(1, &vbo);
    glNamedBufferData(vbo, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STATIC_DRAW);
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, stride);

    glCreateBuffers(1, &ebo);
    glNamedBufferData(ebo, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
    glVertexArrayElementBuffer(vao, ebo);

    u32 offset = 0;
    for (u32 i=0; i<vertexFormat.size(); ++i)
    {
        u32 numElements = 0;
        switch (vertexFormat[i])
        {
            case VertexAttribute::FLOAT1:
                numElements = 1;
                break;
            case VertexAttribute::FLOAT2:
                numElements = 2;
                break;
            case VertexAttribute::FLOAT3:
                numElements = 3;
                break;
            case VertexAttribute::FLOAT4:
                numElements = 4;
                break;
            default:
                FATAL_ERROR("Invalid vertex format");
                break;
        }
        u32 bindIndex = i;
        glEnableVertexArrayAttrib(vao, bindIndex);
        glVertexArrayAttribFormat(vao, bindIndex, numElements, GL_FLOAT, GL_FALSE, offset);
        glVertexArrayAttribBinding(vao, bindIndex, 0);
        offset += numElements * sizeof(f32);
    }
    formatStride = offset;
}

PxTriangleMesh* Mesh::getCollisionMesh()
{
    if (collisionMesh)
    {
        return collisionMesh;
    }

    assert(elementSize == 3);

    PxTriangleMeshDesc desc;
    desc.points.count = numVertices;
    desc.points.stride = stride;
    desc.points.data = vertices.data();
    desc.triangles.count = numIndices / 3;
    desc.triangles.stride = 3 * sizeof(indices[0]);
    desc.triangles.data = indices.data();

    PxDefaultMemoryOutputStream writeBuffer;
    PxTriangleMeshCookingResult::Enum result;
    if (!g_game.physx.cooking->cookTriangleMesh(desc, writeBuffer, &result))
    {
        FATAL_ERROR("Failed to create collision mesh: ", name);
    }

    PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
    collisionMesh = g_game.physx.physics->createTriangleMesh(readBuffer);
    return collisionMesh;
}

PxConvexMesh* Mesh::getConvexCollisionMesh()
{
    if (convexCollisionMesh)
    {
        return convexCollisionMesh;
    }

    assert(elementSize == 3);

    PxConvexMeshDesc convexDesc;
    convexDesc.points.count  = numVertices;
    convexDesc.points.stride = stride;
    convexDesc.points.data   = vertices.data();
    convexDesc.flags         = PxConvexFlag::eCOMPUTE_CONVEX;

    PxDefaultMemoryOutputStream writeBuffer;
    if (!g_game.physx.cooking->cookConvexMesh(convexDesc, writeBuffer))
    {
        FATAL_ERROR("Failed to create convex collision mesh: ", name);
    }

    PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
    convexCollisionMesh = g_game.physx.physics->createConvexMesh(readBuffer);
    return convexCollisionMesh;
}

void Mesh::destroy()
{
    if (vao)
    {
        glDeleteBuffers(0, &vbo);
        glDeleteBuffers(0, &ebo);
        glDeleteVertexArrays(0, &vao);
        vao = 0;
        vbo = 0;
        ebo = 0;
    }
    if (collisionMesh)
    {
        collisionMesh->release();
        collisionMesh = nullptr;
    }
    if (convexCollisionMesh)
    {
        convexCollisionMesh->release();
        convexCollisionMesh = nullptr;
    }
    octree.reset();
}
