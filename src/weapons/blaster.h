#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"

class WBlaster : public Weapon
{
public:
    WBlaster()
    {
        name = "Blaster";
        description = "It shoots";
        icon = "blaster_icon";
        price = 500;
    }

    u32 fire(Scene* scene, Vehicle* vehicle) const override
    {
        glm::vec3 forward = vehicle->getForwardVector();
        glm::vec3 right = vehicle->getRightVector();
        glm::mat4 transform = vehicle->getTransform();
        f32 minSpeed = 40.f;
        glm::vec3 vel = convert(vehicle->getRigidBody()->getLinearVelocity())
            + vehicle->getForwardVector() * minSpeed;
        if (glm::length2(vel) < square(minSpeed))
        {
            vel = glm::normalize(vel) * minSpeed;
        }
        glm::vec3 pos = translationOf(transform);
        scene->addEntity(new Projectile(pos + forward * 3.f + right * 0.8f,
                vel, zAxisOf(transform), vehicle->vehicleIndex));
        scene->addEntity(new Projectile(pos + forward * 3.f - right * 0.8f,
                vel, zAxisOf(transform), vehicle->vehicleIndex));
        // TODO: play sound
        return 1;
    }
};
