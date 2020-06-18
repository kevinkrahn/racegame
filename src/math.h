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
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/compatibility.hpp>

#include <iostream>

using Vec2i = glm::ivec2;
using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;
using Quat = glm::quat;

#include <PxPhysicsAPI.h>
using namespace physx;

#define PI PxPi
#define PI2 (PxPi * 2.f)

inline f32 radians(f32 deg)
{
    return deg * PI / 180.f;
}

inline f32 degrees(f32 rad)
{
    return rad * 180.f / PI;
}

inline f32 absolute(f32 v)
{
    return v < 0.f ? -v : v;
}

inline Vec2 absolute(Vec2 v)
{
    return { absolute(v.x), absolute(v.y) };
}

inline Vec3 absolute(Vec3 const& v)
{
    return { absolute(v.x), absolute(v.y), absolute(v.z) };
}

inline Vec4 absolute(Vec4 const& v)
{
    return { absolute(v.x), absolute(v.y), absolute(v.z), absolute(v.w) };
}

inline f32 lengthSquared(Vec2 v)
{
    return v.x*v.x + v.y*v.y;
}

inline f32 length(Vec2 v)
{
    return sqrtf(lengthSquared(v));
}

inline f32 lengthSquared(Vec3 const& v)
{
    return v.x*v.x + v.y*v.y + v.z*v.z;
}

inline f32 length(Vec3 const& v)
{
    return sqrtf(lengthSquared(v));
}

inline f32 lengthSquared(Vec4 const& v)
{
    return v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w;
}

inline f32 length(Vec4 const& v)
{
    return sqrtf(lengthSquared(v));
}

inline Vec3 xAxisOf(Mat4 const& m)
{
    return m[0];
}

inline Vec3 yAxisOf(Mat4 const& m)
{
    return m[1];
}

inline Vec3 zAxisOf(Mat4 const& m)
{
    return m[2];
}

inline Vec3 translationOf(Mat4 const& m)
{
    return m[3];
}

inline Vec3 scaleOf(Mat4 const& m)
{
    return Vec3(length(Vec3(m[0])), length(Vec3(m[1])), length(Vec3(m[2])));
}

inline Mat4 rotationOf(Mat4 m)
{
    m[0] /= length(Vec3(m[0]));
    m[1] /= length(Vec3(m[1]));
    m[2] /= length(Vec3(m[2]));
    m[3] = Vec4(0, 0, 0, 1);
    return m;
}

inline Vec3 convert(PxVec3 const& v)
{
    return Vec3(v.x, v.y, v.z);
}

inline PxVec3 convert(Vec3 const& v)
{
    return PxVec3(v.x, v.y, v.z);
}

inline Mat4 convert(PxMat44 const& m)
{
    return glm::make_mat4(m.front());
}

inline PxTransform convert(Mat4 const& m)
{
    Vec3 pos(m[3]);
    Quat rot = glm::quat_cast(Mat3(rotationOf(m)));
    return PxTransform(convert(pos), PxQuat(rot.x, rot.y, rot.z, rot.w));
}

inline PxQuat convert(Quat const& q)
{
    return PxQuat(q.x, q.y, q.z, q.w);
}

inline f32 square(f32 v)
{
    return v * v;
}

inline Vec3 rgbFromU32(u32 color)
{
    return Vec3(((color & 0x000000FF)) / 255.f,
                     ((color & 0x0000FF00) >> 8)  / 255.f,
                     ((color & 0x00FF0000) >> 16) / 255.f );
}

inline Vec4 rgbaFromU32(u32 color, f32 alpha = 1.f)
{
    return Vec4(rgbFromU32(color), alpha);
}

inline f32 pointDirection(Vec2 v1, Vec2 v2)
{
    Vec2 d = v2 - v1;
    return -atan2f(d.x, d.y);
}

inline f32 lerp(f32 from, f32 to, f32 t)
{
    return from + (to - from) * t;
}

inline Vec3 lerp(Vec3 const& from, Vec3 const& to, f32 t)
{
    return from + (to - from) * t;
}

