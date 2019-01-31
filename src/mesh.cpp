#include "mesh.h"
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

void Mesh::OctreeNode::debugDraw(glm::mat4 const& transform, glm::vec4 const& col)
{
    if (triangleIndices.size() > 0 || children.size() > 0)
        game.renderer.drawBoundingBox(aabb, transform, col);
    else
        game.renderer.drawBoundingBox(aabb, transform, glm::vec4(1, 0, 0, 1));
    for (auto& child : children)
    {
        child->debugDraw(transform, glm::vec4(glm::vec3(col) * 0.7f, 1.f));
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

    SmallVec<OctreeNode> potentialChildren;
    potentialChildren.push_back({ { aabb.min, aabb.max } });

    const f32 minSize = 2.f;

    u32 triCount = triangleIndices.size() / 3;
    for (u32 i=0; i<3; ++i)
    {
        // don't subdivide if the number of triangles that straddle this axis division exceeds the threshold
        if (crossCount[i] > triCount / 2)
        {
            continue;
        }

        SmallVec<OctreeNode> splits;
        for (auto& node : potentialChildren)
        {
            f32 diff = (node.aabb.max[i] - node.aabb.min[i]) * 0.5f;
            if (diff * 2.f > minSize)
            {
                glm::vec3 dim = node.aabb.max - node.aabb.min;
                dim[i] *= 0.5f;
                glm::vec3 off(0.f);
                off[i] = diff;

                splits.push_back(OctreeNode{ { node.aabb.min, node.aabb.min + dim} });
                splits.push_back(OctreeNode{ { node.aabb.min + off, node.aabb.min + off + dim } });
            }
            else
            {
                splits.push_back(OctreeNode{ { node.aabb.min, node.aabb.max } });
            }
        }
        potentialChildren = std::move(splits);
    }
    if (potentialChildren.size() == 1)
    {
        return;
    }

    std::vector<u32> indices(std::move(triangleIndices));
    triangleIndices = std::vector<u32>();
    BoundingBox shrinkBB[8] = {};
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
        for (u32 i=0; i<potentialChildren.size(); ++i)
        {
            BoundingBox const& bb = potentialChildren[i].aabb;
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
            potentialChildren[hitIndex].triangleIndices.push_back(indices[i+0]);
            potentialChildren[hitIndex].triangleIndices.push_back(indices[i+1]);
            potentialChildren[hitIndex].triangleIndices.push_back(indices[i+2]);
            shrinkBB[hitIndex].min = glm::min(shrinkBB[hitIndex].min, glm::min(glm::min(v0, v1), v2));
            shrinkBB[hitIndex].max = glm::max(shrinkBB[hitIndex].max, glm::max(glm::max(v0, v1), v2));
        }
    }

    for (u32 i=0; i<potentialChildren.size(); ++i)
    {
        OctreeNode& node = potentialChildren[i];
        if (node.triangleIndices.size() > 0)
        {
            // TODO: make this work
            // node.aabb = shrinkBB[i];
            children.push_back(std::make_unique<OctreeNode>(std::move(node)));
            if (children.back()->triangleIndices.size() > 6 && children.back()->aabb.volume() > 4.f)
            {
                children.back()->subdivide(mesh);
            }
        }
    }
}

bool Mesh::OctreeNode::intersect(Mesh const& mesh, glm::mat4 const& transform, BoundingBox const& bb, std::vector<u32>& output) const
{
    if (aabb.intersects(bb))
    {
        game.renderer.drawBoundingBox(aabb, glm::translate(glm::mat4(1.f), { 0, 0, 0.2f }) * transform, glm::vec4(1, 0, 0, 1));
        for (u32 index : triangleIndices)
        {
            output.push_back(index);
        }
        for(auto const& child : children)
        {
            child->intersect(mesh, transform, bb, output);
        }
        return true;
    }
    return false;
}

bool Mesh::intersect(glm::mat4 const& transform, BoundingBox bb, std::vector<u32>& output) const
{
    bb = bb.transform(glm::inverse(transform));
    return octree->intersect(*this, transform, bb, output);
}
