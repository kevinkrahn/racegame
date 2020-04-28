#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/oil.h"

class WOil : public Weapon
{
public:
    WOil()
    {
        info.name = "Oil";
        info.description = "Causes vehicles to loose traction.";
        info.icon = g_res.getTexture("icon_oil");
        info.price = 1000;
        info.maxUpgradeLevel = 4;
        info.weaponType = WeaponInfo::REAR_WEAPON;
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

        PxRaycastBuffer hit;
        glm::vec3 down = convert(vehicle->getRigidBody()->getGlobalPose().q.getBasisVector2() * -1.f);
        if (!scene->raycastStatic(vehicle->getPosition(), down, 2.f, &hit, COLLISION_FLAG_TRACK))
        {
            g_audio.playSound(g_res.getSound("nono"), SoundType::GAME_SFX);
            return;
        }

        glm::vec3 pos = convert(hit.block.position);
        Oil* oil = new Oil(pos);
        scene->addEntity(oil);
        g_audio.playSound3D(g_res.getSound("oil"), SoundType::GAME_SFX,
                vehicle->getPosition(), false, 1.f, 0.9f);
        vehicle->addIgnoredGroundSpot(oil);

        ammo -= 1;
    }

    bool shouldUse(Scene* scene, Vehicle* vehicle) override
    {
        return vehicle->isOnTrack && vehicle->getAI()->aggression > 0.f
                && ammo > 0 && vehicle->getForwardSpeed() > 10.f
                && irandom(scene->randomSeries, 0, 140 + (i32)((1.f - vehicle->getAI()->aggression) * 140)) < 2;
    }
};
