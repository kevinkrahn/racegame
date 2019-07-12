#pragma once

#include "misc.h"
#include "math.h"
#include "renderable.h"
#include "entity.h"
#include "gl.h"

class Track : public Renderable, public Entity
{
    glm::vec3 getPointOnBezierCurve(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, f32 t) const
    {
        f32 u = 1.f - t;
        f32 t2 = t * t;
        f32 u2 = u * u;
        f32 u3 = u2 * u;
        f32 t3 = t2 * t;
        return glm::vec3(u3 * p0 + (3.f * u2 * t) * p1 + (3.f * u * t2) * p2 + t3 * p3);
    }

    void onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime) override;
};
