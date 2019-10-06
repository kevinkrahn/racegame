#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/mine.h"

class WExplosiveMine : public Weapon
{
public:
    WExplosiveMine()
    {
        name = "Exploding Mine";
        description = "It explodes";
        icon = "mine_icon";
        price = 700;
    }

    u32 fire(Scene* scene, Vehicle* vehicle) const override
    {
        PxRaycastBuffer hit;
        glm::vec3 down = convert(vehicle->getRigidBody()->getGlobalPose().q.getBasisVector2() * -1.f);
        if (scene->raycastStatic(vehicle->getPosition(), down, 2.f, &hit,
                    COLLISION_FLAG_GROUND | COLLISION_FLAG_TRACK))
        {
            glm::vec3 pos = convert(hit.block.position);
            glm::vec3 up = convert(hit.block.normal);
            glm::mat4 m(1.f);
            m[0] = glm::vec4(vehicle->getForwardVector(), m[0].w);
            m[1] = glm::vec4(glm::normalize(
                        glm::cross(up, glm::vec3(m[0]))), m[1].w);
            m[2] = glm::vec4(glm::normalize(
                    glm::cross(glm::vec3(m[0]), glm::vec3(m[1]))), m[2].w);
            scene->addEntity(new Mine(glm::translate(glm::mat4(1.f), pos) * m, vehicle->vehicleIndex));

            // TODO: play sound
            return 1;
        }
        // TODO: play "cannot do that now" sound
        return 0;
    }
};
