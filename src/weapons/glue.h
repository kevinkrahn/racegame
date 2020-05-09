#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/glue.h"

class WGlue : public Weapon
{
public:
    WGlue()
    {
        info.name = "Glue";
        info.description = "Force your opponents to slow down!";
        info.icon = g_res.getTexture("icon_glue");
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
        Glue* glue = new Glue(pos);
        scene->addEntity(glue);
        g_audio.playSound3D(g_res.getSound("glue"), SoundType::GAME_SFX,
                vehicle->getPosition(), false, 1.f, 0.9f);
        vehicle->getVehiclePhysics()->addIgnoredGroundSpot(glue);

        ammo -= 1;
    }

    bool shouldUse(Scene* scene, Vehicle* vehicle) override
    {
        return vehicle->isOnTrack && vehicle->getAI()->aggression > 0.f
                && ammo > 0 && vehicle->getForwardSpeed() > 10.f
                && irandom(scene->randomSeries, 0, 140 + (i32)((1.f - vehicle->getAI()->aggression) * 140)) < 2;
    }
};
