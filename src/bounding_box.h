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

    bool intersects(BoundingBox const& bb) const
    {
        return min.x <= bb.max.x && max.x >= bb.min.x &&
               min.y <= bb.max.y && max.y >= bb.min.y &&
               min.z <= bb.max.z && max.z >= bb.min.z;
    }
#define X 0
#define Y 1
#define Z 2

#define CROSS(dest,v1,v2) \
          dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
          dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
          dest[2]=v1[0]*v2[1]-v1[1]*v2[0];

#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])

#define SUB(dest,v1,v2) \
          dest[0]=v1[0]-v2[0]; \
          dest[1]=v1[1]-v2[1]; \
          dest[2]=v1[2]-v2[2];

#define FINDMINMAX(x0,x1,x2,min,max) \
  min = max = x0;   \
  if(x1<min) min=x1;\
  if(x1>max) max=x1;\
  if(x2<min) min=x2;\
  if(x2>max) max=x2;

    int planeBoxOverlap(float normal[3], float vert[3], float maxbox[3]) const
    {
        int q;
        float vmin[3],vmax[3],v;
        for(q=X;q<=Z;q++)
        {
            v=vert[q];
            if(normal[q]>0.0f)
            {
                vmin[q]=-maxbox[q] - v;
                vmax[q]= maxbox[q] - v;
            }
            else
            {
                vmin[q]= maxbox[q] - v;
                vmax[q]=-maxbox[q] - v;
            }
        }
        if(DOT(normal,vmin)>0.0f) return 0;
        if(DOT(normal,vmax)>=0.0f) return 1;

        return 0;
    }


/*======================== X-tests ========================*/
#define AXISTEST_X01(a, b, fa, fb)             \
    p0 = a*v0[Y] - b*v0[Z];                    \
    p2 = a*v2[Y] - b*v2[Z];                    \
        if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
    rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];   \
    if(min>rad || max<-rad) return 0;

#define AXISTEST_X2(a, b, fa, fb)              \
    p0 = a*v0[Y] - b*v0[Z];                    \
    p1 = a*v1[Y] - b*v1[Z];                    \
        if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
    rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];   \
    if(min>rad || max<-rad) return 0;

/*======================== Y-tests ========================*/
#define AXISTEST_Y02(a, b, fa, fb)             \
    p0 = -a*v0[X] + b*v0[Z];                   \
    p2 = -a*v2[X] + b*v2[Z];                       \
        if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
    rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];   \
    if(min>rad || max<-rad) return 0;

#define AXISTEST_Y1(a, b, fa, fb)              \
    p0 = -a*v0[X] + b*v0[Z];                   \
    p1 = -a*v1[X] + b*v1[Z];                       \
        if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
    rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];   \
    if(min>rad || max<-rad) return 0;

/*======================== Z-tests ========================*/

#define AXISTEST_Z12(a, b, fa, fb)             \
    p1 = a*v1[X] - b*v1[Y];                    \
    p2 = a*v2[X] - b*v2[Y];                    \
        if(p2<p1) {min=p2; max=p1;} else {min=p1; max=p2;} \
    rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];   \
    if(min>rad || max<-rad) return 0;

