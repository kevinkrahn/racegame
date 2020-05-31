#pragma once

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef float    f32;
typedef double   f64;

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/compatibility.hpp>

#include <iostream>

#include <PxPhysicsAPI.h>
using namespace physx;

#define PI PxPi
#define PI2 (PxPi * 2.f)

inline glm::vec3 xAxisOf(glm::mat4 const& m)
{
    return m[0];
}

inline glm::vec3 yAxisOf(glm::mat4 const& m)
{
    return m[1];
}

inline glm::vec3 zAxisOf(glm::mat4 const& m)
{
    return m[2];
}

inline glm::vec3 translationOf(glm::mat4 const& m)
{
    return m[3];
}

inline glm::vec3 scaleOf(glm::mat4 const& m)
{
    return glm::vec3(
            glm::length(glm::vec3(m[0])),
            glm::length(glm::vec3(m[1])),
            glm::length(glm::vec3(m[2])));
}

inline glm::mat4 rotationOf(glm::mat4 m)
{
    m[0] /= glm::length(glm::vec3(m[0]));
    m[1] /= glm::length(glm::vec3(m[1]));
    m[2] /= glm::length(glm::vec3(m[2]));
    m[3] = glm::vec4(0, 0, 0, 1);
    return m;
}

inline glm::vec3 convert(PxVec3 const& v)
{
    return glm::vec3(v.x, v.y, v.z);
}

inline PxVec3 convert(glm::vec3 const& v)
{
    return PxVec3(v.x, v.y, v.z);
}

inline glm::mat4 convert(PxMat44 const& m)
{
    return glm::make_mat4(m.front());
}

inline PxTransform convert(glm::mat4 const& m)
{
    glm::vec3 pos(m[3]);
    glm::quat rot = glm::quat_cast(glm::mat3(rotationOf(m)));
    return PxTransform(convert(pos), PxQuat(rot.x, rot.y, rot.z, rot.w));
}

inline PxQuat convert(glm::quat const& q)
{
    return PxQuat(q.x, q.y, q.z, q.w);
}

inline f32 square(f32 v)
{
    return v * v;
}

inline glm::vec3 rgbFromU32(u32 color)
{
    return glm::vec3(((color & 0x000000FF)) / 255.f,
                     ((color & 0x0000FF00) >> 8)  / 255.f,
                     ((color & 0x00FF0000) >> 16) / 255.f );
}

inline glm::vec4 rgbaFromU32(u32 color, f32 alpha = 1.f)
{
    return glm::vec4(rgbFromU32(color), alpha);
}

namespace glm
{
    inline std::ostream& operator << (std::ostream& lhs, glm::vec2 const& rhs)
    {
        return lhs << "{ " << rhs.x << ", " << rhs.y << " }";
    }

    inline std::ostream& operator << (std::ostream& lhs, glm::vec3 const& rhs)
    {
        return lhs << "{ " << rhs.x << ", " << rhs.y << ", " << rhs.z << " }";
    }

    inline std::ostream& operator << (std::ostream& lhs, glm::vec4 const& rhs)
    {
        return lhs << "{ " << rhs.x << ", " << rhs.y << ", " << rhs.z << ", " << rhs.w << " }";
    }
};

inline f32 pointDirection(glm::vec2 v1, glm::vec2 v2)
{
    glm::vec2 d = v2 - v1;
    return -atan2f(d.x, d.y);
}

inline f32 smoothMove(f32 from, f32 to, f32 amount, f32 deltaTime)
{
    return glm::lerp(from, to, 1.f-expf(-amount * deltaTime));
}

inline glm::vec3 smoothMove(const glm::vec3& from, const glm::vec3& to, f32 amount, f32 deltaTime)
{
    return glm::lerp(from, to, 1.f-exp(-amount * deltaTime));
}

inline f32 smoothMoveSnap(f32 from, f32 to, f32 amount, f32 deltaTime, f32 snap)
{
    f32 val = glm::lerp(from, to, 1.f-expf(-amount * deltaTime));
    return glm::abs(val - to) < snap ? to : val;
}

inline bool pointInRectangle(glm::vec2 p, glm::vec2 v1, glm::vec2 v2)
{
    return p.x >= v1.x && p.y >= v1.y && p.x <= v2.x && p.y <= v2.y;
}

inline bool pointInRectangle(glm::vec2 p, glm::vec2 v1, f32 width, f32 height)
{
    glm::vec2 v2 = v1 + glm::vec2(width, height);
    return p.x >= v1.x && p.y >= v1.y && p.x <= v2.x && p.y <= v2.y;
}

inline bool pointInTriangle(glm::vec2 s, glm::vec2 a, glm::vec2 b, glm::vec2 c)
{
    f32 as_x = s.x - a.x;
    f32 as_y = s.y - a.y;

    bool s_ab = (b.x - a.x) * as_y - (b.y - a.y) * as_x > 0;

    if ((c.x - a.x) * as_y - (c.y - a.y) * as_x > 0 == s_ab)
    {
        return false;
    }

    if ((c.x - b.x) * (s.y - b.y) - (c.y - b.y) * (s.x - b.x) > 0 != s_ab)
    {
        return false;
    }

    return true;
}

struct RandomSeries
{
    u32 state = 1234;
};

inline u32 xorshift32(RandomSeries& series)
{
    u32 x = series.state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    series.state = x;
    return x;
}

inline f32 random01(RandomSeries& series)
{
    return (f32)((f64)xorshift32(series) / (f64)std::numeric_limits<u32>::max());
}

inline f32 random(RandomSeries& series, f32 min, f32 max)
{
    return random01(series) * (max - min) + min;
}

