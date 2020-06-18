#include "math.h"

Vec2::Vec2(Vec3 const& v) : x(v.x), y(v.y) {}
Vec3::Vec3(Vec4 const& v) : x(v.x), y(v.y), z(v.z) {}

Mat4 Mat4::ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
{
    Mat4 result(1.f);

    result[0][0] = 2.f / (right - left);
    result[1][1] = 2.f / (top - bottom);
    result[2][2] = - 1.f / (far - near);
    result[3][0] = -(right + left) / (right - left);
    result[3][1] = -(top + bottom) / (top - bottom);
    result[3][2] = -near / (far - near);

    return result;
}

Mat4 Mat4::perspective(f32 fov, f32 aspect, f32 near, f32 far)
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

Mat4 Mat4::lookAt(Vec3 const&eye, Vec3 const& center, Vec3 const& up)
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

Mat4 Mat4::rotation(f32 angle, Vec3 axis)
{
    f32 c = cosf(angle);
    f32 s = sinf(angle);
    axis = normalize(axis);
    Vec3 tmp = axis * (1.f - c);

    Mat4 rot(1.f);
    rot[0][0] = c + tmp.x * axis.x;
    rot[0][1] = 0 + tmp.x * axis.y + s * axis.z;
    rot[0][2] = 0 + tmp.x * axis.z - s * axis.y;
    rot[1][0] = 0 + tmp.y * axis.x - s * axis.z;
    rot[1][1] = c + tmp.y * axis.y;
    rot[1][2] = 0 + tmp.y * axis.z + s * axis.x;
    rot[2][0] = 0 + tmp.z * axis.x + s * axis.y;
    rot[2][1] = 0 + tmp.z * axis.y - s * axis.x;
    rot[2][2] = c + tmp.z * axis.z;

    return rot;
}

Mat4 Mat4::rotationX(f32 angle)
{
    f32 c = cosf(angle);
    f32 s = sinf(angle);

    Mat4 r(1.f);
    r[1][1] = c;
    r[1][2] = s;
    r[2][1] = -s;
    r[2][2] = c;

    return r;
}

Mat4 Mat4::rotationY(f32 angle)
{
    f32 c = cosf(angle);
    f32 s = sinf(angle);

    Mat4 r(1.f);
    r[0][0] = c;
    r[0][2] = s;
    r[2][0] = -s;
    r[2][2] = c;

    return r;
}

Mat4 Mat4::rotationZ(f32 angle)
{
    f32 c = cosf(angle);
    f32 s = sinf(angle);

    Mat4 r(1.f);
    r[0][0] = c;
    r[0][1] = s;
    r[1][0] = -s;
    r[1][1] = c;

    return r;
}

Mat4 Mat4::rotation(f32 rotX, f32 rotY, f32 rotZ)
{
    return Mat4::rotationX(rotX) * Mat4::rotationY(rotY) * Mat4::rotationZ(rotZ);
}

Mat4 Mat4::rotation(Vec3 const& angles)
{
    return Mat4::rotation(angles.x, angles.y, angles.z);
}

Mat4 Mat4::scaling(Vec3 const& scale)
{
    Mat4 r(0.f);
    r[0][0] = scale.x;
    r[1][1] = scale.y;
    r[2][2] = scale.z;
    r[3][3] = 1.f;

    return r;
}

Mat4 Mat4::translation(Vec3 const& trans)
{
    Mat4 r(1.f);
    r[3][0] = trans.x;
    r[3][1] = trans.y;
    r[3][2] = trans.z;

    return r;
}