inline Vec3 mix(Vec3 const& from, Vec3 const& to, f32 t)
{
    return lerp(from, to, t);
}

inline Vec4 lerp(Vec4 const& from, Vec4 const& to, f32 t)
{
    return from + (to - from) * t;
}

inline Vec4 mix(Vec4 const& from, Vec4 const& to, f32 t)
{
    return lerp(from, to, t);
}

inline f32 smoothMove(f32 from, f32 to, f32 amount, f32 deltaTime)
{
    return lerp(from, to, 1.f-expf(-amount * deltaTime));
}

inline Vec3 smoothMove(const Vec3& from, const Vec3& to, f32 amount, f32 deltaTime)
{
    return lerp(from, to, 1.f-exp(-amount * deltaTime));
}

inline f32 smoothMoveSnap(f32 from, f32 to, f32 amount, f32 deltaTime, f32 snap)
{
    f32 val = lerp(from, to, 1.f-expf(-amount * deltaTime));
    return absolute(val - to) < snap ? to : val;
}

inline bool pointInRectangle(Vec2 p, Vec2 v1, Vec2 v2)
{
    return p.x >= v1.x && p.y >= v1.y && p.x <= v2.x && p.y <= v2.y;
}

inline bool pointInRectangle(Vec2 p, Vec2 v1, f32 width, f32 height)
{
    Vec2 v2 = v1 + Vec2(width, height);
    return p.x >= v1.x && p.y >= v1.y && p.x <= v2.x && p.y <= v2.y;
}

inline bool pointInTriangle(Vec2 s, Vec2 a, Vec2 b, Vec2 c)
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
    return (f32)((f64)xorshift32(series) / (f64)UINT32_MAX);
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

inline Vec2 floor(Vec2 v)
{
    return { floorf(v.x), floorf(v.y) };
}

inline Vec3 floor(Vec3 const& v)
{
    return { floorf(v.x), floorf(v.y), floorf(v.z) };
}

inline f32 snap(f32 val, f32 multiple)
{
    return floorf(val / multiple) * multiple;
}

inline Vec2 snap(Vec2 val, f32 multiple)
{
    return floor(val / multiple) * multiple;
}

inline Vec3 snapXY(Vec3 const& val, f32 multiple)
{
    return Vec3(snap(Vec2(val) + Vec2(multiple * 0.5f), multiple), val.z);
}

inline Vec2 lengthdir(f32 angle, f32 len)
{
    return Vec2(cosf(angle), sinf(angle)) * len;
}

template <typename T>
inline T clamp(T val, T min, T max)
{
    return val <= min ? min : (val >= max ? max : val);
}

inline f32 min(f32 a, f32 b)
{
    return a < b ? a : b;
}

inline Vec2 min(Vec2 a, Vec2 b)
{
    return Vec2(min(a.x, b.x), min(a.x, b.y));
}

inline Vec3 min(Vec3 const& a, Vec3 const& b)
{
    return Vec3(min(a.x, b.x), min(a.x, b.y), min(a.z, b.z));
}

inline Vec4 min(Vec4 const& a, Vec4 const& b)
{
    return Vec4(min(a.x, b.x), min(a.x, b.y), min(a.z, b.z), min(a.w, b.w));
}

inline f32 max(f32 a, f32 b)
{
    return a > b ? a : b;
}

inline Vec2 max(Vec2 a, Vec2 b)
{
    return Vec2(max(a.x, b.x), max(a.x, b.y));
}

inline Vec3 max(Vec3 const& a, Vec3 const& b)
{
    return Vec3(max(a.x, b.x), max(a.x, b.y), max(a.z, b.z));
}

inline Vec4 max(Vec4 const& a, Vec4 const& b)
{
    return Vec4(max(a.x, b.x), max(a.x, b.y), max(a.z, b.z), max(a.w, b.w));
}

inline Vec2 normalize(Vec2 const& v)
{
    f32 l = length(v);
    return l > 0.f ? v / l : Vec2(0.f);
}

