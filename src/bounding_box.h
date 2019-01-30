#pragma once

#include "misc.h"
#include "math.h"

struct BoundingBox
{
    glm::vec3 min;
    glm::vec3 max;

    bool contains(BoundingBox const& bb) const
    {
        return bb.min.x >= min.x && bb.min.y >= min.y && bb.min.z >= min.z &&
               bb.max.x <= max.x && bb.max.y <= max.y && bb.max.z <= max.z;
    }

    bool containsPoint(glm::vec3 const p) const
    {
        return p.x >= min.x && p.y >= min.y && p.z >= min.z &&
               p.x <= max.x && p.y <= max.y && p.z <= max.z;
    }

    // Adapted from http://fileadmin.cs.lth.se/cs/Personal/Tomas_Akenine-Moller/code/
    bool intersectsTriangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2) const
    {
        glm::vec3 boxcenter = (min + max) * 0.5f;
        glm::vec3 boxhalfsize = (max - min) * 0.5f;

        v0 -= boxcenter;
        v1 -= boxcenter;
        v2 -= boxcenter;
        glm::vec3 e0 = v1 - v0;
        glm::vec3 e1 = v2 - v1;
        glm::vec3 e2 = v0 - v2;

        f32 p0, p1, p2, rad, min, max;

        p0 = e0.z * v0.y - e0.y * v0.z;
        p2 = e0.z * v2.y - e0.y * v2.z;
        min = glm::min(p0, p2);
        max = glm::min(p0, p2);
        rad = glm::abs(e0.z) * boxhalfsize.y + glm::abs(e0.y) * boxhalfsize.z;
        if (min > rad || max < -rad) return false;

        p0 = -e0.z * v0.x + e0.x * v0.z;
        p2 = -e0.z * v2.x + e0.x * v2.z;
        min = glm::min(p0, p2);
        max = glm::min(p0, p2);
        rad = glm::abs(e0.z) * boxhalfsize.x + glm::abs(e0.x) * boxhalfsize.z;
        if (min > rad || max < -rad) return false;

        p1 = e0.y * v1.x - e0.x * v1.y;
        p2 = e0.y * v2.x - e0.x * v2.y;
        min = glm::min(p0, p2);
        max = glm::min(p0, p2);
        rad = glm::abs(e0.y) * boxhalfsize.x + glm::abs(e0.x) * boxhalfsize.y;
        if (min > rad || max < -rad) return false;

        p0 = e1.z * v0.y - e1.y * v0.z;
        p2 = e1.z * v2.y - e1.y * v2.z;
        min = glm::min(p0, p2);
        max = glm::min(p0, p2);
        rad = glm::abs(e1.z) * boxhalfsize.y + glm::abs(e1.y) * boxhalfsize.z;
        if (min > rad || max < -rad) return false;

        p0 = -e1.z * v0.x + e1.x * v0.z;
        p2 = -e1.z * v2.x + e1.x * v2.z;
        min = glm::min(p0, p2);
        max = glm::min(p0, p2);
        rad = glm::abs(e1.z) * boxhalfsize.x + glm::abs(e1.x) * boxhalfsize.z;
        if (min > rad || max < -rad) return false;

        p0 = e1.y * v0.x - e1.x * v0.y;
        p1 = e1.y * v1.x - e1.x * v1.y;
        min = glm::min(p0, p1);
        max = glm::min(p0, p1);
        rad = glm::abs(e1.y) * boxhalfsize.x + glm::abs(e1.x) * boxhalfsize.y;
        if (min > rad || max < -rad) return false;

        p0 = e2.z * v0.y - e2.y * v0.z;
        p1 = e2.z * v1.y - e2.y * v1.z;
        min = glm::min(p0, p1);
        max = glm::min(p0, p1);
        rad = glm::abs(e2.z) * boxhalfsize.y + glm::abs(e2.y) * boxhalfsize.z;
        if (min > rad || max < -rad) return false;

        p0 = -e2.z * v0.x + e2.x * v0.z;
        p1 = -e2.z * v1.x + e2.x * v1.z;
        min = glm::min(p0, p1);
        max = glm::min(p0, p1);
        rad = glm::abs(e2.z) * boxhalfsize.x + glm::abs(e2.x) * boxhalfsize.z;
        if (min > rad || max < -rad) return false;

        p1 = e2.y * v1.x - e2.x * v1.y;
        p2 = e2.y * v2.x - e2.x * v2.y;
        min = glm::min(p1, p2);
        max = glm::min(p1, p2);
        rad = glm::abs(e2.y) * boxhalfsize.x + glm::abs(e2.x) * boxhalfsize.y;
        if (min > rad || max < -rad) return false;

        glm::vec3 omin = glm::min(glm::min(v0, v1), v2);
        glm::vec3 omax = glm::max(glm::max(v0, v1), v2);
        if ((omin.x > boxhalfsize.x || omax.x < -boxhalfsize.x) ||
            (omin.y > boxhalfsize.y || omax.y < -boxhalfsize.y) ||
            (omin.z > boxhalfsize.z || omax.z < -boxhalfsize.z)) return false;

        glm::vec3 normal = glm::cross(e0, e1);
        glm::vec3 vmin, vmax;
        for (u32 q = 0; q < 3; ++q)
        {
            f32 v = v0[q];
            if (normal[q] > 0.f)
            {
                vmin[q] = -boxhalfsize[q] - v;
                vmax[q] = boxhalfsize[q] - v;
            }
            else
            {
                vmin[q] = boxhalfsize[q] - v;
                vmax[q] = -boxhalfsize[q] - v;
            }
        }

        if (!(glm::dot(normal, vmin) <= 0.f || glm::dot(normal, vmax) >= 0.f)) return false;

        return true;
    }

    bool intersects(BoundingBox const& bb) const
    {
        return min.x <= bb.max.x && max.x >= bb.min.x &&
               min.y <= bb.max.y && max.y >= bb.min.y &&
               min.z <= bb.max.z && max.z >= bb.min.z;
    }

    BoundingBox transform(glm::mat4 const& transform) const
    {
        glm::vec3 minP(FLT_MAX), maxP(-FLT_MAX);
        glm::vec3 corners[8] = {
            { min.x, min.y, min.z },
            { max.x, min.y, min.z },
            { max.x, max.y, min.z },
            { min.x, max.y, min.z },
            { min.x, min.y, max.z },
            { max.x, min.y, max.z },
            { max.x, max.y, max.z },
            { min.x, max.y, max.z },
        };
        for (glm::vec3& v : corners)
        {
            v = glm::vec3(transform * glm::vec4(v, 1.0));
            if (v.x < minP.x) minP.x = v.x;
            if (v.y < minP.y) minP.y = v.y;
            if (v.z < minP.z) minP.z = v.z;
            if (v.x > maxP.x) maxP.x = v.x;
            if (v.y > maxP.y) maxP.y = v.y;
            if (v.z > maxP.z) maxP.z = v.z;
        }
        return { minP, maxP };
    }

    BoundingBox growToFit(BoundingBox const& other) const
    {
        return {
            {
                glm::min(other.min.x, min.x),
                glm::min(other.min.y, min.y),
                glm::min(other.min.z, min.z)
            },
            {
                glm::max(other.max.x, max.x),
                glm::max(other.max.y, max.y),
                glm::max(other.max.z, max.z)
            }
        };
    }

    f32 volume() const
    {
        glm::vec3 dim = max - min;
        return dim.x * dim.y * dim.z;
    }
};

BoundingBox computeCameraFrustumBoundingBox(glm::mat4 const& viewProj)
{
    glm::vec3 ndc[] = {
        { -1,  1, 0 },
        {  1,  1, 0 },
        {  1, -1, 0 },
        { -1, -1, 0 },

        { -1,  1, 1 },
        {  1,  1, 1 },
        {  1, -1, 1 },
        { -1, -1, 1 },
    };

    glm::mat4 m = glm::inverse(viewProj);

    f32 minx =  FLT_MAX;
    f32 maxx = -FLT_MAX;
    f32 miny =  FLT_MAX;
    f32 maxy = -FLT_MAX;
    f32 minz =  FLT_MAX;
    f32 maxz = -FLT_MAX;
    for (auto& v : ndc)
    {
        glm::vec4 b = m * glm::vec4(v, 1.f);
        v = glm::vec3(b) / b.w;

        if (v.x < minx) minx = v.x;
        if (v.x > maxx) maxx = v.x;
        if (v.y < miny) miny = v.y;
        if (v.y > maxy) maxy = v.y;
        if (v.z < minz) minz = v.z;
        if (v.z > maxz) maxz = v.z;
    }

    return BoundingBox{ { minx, miny, minz }, { maxx, maxy, maxz } };
}