Mat4 inverse(const Mat4& m)
{
    Mat4 inv;

    inv(0) = m(5)  * m(10) * m(15) -
             m(5)  * m(11) * m(14) -
             m(9)  * m(6)  * m(15) +
             m(9)  * m(7)  * m(14) +
             m(13) * m(6)  * m(11) -
             m(13) * m(7)  * m(10);

    inv(4) = -m(4)  * m(10) * m(15) +
              m(4)  * m(11) * m(14) +
              m(8)  * m(6)  * m(15) -
              m(8)  * m(7)  * m(14) -
              m(12) * m(6)  * m(11) +
              m(12) * m(7)  * m(10);

    inv(8) = m(4)  * m(9)  * m(15) -
             m(4)  * m(11) * m(13) -
             m(8)  * m(5)  * m(15) +
             m(8)  * m(7)  * m(13) +
             m(12) * m(5)  * m(11) -
             m(12) * m(7)  * m(9);

    inv(12) = -m(4) * m(9)  * m(14) +
              m(4)  * m(10) * m(13) +
              m(8)  * m(5)  * m(14) -
              m(8)  * m(6)  * m(13) -
              m(12) * m(5)  * m(10) +
              m(12) * m(6)  * m(9);

    inv(1) = -m(1)  * m(10) * m(15) +
              m(1)  * m(11) * m(14) +
              m(9)  * m(2)  * m(15) -
              m(9)  * m(3)  * m(14) -
              m(13) * m(2)  * m(11) +
              m(13) * m(3)  * m(10);

    inv(5) = m(0)  * m(10) * m(15) -
             m(0)  * m(11) * m(14) -
             m(8)  * m(2)  * m(15) +
             m(8)  * m(3)  * m(14) +
             m(12) * m(2)  * m(11) -
             m(12) * m(3)  * m(10);

    inv(9) = -m(0)  * m(9)  * m(15) +
              m(0)  * m(11) * m(13) +
              m(8)  * m(1)  * m(15) -
              m(8)  * m(3)  * m(13) -
              m(12) * m(1)  * m(11) +
              m(12) * m(3)  * m(9);

    inv(13) = m(0)  * m(9)  * m(14) -
              m(0)  * m(10) * m(13) -
              m(8)  * m(1)  * m(14) +
              m(8)  * m(2)  * m(13) +
              m(12) * m(1)  * m(10) -
              m(12) * m(2)  * m(9);

    inv(2) = m(1)  * m(6) * m(15) -
             m(1)  * m(7) * m(14) -
             m(5)  * m(2) * m(15) +
             m(5)  * m(3) * m(14) +
             m(13) * m(2) * m(7) -
             m(13) * m(3) * m(6);

    inv(6) = -m(0)  * m(6) * m(15) +
              m(0)  * m(7) * m(14) +
              m(4)  * m(2) * m(15) -
              m(4)  * m(3) * m(14) -
              m(12) * m(2) * m(7) +
              m(12) * m(3) * m(6);

    inv(10) = m(0)  * m(5) * m(15) -
              m(0)  * m(7) * m(13) -
              m(4)  * m(1) * m(15) +
              m(4)  * m(3) * m(13) +
              m(12) * m(1) * m(7) -
              m(12) * m(3) * m(5);

    inv(14) = -m(0)  * m(5) * m(14) +
               m(0)  * m(6) * m(13) +
               m(4)  * m(1) * m(14) -
               m(4)  * m(2) * m(13) -
               m(12) * m(1) * m(6) +
               m(12) * m(2) * m(5);

    inv(3) = -m(1) * m(6) * m(11) +
              m(1) * m(7) * m(10) +
              m(5) * m(2) * m(11) -
              m(5) * m(3) * m(10) -
              m(9) * m(2) * m(7) +
              m(9) * m(3) * m(6);

    inv(7) = m(0) * m(6) * m(11) -
             m(0) * m(7) * m(10) -
             m(4) * m(2) * m(11) +
             m(4) * m(3) * m(10) +
             m(8) * m(2) * m(7) -
             m(8) * m(3) * m(6);

    inv(11) = -m(0) * m(5) * m(11) +
               m(0) * m(7) * m(9) +
               m(4) * m(1) * m(11) -
               m(4) * m(3) * m(9) -
               m(8) * m(1) * m(7) +
               m(8) * m(3) * m(5);

    inv(15) = m(0) * m(5) * m(10) -
              m(0) * m(6) * m(9) -
              m(4) * m(1) * m(10) +
              m(4) * m(2) * m(9) +
              m(8) * m(1) * m(6) -
              m(8) * m(2) * m(5);

    f32 det = m(0) * inv(0) + m(1) * inv(4) + m(2) * inv(8) + m(3) * inv(12);
    f32 invdet = 1.f / det;

    Mat4 invOut;
    for (u32 i = 0; i < 16; i++)
    {
        invOut(i) = inv(i) * invdet;
    }

    return invOut;
}

Mat4 transpose(const Mat4& m)
{
    Mat4 result;
    for (u32 i=0; i<4; ++i)
    {
        for (u32 j=0; j<4; ++j)
        {
            result[j][i] = m[i][j];
        }
    }
    return result;
}

Mat3::Mat3(Mat4 const& m)
{
    value[0] = Vec3(m[0]);
    value[1] = Vec3(m[1]);
    value[2] = Vec3(m[2]);
}

Mat3 inverse(Mat3 const& m)
{
    f32 det = m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) -
              m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
              m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    f32 invdet = 1.f / det;

    Mat3 out;
    out[0][0] = (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * invdet;
    out[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * invdet;
    out[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * invdet;
    out[1][0] = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) * invdet;
    out[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * invdet;
    out[1][2] = (m[1][0] * m[0][2] - m[0][0] * m[1][2]) * invdet;
    out[2][0] = (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * invdet;
    out[2][1] = (m[2][0] * m[0][1] - m[0][0] * m[2][1]) * invdet;
    out[2][2] = (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * invdet;

    return out;
}

Mat3 transpose(Mat3 const& m)
{
    Mat3 result;
    for (u32 i=0; i<3; ++i)
    {
        for (u32 j=0; j<3; ++j)
        {
            result[j][i] = m[i][j];
        }
    }
    return result;
}

Mat3 inverseTranspose(Mat3 const& m)
{
    f32 det = m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) -
              m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
              m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);

    f32 invdet = 1.f / det;

    Mat3 out;
    out[0][0] =  (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * invdet;
    out[1][0] = -(m[0][1] * m[2][2] - m[0][2] * m[2][1]) * invdet;
    out[2][0] =  (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * invdet;
    out[0][1] = -(m[1][0] * m[2][2] - m[1][2] * m[2][0]) * invdet;
    out[1][1] =  (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * invdet;
    out[2][1] = -(m[0][0] * m[1][2] - m[1][0] * m[0][2]) * invdet;
    out[0][2] =  (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * invdet;
    out[1][2] = -(m[0][0] * m[2][1] - m[2][0] * m[0][1]) * invdet;
    out[2][2] =  (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * invdet;

    return out;
}

