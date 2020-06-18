#pragma once

#include "misc.h"
#include "math.h"
#include "datafile.h"

struct BoundingBox
{
    Vec3 min;
    Vec3 max;

    void serialize(Serializer& s)
    {
        s.field(min);
        s.field(max);
    }

    bool contains(BoundingBox const& bb) const
    {
        return bb.min.x >= min.x && bb.min.y >= min.y && bb.min.z >= min.z &&
               bb.max.x <= max.x && bb.max.y <= max.y && bb.max.z <= max.z;
    }

    bool containsPoint(Vec3 const p) const
    {
        return p.x >= min.x && p.y >= min.y && p.z >= min.z &&
               p.x <= max.x && p.y <= max.y && p.z <= max.z;
    }

    bool intersects(BoundingBox const& bb) const
    {
        return min.x <= bb.max.x && max.x >= bb.min.x &&
               min.y <= bb.max.y && max.y >= bb.min.y &&
               min.z <= bb.max.z && max.z >= bb.min.z;
    }

    // Adapted from http://fileadmin.cs.lth.se/cs/Personal/Tomas_Akenine-Moller/code/
    bool triBoxOverlap(Vec3 boxcenter, Vec3 const& boxhalfsize, Vec3 triverts[3]) const
    {
        Vec3 v0 = triverts[0] - boxcenter;
        Vec3 v1 = triverts[1] - boxcenter;
        Vec3 v2 = triverts[2] - boxcenter;

        Vec3 e0 = v1 - v0;
        Vec3 e1 = v2 - v1;
        Vec3 e2 = v0 - v2;

        f32 min, max, p0, p1, p2, rad;

        Vec3 fe = absolute(e0);
        p0 = e0.z * v0.y - e0.y * v0.z;
        p2 = e0.z * v2.y - e0.y * v2.z;
        if (p0 < p2) { min = p0; max = p2; }
        else { min = p2; max = p0; }
        rad = fe.z * boxhalfsize.y + fe.y * boxhalfsize.z;
        if (min > rad || max < -rad) return false;
        p0 = -e0.z * v0.x + e0.x * v0.z;
        p2 = -e0.z * v2.x + e0.x * v2.z;
        if (p0 < p2) { min = p0; max = p2; }
        else { min = p2; max = p0; }
        rad = fe.z * boxhalfsize.x + fe.x * boxhalfsize.z;
        if (min > rad || max < -rad) return false;
        p1 = e0.y * v1.x - e0.x * v1.y;
        p2 = e0.y * v2.x - e0.x * v2.y;
        if (p2 < p1) { min = p2; max = p1; } else { min = p1; max = p2; }
        rad = fe.y * boxhalfsize.x + fe.x * boxhalfsize.y;
        if (min > rad || max < -rad) return false;

        fe = absolute(e1);
        p0 = e1.z * v0.y - e1.y * v0.z;
        p2 = e1.z * v2.y - e1.y * v2.z;
        if (p0 < p2) { min = p0; max = p2; } else { min = p2; max = p0; }
        rad = fe.z * boxhalfsize.y + fe.y * boxhalfsize.z;
        if (min > rad || max < -rad) return false;
        p0 = -e1.z * v0.x + e1.x * v0.z;
        p2 = -e1.z * v2.x + e1.x * v2.z;
        if (p0 < p2) { min = p0; max = p2; } else { min = p2; max = p0; }
        rad = fe.z * boxhalfsize.x + fe.x * boxhalfsize.z;
        if (min > rad || max < -rad) return false;
        p0 = e1.y * v0.x - e1.x * v0.y;
        p1 = e1.y * v1.x - e1.x * v1.y;
        if (p0 < p1) { min = p0; max = p1; } else { min = p1; max = p0; }
        rad = fe.y * boxhalfsize.x + fe.x * boxhalfsize.y;
        if (min > rad || max < -rad) return false;

        fe = absolute(e2);
        p0 = e2.z * v0.y - e2.y * v0.z;
        p1 = e2.z * v1.y - e2.y * v1.z;
        if (p0 < p1) { min = p0; max = p1; } else { min = p1; max = p0; }
        rad = fe.z * boxhalfsize.y + fe.y * boxhalfsize.z;
        if (min > rad || max < -rad) return false;
        p0 = -e2.z * v0.x + e2.x * v0.z;
        p1 = -e2.z * v1.x + e2.x * v1.z;
        if (p0 < p1) { min = p0; max = p1; } else { min = p1; max = p0; }
        rad = fe.z * boxhalfsize.x + fe.x * boxhalfsize.z;
        if (min > rad || max < -rad) return false;
        p1 = e2.y * v1.x - e2.x * v1.y;
        p2 = e2.y * v2.x - e2.x * v2.y;
        if (p2 < p1) { min = p2; max = p1; } else { min = p1; max = p2; }
        rad = fe.y * boxhalfsize.x + fe.x * boxhalfsize.y;
        if (min > rad || max < -rad) return false;

        Vec3 omin = ::min(::min(v0, v1), v2);
        Vec3 omax = ::max(::max(v0, v1), v2);
        if ((omin.x > boxhalfsize.x || omax.x < -boxhalfsize.x) ||
            (omin.y > boxhalfsize.y || omax.y < -boxhalfsize.y) ||
            (omin.z > boxhalfsize.z || omax.z < -boxhalfsize.z)) return false;

        Vec3 normal = cross(e0, e1);

        Vec3 vmin, vmax;
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

        if (!(dot(normal, vmin) <= 0.f || dot(normal, vmax) >= 0.f)) return false;

        return true;
    }

