#include "mesh.h"
#include "game.h"

void Mesh::buildOctree()
{
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
    glm::vec3 dim = (aabb.max - aabb.min) * 0.5f;
    glm::vec3 dimx(dim.x, 0, 0);
    glm::vec3 dimy(0, dim.y, 0);
    glm::vec3 dimz(0, 0, dim.z);
    OctreeNode potentialChildren[8] = {
        { aabb.min, aabb.min + dim },
        { aabb.min + dimx, aabb.min + dimx + dim },
        { aabb.min + dimy, aabb.min + dimy + dim },
        { aabb.min + dimx + dimy, aabb.min + dimx + dimy + dim },
        { aabb.min + dimz, aabb.min + dim },
        { aabb.min + dimz + dimx, aabb.min + dimz + dimx + dim },
        { aabb.min + dimz + dimy, aabb.min + dimz + dimy + dim },
        { aabb.min + dimz + dimx + dimy, aabb.min + dimz + dimx + dimy + dim },
    };

    std::vector<u32> indices(std::move(triangleIndices));
    triangleIndices.clear();
    for (u32 i=0; i<indices.size(); i+=3)
    {
        u32 j = indices[i+0] * mesh.stride / sizeof(f32);
        glm::vec3 v1(mesh.vertices[j+0], mesh.vertices[j+1], mesh.vertices[j+2]);
        j = indices[i+1] * mesh.stride / sizeof(f32);
        glm::vec3 v2(mesh.vertices[j+0], mesh.vertices[j+1], mesh.vertices[j+2]);
        j = indices[i+2] * mesh.stride / sizeof(f32);
        glm::vec3 v3(mesh.vertices[j+0], mesh.vertices[j+1], mesh.vertices[j+2]);

        u32 hitCount = 0;
        u32 hitIndex = 0;
        for (u32 i=0; i<8; ++i)
        {
            BoundingBox const& bb = potentialChildren[i].aabb;
            if (bb.intersectsTriangle(v1, v2, v3))
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
        }
    }

    for (OctreeNode& node : potentialChildren)
    {
        if (node.triangleIndices.size() > 0)
        {
            children.push_back(std::make_unique<OctreeNode>(std::move(node)));
            if (children.back()->triangleIndices.size() > 4 && children.back()->aabb.volume() > 4.f)
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
        //if (triangleIndices.size() > 0) game.renderer.drawBoundingBox(aabb, glm::translate(glm::mat4(1.f), { 0, 0, 0.2f }) * transform, glm::vec4(1, 0, 0, 1));
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