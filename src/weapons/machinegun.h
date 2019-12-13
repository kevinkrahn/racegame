#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"
#include "../mesh_renderables.h"

class WMachineGun : public Weapon
{
    f32 repeatTimer = 0.f;

public:
    WMachineGun()
    {
        info.name = "Machine Gun";
        info.description = "Low damage but high rate of fire.";
        info.icon = &g_res.textures->icon_mg;
        info.price = 1000;
        info.maxUpgradeLevel = 5;
        info.weaponType = WeaponInfo::FRONT_WEAPON;

        ammoUnitCount = 12;
        fireMode = FireMode::CONTINUOUS;

        loadSceneData("minigun.Minigun");
    }

    void update(Scene* scene, Vehicle* vehicle, bool fireBegin, bool fireHold,
            f32 deltaTime) override
    {
        if (!fireHold)
        {
            return;
        }

        if (ammo == 0)
        {
            if (fireBegin)
            {
                outOfAmmo(vehicle);
            }
            return;
        }

        if (repeatTimer > 0.f)
        {
            repeatTimer = glm::max(repeatTimer - deltaTime, 0.f);
            return;
        }

        glm::mat4 transform = vehicle->getTransform();
        f32 minSpeed = 90.f;
        glm::vec3 vel = convert(vehicle->getRigidBody()->getLinearVelocity())
            + vehicle->getForwardVector() * minSpeed;
        if (glm::length2(vel) < square(minSpeed))
        {
            vel = glm::normalize(vel) * minSpeed;
        }
        glm::vec3 pos = transform * mountTransform * glm::vec4(projectileSpawnPoint, 1.f);
        scene->addEntity(new Projectile(pos,
                vel, zAxisOf(transform), vehicle->vehicleIndex, Projectile::BULLET));

        g_audio.playSound3D(&g_res.sounds->mg2,
                SoundType::GAME_SFX, vehicle->getPosition(), false,
                random(scene->randomSeries, 0.95f, 1.05f), 1.f,
                random(scene->randomSeries, -0.05f, 0.05f));

        ammo -= 1;

        repeatTimer = 0.09f;
    }

    void render(class RenderWorld* rw, glm::mat4 const& vehicleTransform,
            VehicleConfiguration const& config, VehicleData const& vehicleData) override
    {
        rw->push(LitRenderable(g_res.getMesh("minigun.Minigun"), vehicleTransform * mountTransform));
    }
};
