#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"

class WJumpJets : public Weapon
{
    f32 t = 0.f;

public:
    WJumpJets()
    {
        info.name = "Jump Jet";
        info.description =
            "Propels the vehicle upwards with a single burst of energy.\n"
            "Useful for avoiding hazards.";
        info.icon = &g_res.textures->icon_jumpjet;
        info.price = 900;
        info.maxUpgradeLevel = 5;
        info.weaponType = WeaponInfo::REAR_WEAPON;
    }

    void update(Scene* scene, Vehicle* vehicle, bool fireBegin, bool fireHold,
            f32 deltaTime) override
    {
        t = glm::max(t - deltaTime, 0.f);
        if (t > 0.f)
        {
            PxVec3 vel = vehicle->getRigidBody()->getAngularVelocity();
            vel.x = 0.f;
            vel.y = 0.f;
            vehicle->getRigidBody()->setAngularVelocity(vel);
        }

        if (!fireBegin)
        {
            return;
        }

        if (vehicle->isInAir || ammo == 0)
        {
            // TODO: play no-no sound
            return;
        }
        g_audio.playSound3D(&g_res.sounds->jumpjet, SoundType::GAME_SFX,
                vehicle->getPosition(), false, 1.f, 0.9f);
        PxVec3 vel = vehicle->getRigidBody()->getAngularVelocity();
        vel.x = 0.f;
        vel.y = 0.f;
        vel.z *= 0.5f;
        vehicle->getRigidBody()->setAngularVelocity(vel);
        vehicle->getRigidBody()->addForce(PxVec3(0, 0, 12),
                //convert(zAxisOf(vehicle->getTransform()) * 12.f),
                PxForceMode::eVELOCITY_CHANGE);
        // TODO: add effect under the vehicle
        ammo -= 1;
        t = 0.1f;
    }

    bool shouldUse(Scene* scene, Vehicle* vehicle) override
    {
        return !vehicle->isInAir && ammo > 0 && vehicle->getForwardSpeed() > 13.f
                && vehicle->isNearHazard && irandom(scene->randomSeries, 0, 8) < 2;
    }
};