inline Vec3 normalize(Vec3 const& v)
{
    f32 l = length(v);
    return l > 0.f ? v / l : Vec3(0.f);
}

inline Vec4 normalize(Vec4 const& v)
{
    f32 l = length(v);
    return l > 0.f ? v / l : Vec4(0.f);
}

inline f32 angleDifference(f32 angle0, f32 angle1)
{
    f32 a = angle1 - angle0;
    if (a >  PI) a -= PI * 2.f;
    if (a < -PI) a += PI * 2.f;
    return a;
}

inline f32 distance(Vec2 a, Vec2 b)
{
    return length(a - b);
}

inline f32 distance(Vec3 const& a, Vec3 const& b)
{
    return length(a - b);
}

inline f32 distance(Vec4 const& a, Vec4 const& b)
{
    return length(a - b);
}

inline f32 distanceSquared(Vec2 a, Vec2 b)
{
    return lengthSquared(a - b);
}

inline f32 distanceSquared(Vec3 const& a, Vec3 const& b)
{
    return lengthSquared(a - b);
}

inline f32 distanceSquared(Vec4 const& a, Vec4 const& b)
{
    return lengthSquared(a - b);
}

inline f32 dot(Vec2 a, Vec2 b)
{
    return a.x*b.x + a.y*b.y;
}