#define AXISTEST_Z0(a, b, fa, fb)              \
    p0 = a*v0[X] - b*v0[Y];                \
    p1 = a*v1[X] - b*v1[Y];                    \
        if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
    rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];   \
    if(min>rad || max<-rad) return 0;

    int triBoxOverlap(float boxcenter[3],float boxhalfsize[3],float triverts[3][3]) const
    {
        /*    use separating axis theorem to test overlap between triangle and box */
        /*    need to test for overlap in these directions: */
        /*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
        /*       we do not even need to test these) */
        /*    2) normal of the triangle */
        /*    3) crossproduct(edge from tri, {x,y,z}-directin) */
        /*       this gives 3x3=9 more tests */
        float v0[3],v1[3],v2[3];
        //   float axis[3];
        float min,max,p0,p1,p2,rad,fex,fey,fez;     // -NJMP- "d" local variable removed
        float normal[3],e0[3],e1[3],e2[3];

        /* This is the fastest branch on Sun */
        /* move everything so that the boxcenter is in (0,0,0) */
        SUB(v0,triverts[0],boxcenter);
        SUB(v1,triverts[1],boxcenter);
        SUB(v2,triverts[2],boxcenter);

        /* compute triangle edges */
        SUB(e0,v1,v0);      /* tri edge 0 */
        SUB(e1,v2,v1);      /* tri edge 1 */
        SUB(e2,v0,v2);      /* tri edge 2 */

        /* Bullet 3:  */
        /*  test the 9 tests first (this was faster) */
        fex = fabsf(e0[X]);
        fey = fabsf(e0[Y]);
        fez = fabsf(e0[Z]);
        AXISTEST_X01(e0[Z], e0[Y], fez, fey);
        AXISTEST_Y02(e0[Z], e0[X], fez, fex);
        AXISTEST_Z12(e0[Y], e0[X], fey, fex);

        fex = fabsf(e1[X]);
        fey = fabsf(e1[Y]);
        fez = fabsf(e1[Z]);
        AXISTEST_X01(e1[Z], e1[Y], fez, fey);
        AXISTEST_Y02(e1[Z], e1[X], fez, fex);
        AXISTEST_Z0(e1[Y], e1[X], fey, fex);

        fex = fabsf(e2[X]);
        fey = fabsf(e2[Y]);
        fez = fabsf(e2[Z]);
        AXISTEST_X2(e2[Z], e2[Y], fez, fey);
        AXISTEST_Y1(e2[Z], e2[X], fez, fex);
        AXISTEST_Z12(e2[Y], e2[X], fey, fex);

        /* Bullet 1: */
        /*  first test overlap in the {x,y,z}-directions */
        /*  find min, max of the triangle each direction, and test for overlap in */
        /*  that direction -- this is equivalent to testing a minimal AABB around */
        /*  the triangle against the AABB */

        /* test in X-direction */
        FINDMINMAX(v0[X],v1[X],v2[X],min,max);
        if(min>boxhalfsize[X] || max<-boxhalfsize[X]) return 0;

        /* test in Y-direction */
        FINDMINMAX(v0[Y],v1[Y],v2[Y],min,max);
        if(min>boxhalfsize[Y] || max<-boxhalfsize[Y]) return 0;

        /* test in Z-direction */
        FINDMINMAX(v0[Z],v1[Z],v2[Z],min,max);
        if(min>boxhalfsize[Z] || max<-boxhalfsize[Z]) return 0;

        /* Bullet 2: */
        /*  test if the box intersects the plane of the triangle */
        /*  compute plane equation of triangle: normal*x+d=0 */
        CROSS(normal,e0,e1);
        // -NJMP- (line removed here)
        if(!planeBoxOverlap(normal,v0,boxhalfsize)) return 0;

        return 1;
    }

#if 1
    bool intersectsTriangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2) const
    {
        glm::vec3 boxcenter = (min + max) * 0.5f;
        glm::vec3 boxhalfsize = (max - min) * 0.5f;

        f32 verts[3][3] = {
            { v0.x, v0.y, v0.z },
            { v1.x, v1.y, v1.z },
            { v2.x, v2.y, v2.z },
        };
        return triBoxOverlap((f32*)&boxcenter, (f32*)&boxhalfsize, verts);
    }
#else
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

        //if (!(glm::dot(normal, vmin) <= 0.f || glm::dot(normal, vmax) >= 0.f)) return false;
        bool hit = false;
        if (glm::dot(normal, vmin) > 0.f) hit = false;
        else if (glm::dot(normal, vmax) >= 0.f) hit = true;
        if (!hit) return false;

        return true;
    }
#endif

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
        return { glm::min(other.min, min), glm::max(other.max, max) };
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
