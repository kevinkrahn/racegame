#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"

class WBouncer : public Weapon
{
public:
    WBouncer()
    {
        info.name = "Bouncer";
        info.description = "Medium damage.\nBounces off obstacles and follows slopes.";
        info.icon = "bouncer_icon";
        info.price = 800;
        info.maxUpgradeLevel = 5;
        info.weaponType = WeaponInfo::FRONT_WEAPON;
    }

    void update(Scene* scene, Vehicle* vehicle, bool fireBegin, bool fireHold,
            f32 deltaTime) override
    {
        if (!fireBegin)
        {
            return;
        }

        if (ammo == 0)
        {
            outOfAmmo(vehicle);
            return;
        }

        glm::vec3 forward = vehicle->getForwardVector();
        glm::mat4 transform = vehicle->getTransform();
        f32 minSpeed = 34.f;
        glm::vec3 vel = convert(vehicle->getRigidBody()->getLinearVelocity())
            + vehicle->getForwardVector() * minSpeed;
        if (glm::length2(vel) < square(minSpeed))
        {
            vel = glm::normalize(vel) * minSpeed;
        }
        glm::vec3 pos = translationOf(transform);
        scene->addEntity(new Projectile(pos + forward * 2.5f,
                vel, zAxisOf(transform), vehicle->vehicleIndex, Projectile::BOUNCER));
        g_audio.playSound3D(g_resources.getSound("bouncer_fire"),
                SoundType::GAME_SFX, vehicle->getPosition(), false,
                random(scene->randomSeries, 0.95f, 1.05f), 0.9f);

        ammo -= 1;
    }
};