    bool intersectsTriangle(Vec3 v0, Vec3 v1, Vec3 v2) const
    {
        Vec3 boxcenter = (min + max) * 0.5f;
        Vec3 boxhalfsize = (max - min) * 0.5f;

        Vec3 verts[3] = {
            { v0.x, v0.y, v0.z },
            { v1.x, v1.y, v1.z },
            { v2.x, v2.y, v2.z },
        };
        return triBoxOverlap(boxcenter, boxhalfsize, verts);
    }

    BoundingBox transform(Mat4 const& transform) const
    {
        Vec3 minP(FLT_MAX), maxP(-FLT_MAX);
        Vec3 corners[8] = {
            { min.x, min.y, min.z },
            { max.x, min.y, min.z },
            { max.x, max.y, min.z },
            { min.x, max.y, min.z },
            { min.x, min.y, max.z },
            { max.x, min.y, max.z },
            { max.x, max.y, max.z },
            { min.x, max.y, max.z },
        };
        for (Vec3& v : corners)
        {
            v = (transform * Vec4(v, 1.f)).xyz;
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
        return { ::min(other.min, min), ::max(other.max, max) };
    }

    f32 volume() const
    {
        Vec3 dim = max - min;
        return dim.x * dim.y * dim.z;
    }

    BoundingBox expand(f32 amount)
    {
        return { min - amount, max + amount };
    }
};

BoundingBox computeCameraFrustumBoundingBox(Mat4 const& inverseViewProj)
{
    Vec3 ndc[] = {
        { -1,  1, 0 },
        {  1,  1, 0 },
        {  1, -1, 0 },
        { -1, -1, 0 },

        { -1,  1, 1 },
        {  1,  1, 1 },
        {  1, -1, 1 },
        { -1, -1, 1 },
    };

    f32 minx =  FLT_MAX;
    f32 maxx = -FLT_MAX;
    f32 miny =  FLT_MAX;
    f32 maxy = -FLT_MAX;
    f32 minz =  FLT_MAX;
    f32 maxz = -FLT_MAX;
    for (auto& v : ndc)
    {
        Vec4 b = inverseViewProj * Vec4(v, 1.f);
        v = b.xyz / b.w;

        if (v.x < minx) minx = v.x;
        if (v.x > maxx) maxx = v.x;
        if (v.y < miny) miny = v.y;
        if (v.y > maxy) maxy = v.y;
        if (v.z < minz) minz = v.z;
        if (v.z > maxz) maxz = v.z;
    }

    return BoundingBox{ { minx, miny, minz }, { maxx, maxy, maxz } };
}
