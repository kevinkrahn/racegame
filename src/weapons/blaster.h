#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"
#include "../mesh_renderables.h"

class WBlaster : public Weapon
{
    Mesh* mesh;
    Mesh* meshBarrel;
    f32 recoil = 0.f;
    f32 recoilVel = 0.f;

public:
    WBlaster()
    {
        info.name = "Blaster";
        info.description = "High damage split into two shots.\n";
        info.icon = g_res.getTexture("icon_blaster");
        info.price = 800;
        info.maxUpgradeLevel = 5;
        info.weaponType = WeaponInfo::FRONT_WEAPON;

        loadSceneData("blaster.Blaster");
        mesh = g_res.getMesh("blaster.BlasterBase");
        meshBarrel = g_res.getMesh("blaster.BlasterBarrel");
    }

    void update(Scene* scene, Vehicle* vehicle, bool fireBegin, bool fireHold,
            f32 deltaTime) override
    {
        recoilVel = glm::min(recoilVel + deltaTime * 30.f, 10.f);
        recoil = glm::clamp(recoil + recoilVel * deltaTime, -0.5f, 0.f);

        if (!fireBegin)
        {
            return;
        }

        if (ammo == 0)
        {
            outOfAmmo(vehicle);
            return;
        }

        glm::mat4 transform = vehicle->getTransform();
        f32 minSpeed = 40.f;
        glm::vec3 vel = convert(vehicle->getRigidBody()->getLinearVelocity())
            + vehicle->getForwardVector() * minSpeed;
        if (glm::length2(vel) < square(minSpeed))
        {
            vel = glm::normalize(vel) * minSpeed;
        }

        glm::vec3 pos1 = transform * mountTransform * glm::vec4(projectileSpawnPoints[0], 1.f);
        glm::vec3 pos2 = transform * mountTransform * glm::vec4(projectileSpawnPoints[1], 1.f);
        scene->addEntity(new Projectile(pos1, vel, zAxisOf(transform), vehicle->vehicleIndex,
                    Projectile::BLASTER));
        scene->addEntity(new Projectile(pos2, vel, zAxisOf(transform), vehicle->vehicleIndex,
                    Projectile::BLASTER));
        g_audio.playSound3D(&g_res.sounds->blaster,
                SoundType::GAME_SFX, vehicle->getPosition(), false,
                random(scene->randomSeries, 0.95f, 1.05f), 1.f);

        ammo -= 1;
        recoilVel = -8.f;
    }

    void render(class RenderWorld* rw, glm::mat4 const& vehicleTransform,
            VehicleConfiguration const& config, VehicleData const& vehicleData) override
    {
        rw->push(LitRenderable(mesh, vehicleTransform * mountTransform));
        rw->push(LitRenderable(meshBarrel, vehicleTransform * mountTransform
                    * glm::translate(glm::mat4(1.f), glm::vec3(recoil, 0.f, 0.f))));
    }
};
