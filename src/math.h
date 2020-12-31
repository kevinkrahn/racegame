#pragma once

#include <utility>
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

template <typename T>
void swp(T& a, T& b)
{
    auto tmp = std::move(a);
    a = std::move(b);
    b = std::move(tmp);
}

#include <PxPhysicsAPI.h>
using namespace physx;

#define PI PxPi
#define PI2 (PxPi * 2.f)

struct Vec2i
{
    i32 x, y;

    Vec2i(i32 x, i32 y) : x(x), y(y) {}
    explicit Vec2i(i32 x) : x(x), y(x) {}
    Vec2i() = default;
};

struct Vec2
{
    union
    {
        struct
        {
            f32 x, y;
        };

        struct
        {
            f32 u, v;
        };
    };

    Vec2(f32 x, f32 y) : x(x), y(y) {}
    explicit Vec2(f32 x) : x(x), y(x) {}
    Vec2() = default;
    explicit Vec2(struct Vec3 const& v);

    f32 direction() const
    {
        return -atan2f(x, y);
    }

    Vec2 perpendicular() const
    {
        return Vec2(-y, x);
    }

    Vec2& operator+=(Vec2 const& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    Vec2& operator+=(f32 rhs)
    {
        x += rhs;
        y += rhs;
        return *this;
    }

    Vec2& operator-=(Vec2 const& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    Vec2& operator-=(f32 rhs)
    {
        x -= rhs;
        y -= rhs;
        return *this;
    }

    Vec2& operator*=(f32 rhs)
    {
        x *= rhs;
        y *= rhs;
        return *this;
    }

    Vec2& operator*=(Vec2 const& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }

    Vec2& operator/=(f32 rhs)
    {
        x /= rhs;
        y /= rhs;
        return *this;
    }

    Vec2& operator/=(Vec2 const& rhs)
    {
        x /= rhs.x;
        y /= rhs.y;
        return *this;
    }

    Vec2 operator-() const
    {
        return { -x, -y };
    }
};

inline Vec2 operator+(Vec2 lhs, Vec2 const& rhs)
{
    lhs += rhs;
    return lhs;
}

inline Vec2 operator+(Vec2 lhs, f32 rhs)
{
    lhs += rhs;
    return lhs;
}

inline Vec2 operator-(Vec2 lhs, Vec2 const& rhs)
{
    lhs -= rhs;
    return lhs;
}

inline Vec2 operator-(Vec2 lhs, f32 rhs)
{
    lhs -= rhs;
    return lhs;
}

inline Vec2 operator*(Vec2 lhs, f32 rhs)
{
    lhs *= rhs;
    return lhs;
}

inline Vec2 operator*(f32 lhs, Vec2 rhs)
{
    rhs *= lhs;
    return rhs;
}

inline Vec2 operator*(Vec2 lhs, Vec2 const& rhs)
{
    lhs *= rhs;
    return lhs;
}

inline Vec2 operator/(Vec2 lhs, f32 rhs)
{
    lhs /= rhs;
    return lhs;
}

inline Vec2 operator/(f32 lhs, Vec2 rhs)
{
    return Vec2(lhs / rhs.x, lhs / rhs.y);
}

inline Vec2 operator/(Vec2 lhs, Vec2 const& rhs)
{
    lhs /= rhs;
    return lhs;
}

inline bool operator==(Vec2 const& lhs, Vec2 const& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline bool operator!=(Vec2 const& lhs, Vec2 const& rhs)
{
    return !operator==(lhs, rhs);
}

struct Vec3
{
    union
    {
        struct
        {
            f32 x, y, z;
        };
        struct
        {
            f32 r, g, b;
        };
        Vec2 xy;
        Vec2 uv;
        f32 data[3];
    };

    Vec3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}
    explicit Vec3(f32 x) : x(x), y(x), z(x) {}
    Vec3(Vec2 const& v, f32 z) : x(v.x), y(v.y), z(z) {}
    Vec3() = default;
    explicit Vec3(struct Vec4 const& v);

    Vec3(PxVec3 const& v) : x(v.x), y(v.y), z(v.z) {}
    //operator PxVec3() { return PxVec3(x, y, z); }

    void operator=(Vec3 const& rhs)
    {
        x = rhs.x;
        y = rhs.y;
        z = rhs.z;
    }

    f32& operator[](u32 index)
    {
        return data[index];
    }

    const f32& operator[](u32 index) const
    {
        return data[index];
    }

    Vec3& operator+=(Vec3 const& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    Vec3& operator+=(f32 rhs)
    {
        x += rhs;
        y += rhs;
        z += rhs;
        return *this;
    }

    Vec3& operator-=(Vec3 const& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    Vec3& operator-=(f32 rhs)
    {
        x -= rhs;
        y -= rhs;
        z -= rhs;
        return *this;
    }

    Vec3& operator*=(f32 rhs)
    {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        return *this;
    }

    Vec3& operator*=(Vec3 const& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        return *this;
    }

    Vec3& operator/=(f32 rhs)
    {
        x /= rhs;
        y /= rhs;
        z /= rhs;
        return *this;
    }

    Vec3& operator/=(Vec3 const& rhs)
    {
        x /= rhs.x;
        y /= rhs.y;
        z /= rhs.z;
        return *this;
    }

    Vec3 operator-() const
    {
        return { -x, -y, -z };
    }
};
//using Rgb = Vec3;

inline Vec3 operator+(Vec3 lhs, Vec3 const& rhs)
{
    lhs += rhs;
    return lhs;
}

inline Vec3 operator+(Vec3 lhs, f32 rhs)
{
    lhs += rhs;
    return lhs;
}

inline Vec3 operator-(Vec3 lhs, Vec3 const& rhs)
{
    lhs -= rhs;
    return lhs;
}

inline Vec3 operator-(Vec3 lhs, f32 rhs)
{
    lhs -= rhs;
    return lhs;
}

inline Vec3 operator*(Vec3 lhs, f32 rhs)
{
    lhs *= rhs;
    return lhs;
}

inline Vec3 operator*(f32 lhs, Vec3 rhs)
{
    rhs *= lhs;
    return rhs;
}

inline Vec3 operator*(Vec3 lhs, Vec3 const& rhs)
{
    lhs *= rhs;
    return lhs;
}

inline Vec3 operator/(Vec3 lhs, f32 rhs)
{
    lhs /= rhs;
    return lhs;
}

inline Vec3 operator/(Vec3 lhs, Vec3 const& rhs)
{
    lhs /= rhs;
    return lhs;
}

inline bool operator==(Vec3 const& lhs, Vec3 const& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

inline bool operator!=(Vec3 const& lhs, Vec3 const& rhs)
{
    return !operator==(lhs, rhs);
}

struct Vec4
{
    union
    {
        struct
        {
            f32 x, y, z, w;
        };
        struct
        {
            f32 r, g, b, a;
        };
        Vec3 xyz;
        Vec3 rgb;
        f32 data[4];
    };

    Vec4(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {}
    explicit Vec4(f32 x) : x(x), y(x), z(x), w(x) {}
    Vec4(Vec3 const& v, f32 w) : x(v.x), y(v.y), z(v.z), w(w) {}
    Vec4() = default;

    void operator=(Vec4 const& rhs)
    {
        x = rhs.x;
        y = rhs.y;
        z = rhs.z;
        w = rhs.w;
    }

    f32& operator[](u32 index)
    {
        return data[index];
    }

    const f32& operator[](u32 index) const
    {
        return data[index];
    }

    Vec4& operator+=(Vec4 const& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }

    Vec4& operator+=(f32 rhs)
    {
        x += rhs;
        y += rhs;
        z += rhs;
        w += rhs;
        return *this;
    }

    Vec4& operator-=(Vec4 const& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
        return *this;
    }

    Vec4& operator-=(f32 rhs)
    {
        x -= rhs;
        y -= rhs;
        z -= rhs;
        w -= rhs;
        return *this;
    }

    Vec4& operator*=(f32 rhs)
    {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        w *= rhs;
        return *this;
    }

    Vec4& operator*=(Vec4 const& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        w *= rhs.w;
        return *this;
    }

    Vec4& operator/=(f32 rhs)
    {
        x /= rhs;
        y /= rhs;
        z /= rhs;
        w /= rhs;
        return *this;
    }

    Vec4& operator/=(Vec4 const& rhs)
    {
        x /= rhs.x;
        y /= rhs.y;
        z /= rhs.z;
        w /= rhs.w;
        return *this;
    }

    Vec4 operator-() const
    {
        return { -x, -y, -z, -w };
    }
};
//using Rgba = Vec4;

inline Vec4 operator+(Vec4 lhs, Vec4 const& rhs)
{
    lhs += rhs;
    return lhs;
}

inline Vec4 operator+(Vec4 lhs, f32 rhs)
{
    lhs += rhs;
    return lhs;
}

inline Vec4 operator-(Vec4 lhs, Vec4 const& rhs)
{
    lhs -= rhs;
    return lhs;
}

inline Vec4 operator-(Vec4 lhs, f32 rhs)
{
    lhs -= rhs;
    return lhs;
}

inline Vec4 operator*(Vec4 lhs, f32 rhs)
{
    lhs *= rhs;
    return lhs;
}

inline Vec4 operator*(Vec4 lhs, Vec4 const& rhs)
{
    lhs *= rhs;
    return lhs;
}

inline Vec4 operator/(Vec4 lhs, f32 rhs)
{
    lhs /= rhs;
    return lhs;
}

inline Vec4 operator/(Vec4 lhs, Vec4 const& rhs)
{
    lhs /= rhs;
    return lhs;
}

inline bool operator==(Vec4 const& lhs, Vec4 const& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

inline bool operator!=(Vec4 const& lhs, Vec4 const& rhs)
{
    return !operator==(lhs, rhs);
}

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

inline f32 sign(f32 v)
{
    return v < 0.f ? -1.f : (v == 0.f ? 0.f : 1.f);
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

inline Vec3 smoothMove(Vec3 const& from, Vec3 const& to, f32 amount, f32 deltaTime)
{
    return lerp(from, to, 1.f-expf(-amount * deltaTime));
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
    return Vec3(snap(val.xy + Vec2(multiple * 0.5f), multiple), val.z);
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
    return Vec2(min(a.x, b.x), min(a.y, b.y));
}

inline Vec3 min(Vec3 const& a, Vec3 const& b)
{
    return Vec3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
}

inline Vec4 min(Vec4 const& a, Vec4 const& b)
{
    return Vec4(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w));
}

inline f32 max(f32 a, f32 b)
{
    return a > b ? a : b;
}

inline Vec2 max(Vec2 a, Vec2 b)
{
    return Vec2(max(a.x, b.x), max(a.y, b.y));
}

inline Vec3 max(Vec3 const& a, Vec3 const& b)
{
    return Vec3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
}

inline Vec4 max(Vec4 const& a, Vec4 const& b)
{
    return Vec4(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w));
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

struct Mat3
{
    Vec3 value[3];

    Mat3() {}
    explicit Mat3(f32 n)
    {
        value[0] = Vec3(n, 0.f, 0.f);
        value[1] = Vec3(0.f, n, 0.f);
        value[2] = Vec3(0.f, 0.f, n);
    }
    explicit Mat3(struct Mat4 const& m);
    Mat3(Vec3 const& basis1, Vec3 const& basis2, Vec3 const& basis3)
    {
        value[0] = basis1;
        value[1] = basis2;
        value[2] = basis3;
    }

    f32* valuePtr() const { return (f32*)value; }

    Vec3& operator[](u32 row)
    {
        return value[row];
    }

    Vec3 const& operator[](u32 row) const
    {
        return value[row];
    }

    void operator=(const Mat3& rhs)
    {
        value[0] = rhs[0];
        value[1] = rhs[1];
        value[2] = rhs[2];
    }

    Mat3& operator*=(f32 rhs)
    {
        value[0] *= rhs;
        value[1] *= rhs;
        value[2] *= rhs;
        return *this;
    }

    Mat3& operator/=(f32 rhs)
    {
        value[0] /= rhs;
        value[1] /= rhs;
        value[2] /= rhs;
        return *this;
    }

    Mat3& operator+=(f32 rhs)
    {
        value[0] += Vec3(rhs);
        value[1] += Vec3(rhs);
        value[2] += Vec3(rhs);
        return *this;
    }

    Mat3& operator-=(f32 rhs)
    {
        value[0] -= Vec3(rhs);
        value[1] -= Vec3(rhs);
        value[2] -= Vec3(rhs);
        return *this;
    }

    Mat3& operator-=(const Mat3& rhs)
    {
        value[0] -= rhs[0];
        value[1] -= rhs[1];
        value[2] -= rhs[2];
        return *this;
    }

    Mat3& operator+=(const Mat3& rhs)
    {
        value[0] += rhs[0];
        value[1] += rhs[1];
        value[2] += rhs[2];
        return *this;
    }

    Mat3& operator*=(const Mat3& rhs)
    {
        Mat3 result;
        for (u32 i=0; i<3; ++i)
        {
            for (u32 j=0; j<3; ++j)
            {
                result[i][j] = 0.f;
                for (u32 k=0; k<3; ++k)
                {
                    result[i][j] += value[k][j] * rhs[i][k];
                }
            }
        }
        *this = result;
        return *this;
    }

    static Mat3 faceDirection(Vec3 const& forwardVector, Vec3 const& upVector);
};

Mat3 inverse(Mat3 const& m);
Mat3 transpose(Mat3 const& m);
Mat3 inverseTranspose(Mat3 const& m);

inline Mat3 operator*(Mat3 lhs, f32 rhs)
{
    lhs *= rhs;
    return lhs;
}

inline Mat3 operator+(Mat3 lhs, f32 rhs)
{
    lhs += rhs;
    return lhs;
}

inline Mat3 operator-(Mat3 lhs, f32 rhs)
{
    lhs -= rhs;
    return lhs;
}

inline Mat3 operator/(Mat3 lhs, f32 rhs)
{
    lhs /= rhs;
    return lhs;
}

inline Mat3 operator*(Mat3 lhs, const Mat3& rhs)
{
    lhs *= rhs;
    return lhs;
}

inline Mat3 operator+(Mat3 lhs, const Mat3& rhs)
{
    lhs += rhs;
    return lhs;
}

inline Mat3 operator-(Mat3 lhs, const Mat3& rhs)
{
    lhs -= rhs;
    return lhs;
}

inline Vec3 operator*(const Mat3& m, const Vec3& v)
{
    return Vec3(
        m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2],
        m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2],
        m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2]);
}

struct alignas(16) Mat4
{
    union
    {
        Vec4 value[4];
        f32 f[16];
    };

    Mat4() {}
    explicit Mat4(f32 n)
    {
        value[0] = Vec4(n,   0.f, 0.f, 0.f);
        value[1] = Vec4(0.f, n,   0.f, 0.f);
        value[2] = Vec4(0.f, 0.f, n,   0.f);
        value[3] = Vec4(0.f, 0.f, 0.f, n  );
    }
    explicit Mat4(const f32 * const m)
    {
        for (u32 i=0; i<16; ++i) f[i] = m[i];
    }
    explicit Mat4(Mat3 const& m)
    {
        value[0] = Vec4(m[0], 0.f);
        value[1] = Vec4(m[1], 0.f);
        value[2] = Vec4(m[2], 0.f);
        value[3] = { 0, 0, 0, 1 };
    }
    Mat4(Vec3 const& basis1, Vec3 const& basis2, Vec3 const& basis3)
    {
        value[0] = Vec4(basis1, 0.f);
        value[1] = Vec4(basis2, 0.f);
        value[2] = Vec4(basis3, 0.f);
        value[3] = Vec4(0.f, 0.f, 0.f, 1.f);
    }

    explicit Mat4(PxMat44 const& m) : Mat4(m.front()) {}
    explicit Mat4(struct Quat const& q);

    f32* valuePtr() const { return (f32*)f; }

    static Mat4 ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near=0.f, f32 far=100.f);
    static Mat4 perspective(f32 fov, f32 aspect, f32 near, f32 far);
    static Mat4 lookAt(Vec3 const& eye, Vec3 const& center, const Vec3 &up);
    static Mat4 rotation(f32 rotX, f32 rotY, f32 rotZ);
    static Mat4 rotation(Vec3 const& angles);
    static Mat4 rotation(f32 angle, Vec3 axis);
    static Mat4 rotationX(f32 angle);
    static Mat4 rotationY(f32 angle);
    static Mat4 rotationZ(f32 angle);
    static Mat4 scaling(Vec3 const& scale);
    static Mat4 translation(Vec3 const& trans);
    static Mat4 faceDirection(Vec3 const& forwardVector, Vec3 const& upVector);

    Vec3 const& xAxis() const
    {
        return value[0].xyz;
    }

    Vec3 const& yAxis() const
    {
        return value[1].xyz;
    }

    Vec3 const& zAxis() const
    {
        return value[2].xyz;
    }

    Vec3 position() const
    {
        return value[3].xyz;
    }

    Vec3 scale() const
    {
        return Vec3(length(value[0].xyz), length(value[1].xyz), length(value[2].xyz));
    }

    Mat4 rotation() const
    {
        Mat4 m = *this;
        Vec3 s = scale();
        m[0] /= s.x;
        m[1] /= s.y;
        m[2] /= s.z;
        m[3] = Vec4(0, 0, 0, 1);
        return m;
    }

    // NOTE: [] operator accesses row (0..4)
    Vec4& operator[](u32 row)
    {
        return value[row];
    }

    const Vec4& operator[](u32 row) const
    {
        return value[row];
    }

    // NOTE: () operator accesses value (0..16)
    f32& operator()(u32 index)
    {
        return f[index];
    }

    f32 const& operator()(u32 index) const
    {
        return f[index];
    }

    void operator=(const Mat4& rhs)
    {
        value[0] = rhs[0];
        value[1] = rhs[1];
        value[2] = rhs[2];
        value[3] = rhs[3];
    }

    Mat4& operator*=(f32 rhs)
    {
        value[0] *= rhs;
        value[1] *= rhs;
        value[2] *= rhs;
        value[3] *= rhs;
        return *this;
    }

    Mat4& operator/=(f32 rhs)
    {
        value[0] /= rhs;
        value[1] /= rhs;
        value[2] /= rhs;
        value[3] /= rhs;
        return *this;
    }

    Mat4& operator+=(f32 rhs)
    {
        value[0] += Vec4(rhs);
        value[1] += Vec4(rhs);
        value[2] += Vec4(rhs);
        value[3] += Vec4(rhs);
        return *this;
    }

    Mat4& operator-=(f32 rhs)
    {
        value[0] -= Vec4(rhs);
        value[1] -= Vec4(rhs);
        value[2] -= Vec4(rhs);
        value[3] -= Vec4(rhs);
        return *this;
    }

    Mat4& operator-=(Mat4 const& rhs)
    {
        value[0] -= rhs[0];
        value[1] -= rhs[1];
        value[2] -= rhs[2];
        value[3] -= rhs[3];
        return *this;
    }

    Mat4& operator+=(Mat4 const& rhs)
    {
        value[0] += rhs[0];
        value[1] += rhs[1];
        value[2] += rhs[2];
        value[3] += rhs[3];
        return *this;
    }

#if 1
    Mat4& operator*=(Mat4 const& rhs)
    {
        Mat4 result;
        for (u32 i=0; i<4; ++i)
        {
            for (u32 j=0; j<4; ++j)
            {
                result[i][j] = 0.f;
                for (u32 k=0; k<4; ++k)
                {
                    result[i][j] += value[k][j] * rhs[i][k];
                }
            }
        }
        *this = result;
        return *this;
    }
#else
    Mat4& operator*=(Mat4 const& rhs)
    {
        Mat4 result;
        __m128 row1 = _mm_load_ps(&f[0]);
        __m128 row2 = _mm_load_ps(&f[4]);
        __m128 row3 = _mm_load_ps(&f[8]);
        __m128 row4 = _mm_load_ps(&f[12]);
        for(u32 i=0; i<4; ++i)
        {
            __m128 brod1 = _mm_set1_ps(rhs(4*i + 0));
            __m128 brod2 = _mm_set1_ps(rhs(4*i + 1));
            __m128 brod3 = _mm_set1_ps(rhs(4*i + 2));
            __m128 brod4 = _mm_set1_ps(rhs(4*i + 3));
            __m128 row = _mm_add_ps(
                            _mm_add_ps(_mm_mul_ps(brod1, row1), _mm_mul_ps(brod2, row2)),
                            _mm_add_ps( _mm_mul_ps(brod3, row3), _mm_mul_ps(brod4, row4)));
            _mm_store_ps(&result(4*i), row);
        }
        *this = result;
        return *this;
    }
#endif
};

Mat4 inverse(Mat4 const& m);
Mat4 transpose(Mat4 const& m);

inline Mat4 operator*(Mat4 lhs, f32 rhs)
{
    lhs *= rhs;
    return lhs;
}

inline Mat4 operator+(Mat4 lhs, f32 rhs)
{
    lhs += rhs;
    return lhs;
}

inline Mat4 operator-(Mat4 lhs, f32 rhs)
{
    lhs -= rhs;
    return lhs;
}

inline Mat4 operator/(Mat4 lhs, f32 rhs)
{
    lhs /= rhs;
    return lhs;
}

inline Mat4 operator*(Mat4 lhs, Mat4 const& rhs)
{
    lhs *= rhs;
    return lhs;
}

inline Mat4 operator+(Mat4 lhs, Mat4 const& rhs)
{
    lhs += rhs;
    return lhs;
}

inline Mat4 operator-(Mat4 lhs, Mat4 const& rhs)
{
    lhs -= rhs;
    return lhs;
}

inline Vec4 operator*(Mat4 const& m, Vec4 const& v)
{
    return Vec4(
        m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2] + m[3][0] * v[3],
        m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2] + m[3][1] * v[3],
        m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2] + m[3][2] * v[3],
        m[0][3] * v[0] + m[1][3] * v[1] + m[2][3] * v[2] + m[3][3] * v[3]);
}

