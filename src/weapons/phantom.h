#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"

class WPhantom : public Weapon
{
    Mesh* mesh;
    Mesh* meshBarrel;
    f32 recoil = 0.f;
    f32 recoilVel = 0.f;

public:
    WPhantom()
    {
        info.name = "Phantom";
        info.description = "Can hit multiple targets with a single shot by passing through vehicles!";
        info.icon = g_res.getTexture("icon_phantom");
        info.price = 1100;
        info.maxUpgradeLevel = 5;
        info.weaponType = WeaponType::FRONT_WEAPON;
        info.tags[0] = "hood-1";
        info.tags[1] = "narrow";

        loadModelData("weapon_blaster");
        Model* model = g_res.getModel("weapon_blaster");
        mesh = model->getMeshByName("blaster.BlasterBase");
        meshBarrel = model->getMeshByName("blaster.BlasterBarrel");
    }

    void update(Scene* scene, Vehicle* vehicle, bool fireBegin, bool fireHold,
            f32 deltaTime) override
    {
        recoilVel = min(recoilVel + deltaTime * 30.f, 10.f);
        recoil = clamp(recoil + recoilVel * deltaTime, -0.5f, 0.f);

        if (!fireBegin)
        {
            return;
        }

        if (ammo == 0)
        {
            outOfAmmo(vehicle);
            return;
        }

        Mat4 transform = vehicle->getTransform();
        f32 minSpeed = 45.f;
        Vec3 vel = convert(vehicle->getRigidBody()->getLinearVelocity())
            + vehicle->getForwardVector() * minSpeed;
        if (lengthSquared(vel) < square(minSpeed))
        {
            vel = normalize(vel) * minSpeed;
        }

        Vec3 pos1 = Vec3(transform * mountTransform * Vec4(projectileSpawnPoints[0], 1.f));
        Vec3 pos2 = Vec3(transform * mountTransform * Vec4(projectileSpawnPoints[1], 1.f));
        scene->addEntity(new Projectile(pos1, vel, transform.zAxis(), vehicle->vehicleIndex,
                    Projectile::PHANTOM));
        scene->addEntity(new Projectile(pos2, vel, transform.zAxis(), vehicle->vehicleIndex,
                    Projectile::PHANTOM));
        g_audio.playSound3D(g_res.getSound("blaster"),
                SoundType::GAME_SFX, vehicle->getPosition(), false,
                random(scene->randomSeries, 0.95f, 1.05f), 1.f);

        ammo -= 1;
        recoilVel = -8.f;
    }

    void render(class RenderWorld* rw, Mat4 const& vehicleTransform,
            VehicleConfiguration const& config, VehicleData const& vehicleData) override
    {
        Material* mat = g_res.getMaterial("plastic");
        mat->draw(rw, vehicleTransform * mountTransform, mesh, 2);
        mat->draw(rw, vehicleTransform * mountTransform
                * Mat4::translation(Vec3(recoil, 0.f, 0.f)), meshBarrel, 2);
    }
};
