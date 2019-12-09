#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"

class WMissiles : public Weapon
{
public:
    WMissiles()
    {
        info.name = "Missiles";
        info.description = "Deals high damage and follows slopes!";
        info.icon = &g_res.textures->icon_missile;
        info.price = 1000;
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
        f32 minSpeed = 25.f;
        glm::vec3 vel = convert(vehicle->getRigidBody()->getLinearVelocity())
            + vehicle->getForwardVector() * minSpeed;
        if (glm::length2(vel) < square(minSpeed))
        {
            vel = glm::normalize(vel) * minSpeed;
        }
        glm::vec3 pos = translationOf(transform);
        scene->addEntity(new Projectile(pos + forward * 1.f,
                vel, zAxisOf(transform), vehicle->vehicleIndex, Projectile::MISSILE));
        g_audio.playSound3D(&g_res.sounds->missile,
                SoundType::GAME_SFX, vehicle->getPosition(), false,
                random(scene->randomSeries, 0.95f, 1.05f), 0.9f);

        ammo -= 1;
    }
};
