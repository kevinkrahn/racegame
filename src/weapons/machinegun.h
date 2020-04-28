#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"
#include "../mesh_renderables.h"

class WMachineGun : public Weapon
{
    f32 repeatTimer = 0.f;
    Mesh* mesh;
    Mesh* meshBarrel;
    f32 barrelSpin = 0.f;
    f32 barrelSpinSpeed = 0.f;

public:
    WMachineGun()
    {
        info.name = "Machine Gun";
        info.description = "Low damage but high rate of fire.";
        info.icon = g_res.getTexture("icon_mg");
        info.price = 1000;
        info.maxUpgradeLevel = 5;
        info.weaponType = WeaponInfo::FRONT_WEAPON;

        ammoUnitCount = 12;
        fireMode = FireMode::CONTINUOUS;

        loadSceneData("minigun.Minigun");
        mesh = g_res.getMesh("minigun.Minigun");
        meshBarrel = g_res.getMesh("minigun.MinigunBarrel");
    }

    void update(Scene* scene, Vehicle* vehicle, bool fireBegin, bool fireHold,
            f32 deltaTime) override
    {
        barrelSpinSpeed = glm::max(barrelSpinSpeed - deltaTime * 8.f, 0.f);
        barrelSpin += barrelSpinSpeed * deltaTime;

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

        barrelSpinSpeed = 12.f;

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
        glm::vec3 pos = transform * mountTransform * glm::vec4(projectileSpawnPoints[0], 1.f);
        scene->addEntity(new Projectile(pos,
                vel, zAxisOf(transform), vehicle->vehicleIndex, Projectile::BULLET));

        g_audio.playSound3D(g_res.getSound("mg2"),
                SoundType::GAME_SFX, vehicle->getPosition(), false,
                random(scene->randomSeries, 0.95f, 1.05f), 1.f,
                random(scene->randomSeries, -0.05f, 0.05f));

        ammo -= 1;

        repeatTimer = 0.09f;
    }

    void render(class RenderWorld* rw, glm::mat4 const& vehicleTransform,
            VehicleConfiguration const& config, VehicleData const& vehicleData) override
    {
        rw->push(LitRenderable(mesh, vehicleTransform * mountTransform));
        rw->push(LitRenderable(meshBarrel, vehicleTransform * mountTransform
                    * glm::translate(glm::mat4(1.f), glm::vec3(0.556007, 0, 0.397523f))
                    * glm::rotate(glm::mat4(1.f), barrelSpin, glm::vec3(1, 0, 0))));
    }
};
