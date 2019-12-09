#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"

class WBlaster : public Weapon
{
public:
    WBlaster()
    {
        info.name = "Blaster";
        info.description = "High damage split into two shots.\n";
        info.icon = &g_res.textures->icon_blaster;
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
        scene->addEntity(new Projectile(pos + forward * 2.5f + right * 0.8f,
                vel, zAxisOf(transform), vehicle->vehicleIndex, Projectile::BLASTER));
        scene->addEntity(new Projectile(pos + forward * 2.5f - right * 0.8f,
                vel, zAxisOf(transform), vehicle->vehicleIndex, Projectile::BLASTER));
        g_audio.playSound3D(&g_res.sounds->blaster,
                SoundType::GAME_SFX, vehicle->getPosition(), false,
                random(scene->randomSeries, 0.95f, 1.05f), 1.f);

        ammo -= 1;
    }
};