inline f32 dot(Vec3 const& a, Vec3 const& b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

inline f32 dot(Vec4 const& a, Vec4 const& b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

inline Vec3 cross(Vec3 const& a, Vec3 const& b)
{
    return Vec3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
}

inline Vec3 screenToWorldRay(Vec2 screenPos, Vec2 screenSize, Mat4 const& view, Mat4 const& projection)
{
    Vec3 pos(
        (2.f * screenPos.x) / screenSize.x - 1.f,
        (1.f - (2.f * screenPos.y) / screenSize.y), 1.f
    );
    Vec4 rayClip(pos.x, pos.y, 0.f, 1.f);
    Vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = Vec4(rayEye.x, rayEye.y, -1.f, 0.f);
    Vec3 rayWorld = normalize(Vec3(glm::inverse(view) * rayEye));
    return rayWorld;
}

inline Vec2 project(Vec3 const& pos, Mat4 const& viewProj)
{
    Vec4 p = viewProj * Vec4(pos, 1.f);
    f32 x = ((p.x / p.w) + 1.f) / 2.f;
    f32 y = ((-p.y / p.w) + 1.f) / 2.f;
    return { x, y };
}

inline Vec2 projectScale(Vec3 const& pos, Vec3 const& offset, Mat4 const& viewProj, f32 scaleFactor = 0.01f)
{
    Mat4 mvp = viewProj * glm::translate(Mat4(1.f), pos);
    f32 w = (mvp * Vec4(0, 0, 0, 1)).w * scaleFactor;
    Vec4 p = mvp * Vec4(offset * w, 1.f);
    f32 x = ((p.x / p.w) + 1.f) / 2.f;
    f32 y = ((-p.y / p.w) + 1.f) / 2.f;
    return { x, y };
}

inline f32 rayPlaneIntersection(Vec3 const& rayOrigin, Vec3 const& rayDir,
        Vec3 const& planeNormal, Vec3 const& planePoint)
{
    f32 denom = dot(planeNormal, rayDir);
    f32 t = dot(planePoint - rayOrigin, planeNormal) / denom;
    return t;
}

inline f32 raySphereIntersection(Vec3 const& rayOrigin, Vec3 const& rayDir,
        Vec3 const& sphereCenter, f32 sphereRadius)
{
    Vec3 m = rayOrigin - sphereCenter;
    f32 b = dot(m, rayDir);
    f32 c = dot(m, m) - (sphereRadius * sphereRadius);
    if (c > 0.0f && b > 0.0f) return 0;
    f32 discr = b * b - c;
    if (discr < 0.0f) return 0;
    f32 t = -b - sqrtf(discr);
    if (t < 0.0f) t = 0.0f;
    return t;
}

inline bool lineIntersection(Vec2 p1, Vec2 p2, Vec2 p3, Vec2 p4, Vec2& out)
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
    if (x < min(x1, x2) || x > max(x1, x2) ||
        x < min(x3, x4) || x > max(x3, x4))
    {
        return false;
    }
    if (y < min(y1, y2) || y > max(y1, y2) ||
        y < min(y3, y4) || y > max(y3, y4))
    {
        return false;
    }
    out.x = x;
    out.y = y;
    return true;
}

inline Vec3 pointOnBezierCurve(Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3, f32 t)
{
    f32 u = 1.f - t;
    f32 t2 = t * t;
    f32 u2 = u * u;
    f32 u3 = u2 * u;
    f32 t3 = t2 * t;
    return Vec3(u3 * p0 + (3.f * u2 * t) * p1 + (3.f * u * t2) * p2 + t3 * p3);
}

inline f32 nonZeroOrDefault(f32 val, f32 defaultVal)
{
    return val != 0.f ? val : defaultVal;
}

inline Vec3 srgb(f32 r, f32 g, f32 b)
{
    return Vec3(powf(r, 2.2f), powf(g, 2.2f), powf(b, 2.2f));
}

inline Vec3 srgb(Vec3 const& rgb)
{
    return srgb(rgb.r, rgb.g, rgb.b);
}

inline Vec3 hsvToRgb(f32 h, f32 s, f32 v)
{
    h *= 360;
    f32 chroma = v * s;
    f32 hprime = fmodf(h / 60.f, 6);
    f32 x = chroma * (1 - absolute(fmodf(hprime, 2) - 1));
    f32 m = v - chroma;

    Vec3 out;
    if (0 <= hprime && hprime < 1)
    {
        out = { chroma, x, 0 };
    }
    else if (1 <= hprime && hprime < 2)
    {
        out = { x, chroma, 0 };
    }
    else if (2 <= hprime && hprime < 3)
    {
        out = { 0, chroma, x };
    }
    else if (3 <= hprime && hprime < 4)
    {
        out = { 0, x, chroma };
    }
    else if (4 <= hprime && hprime < 5)
    {
        out = { x, 0, chroma };
    }
    else if (5 <= hprime && hprime < 6)
    {
        out = { chroma, 0, x };
    }
    else
    {
        out = { 0, 0, 0 };
    }

    out += Vec3(m);

    return out;
}

inline Mat4 perspective(f32 fov, f32 aspect, f32 near, f32 far)
{
    const f32 tanHalfFov = tanf(fov / 2.f);

    Mat4 result(0.f);

    result[0][0] = 1.f / (aspect * tanHalfFov);
#if 0
    result[1][1] = -1.f / tanHalfFov; // flipped
#else
    result[1][1] = 1.f / tanHalfFov;
#endif

#if 0
    // depth zero to one
    result[2][2] = far / (near - far);
    result[2][3] = -1.f;
    result[3][2] = -(far * near) / (far - near);
#else
    result[2][2] = -(far + near) / (far - near);
    result[2][3] = -1.f;
    result[3][2] = -(2.f * far * near) / (far - near);
#endif

    return result;
}

inline Mat4 lookAt(Vec3 const& eye, Vec3 const& center, Vec3 const& up)
{
    const Vec3 f = normalize(center - eye);
    const Vec3 s = normalize(cross(f, up));
    const Vec3 u = cross(s, f);

    Mat4 result(1.f);

    result[0][0] = s.x;
    result[1][0] = s.y;
    result[2][0] = s.z;
    result[0][1] = u.x;
    result[1][1] = u.y;
    result[2][1] = u.z;
    result[0][2] = -f.x;
    result[1][2] = -f.y;
    result[2][2] = -f.z;
    result[3][0] = -dot(s, eye);
    result[3][1] = -dot(u, eye);
    result[3][2] =  dot(f, eye);

    return result;
}
