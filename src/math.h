#pragma once

#include "misc.h"

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

inline PxTransform convert(glm::mat4 m)
{
    glm::vec3 pos(m[3]);
    glm::quat rot = glm::quat_cast(glm::mat3(rotationOf(m)));
    return PxTransform(convert(pos), PxQuat(rot.x, rot.y, rot.z, rot.w));
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
    std::ostream& operator << (std::ostream& lhs, glm::vec3 const& rhs)
    {
        return lhs << "{ " << rhs.x << ", " << rhs.y << ", " << rhs.z << " }";
    }

    std::ostream& operator << (std::ostream& lhs, glm::vec4 const& rhs)
    {
        return lhs << "{ " << rhs.x << ", " << rhs.y << ", " << rhs.z << ", " << rhs.w << " }";
    }
};

inline f32 pointDirection(glm::vec2 v1, glm::vec2 v2)
{
    glm::vec2 d = v2 - v1;
    return -atan2f(d.x, d.y);
}
