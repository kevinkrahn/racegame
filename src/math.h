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

#include <PxPhysicsAPI.h>
using namespace physx;

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
    glm::quat rot = glm::quat_cast(m);
    return PxTransform(PxVec3(pos.x, pos.y, pos.z), PxQuat(rot.x, rot.y, rot.z, rot.w));
}

inline f32 square(f32 v)
{
    return v * v;
}