struct Quat
{
    union
    {
        struct
        {
            f32 x, y, z, w;
        };

        f32 data[4];
    };

    Quat(f32 w, f32 x, f32 y, f32 z) : x(x), y(y), z(z), w(w) {}
    Quat() : x(0.f), y(0.f), z(0.f), w(1.f) {}
    Quat(Quat const& other) :x(other.x), y(other.y), z(other.z), w(other.w) {}
    Quat(Mat3 const& m);

    void operator=(Quat const& rhs)
    {
        x = rhs.x;
        y = rhs.y;
        z = rhs.z;
        w = rhs.w;
    }

    f32& operator[](u32 index)
    {
        return data[index];
    }

    const f32& operator[](u32 index) const
    {
        return data[index];
    }

    inline static Quat rotationAxis(f32 angle, Vec3 const& axis);
    inline static Quat fromEulerAngles(Vec3 const& eulerAngles);
    inline static Quat rotationX(f32 angle);
    inline static Quat rotationY(f32 angle);
    inline static Quat rotationZ(f32 angle);

    Quat& operator*=(Quat const& q)
    {
        Quat p = *this;
		w = p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z;
		x = p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y;
		y = p.w * q.y + p.y * q.w + p.z * q.x - p.x * q.z;
		z = p.w * q.z + p.z * q.w + p.x * q.y - p.y * q.x;
        return *this;
    }
};

inline Quat operator*(Quat lhs, Quat const& rhs)
{
    lhs *= rhs;
    return lhs;
}

inline Vec3 screenToWorldRay(Vec2 screenPos, Vec2 screenSize, Mat4 const& view, Mat4 const& projection)
{
    Vec3 pos((2.f * screenPos.x)/screenSize.x - 1.f, (1.f - (2.f * screenPos.y)/screenSize.y), 1.f);
    Vec4 rayClip(pos.x, pos.y, 0.f, 1.f);
    Vec4 rayEye = inverse(projection) * rayClip;
    rayEye = Vec4(rayEye.x, rayEye.y, -1.f, 0.f);
    Vec3 rayWorld = normalize((inverse(view) * rayEye).xyz);
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
    Mat4 mvp = viewProj * Mat4::translation(pos);
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

inline PxVec3 convert(Vec3 const& v)
{
    return PxVec3(v.x, v.y, v.z);
}

inline PxQuat convert(Quat const& q)
{
    return PxQuat(q.x, q.y, q.z, q.w);
}

inline PxTransform convert(Mat4 const& m)
{
    return PxTransform(convert(m.position()), convert(Quat(Mat3(m.rotation()))));
}