// return random integer between min (inclusive) and max (exclusive)
inline i32 irandom(RandomSeries& series, i32 min, i32 max)
{
    i32 diff = max - min;
    return diff > 0 ? (xorshift32(series) % diff) + min : min;
}

inline glm::vec3 closest(glm::vec3 const& p, glm::vec3 const& v1, glm::vec3 const& v2)
{
    return glm::length2(p - v1) < glm::length2(p - v2) ? v1 : v2;
}

inline f32 snap(f32 val, f32 multiple)
{
    return glm::floor(val / multiple) * multiple;
}

inline glm::vec2 snap(glm::vec2 val, f32 multiple)
{
    return glm::floor(val / multiple) * multiple;
}

inline glm::vec3 snapXY(glm::vec3 const& val, f32 multiple)
{
    return glm::vec3(snap(glm::vec2(val) + glm::vec2(multiple * 0.5f), multiple), val.z);
}

inline glm::vec2 lengthdir(f32 angle, f32 len)
{
    return glm::vec2(glm::cos(angle), glm::sin(angle)) * len;
}

template <typename T>
inline T clamp(T val, T min, T max)
{
    return val <= min ? min : (val >= max ? max : val);
}

inline f32 angleDifference(f32 angle0, f32 angle1)
{
    f32 m = glm::pi<f32>();
    f32 a = angle1 - angle0;
    if (a >  m) a -= m * 2.f;
    if (a < -m) a += m * 2.f;
    return a;
}

inline glm::vec3 screenToWorldRay(glm::vec2 screenPos, glm::vec2 screenSize, glm::mat4 const& view, glm::mat4 const& projection)
{
    glm::vec3 pos(
        (2.f * screenPos.x) / screenSize.x - 1.f,
        (1.f - (2.f * screenPos.y) / screenSize.y), 1.f
    );
    glm::vec4 rayClip(pos.x, pos.y, 0.f, 1.f);
    glm::vec4 rayEye = inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.f, 0.f);
    glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
    return rayWorld;
}

inline glm::vec2 project(glm::vec3 const& pos, glm::mat4 const& viewProj)
{
    glm::vec4 p = viewProj * glm::vec4(pos, 1.f);
    f32 x = ((p.x / p.w) + 1.f) / 2.f;
    f32 y = ((-p.y / p.w) + 1.f) / 2.f;
    return { x, y };
}

inline glm::vec2 projectScale(glm::vec3 const& pos, glm::vec3 const& offset, glm::mat4 const& viewProj, f32 scaleFactor = 0.01f)
{
    glm::mat4 mvp = viewProj * glm::translate(glm::mat4(1.f), pos);
    f32 w = (mvp * glm::vec4(0, 0, 0, 1)).w * scaleFactor;
    glm::vec4 p = mvp * glm::vec4(offset * w, 1.f);
    f32 x = ((p.x / p.w) + 1.f) / 2.f;
    f32 y = ((-p.y / p.w) + 1.f) / 2.f;
    return { x, y };
}

inline f32 rayPlaneIntersection(glm::vec3 const& rayOrigin, glm::vec3 const& rayDir,
        glm::vec3 const& planeNormal, glm::vec3 const& planePoint)
{
    f32 denom = glm::dot(planeNormal, rayDir);
    f32 t = glm::dot(planePoint - rayOrigin, planeNormal) / denom;
    return t;
}

inline f32 raySphereIntersection(glm::vec3 const& rayOrigin, glm::vec3 const& rayDir,
        glm::vec3 const& sphereCenter, f32 sphereRadius)
{
    glm::vec3 m = rayOrigin - sphereCenter;
    f32 b = glm::dot(m, rayDir);
    f32 c = glm::dot(m, m) - (sphereRadius * sphereRadius);
    if (c > 0.0f && b > 0.0f) return 0;
    f32 discr = b * b - c;
    if (discr < 0.0f) return 0;
    f32 t = -b - sqrtf(discr);
    if (t < 0.0f) t = 0.0f;
    return t;
}

inline bool lineIntersection(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, glm::vec2& out)
{
    f32 x1 = p1.x, x2 = p2.x, x3 = p3.x, x4 = p4.x;
    f32 y1 = p1.y, y2 = p2.y, y3 = p3.y, y4 = p4.y;
    f32 d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    if (d == 0)
    {
        return false;
    }
    f32 pre = (x1*y2 - y1*x2);
    f32 post = (x3*y4 - y3*x4);
    f32 x = (pre * (x3 - x4) - (x1 - x2) * post) / d;
    f32 y = (pre * (y3 - y4) - (y1 - y2) * post) / d;
    if (x < glm::min(x1, x2) || x > glm::max(x1, x2) ||
        x < glm::min(x3, x4) || x > glm::max(x3, x4))
    {
        return false;
    }
    if (y < glm::min(y1, y2) || y > glm::max(y1, y2) ||
        y < glm::min(y3, y4) || y > glm::max(y3, y4))
    {
        return false;
    }
    out.x = x;
    out.y = y;
    return true;
}

inline glm::vec3 pointOnBezierCurve(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, f32 t)
{
    f32 u = 1.f - t;
    f32 t2 = t * t;
    f32 u2 = u * u;
    f32 u3 = u2 * u;
    f32 t3 = t2 * t;
    return glm::vec3(u3 * p0 + (3.f * u2 * t) * p1 + (3.f * u * t2) * p2 + t3 * p3);
}

inline f32 nonZeroOrDefault(f32 val, f32 defaultVal)
{
    return val != 0.f ? val : defaultVal;
}

inline glm::vec3 srgb(f32 r, f32 g, f32 b)
{
    return glm::vec3(glm::pow(r, 2.2f), glm::pow(g, 2.2f), glm::pow(b, 2.2f));
}
