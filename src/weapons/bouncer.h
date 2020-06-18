#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"
#include "../billboard.h"

class WBouncer : public Weapon
{
    Mesh* mesh;
    f32 glow = 0.f;

public:
    WBouncer()
    {
        info.name = "Bouncer";
        info.description = "Medium damage. Bounces off obstacles and follows slopes.";
        info.icon = g_res.getTexture("icon_bouncer");
        info.price = 800;
        info.maxUpgradeLevel = 5;
        info.weaponType = WeaponType::FRONT_WEAPON;

        loadModelData("weapon_bouncer");
        mesh = g_res.getModel("weapon_bouncer")->getMeshByName("bouncer.Bouncer");
    }

    void update(Scene* scene, Vehicle* vehicle, bool fireBegin, bool fireHold,
            f32 deltaTime) override
    {
        if (ammo > 0)
        {
            glow = min(glow + deltaTime * 3.f, 1.f);
        }

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
        f32 minSpeed = 34.f;
        Vec3 vel = convert(vehicle->getRigidBody()->getLinearVelocity())
            + vehicle->getForwardVector() * minSpeed;
        if (length2(vel) < square(minSpeed))
        {
            vel = normalize(vel) * minSpeed;
        }
        Vec3 pos = transform * mountTransform * Vec4(projectileSpawnPoints[0], 1.f);
        scene->addEntity(new Projectile(pos,
                vel, zAxisOf(transform), vehicle->vehicleIndex, Projectile::BOUNCER));
        g_audio.playSound3D(g_res.getSound("bouncer_fire"),
                SoundType::GAME_SFX, vehicle->getPosition(), false,
                random(scene->randomSeries, 0.95f, 1.05f), 0.9f);

        ammo -= 1;
        glow = 0.f;
    }

    void render(class RenderWorld* rw, Mat4 const& vehicleTransform,
            VehicleConfiguration const& config, VehicleData const& vehicleData) override
    {
        Material* mat = g_res.getMaterial("plastic");
        mat->draw(rw, vehicleTransform * mountTransform, mesh, 2);
        Vec3 pos = vehicleTransform * mountTransform *
            Vec4(projectileSpawnPoints[0] + Vec3(0.01f, 0.f, 0.05f), 1.f);
        drawBillboard(rw, g_res.getTexture("bouncer_projectile"),
                    pos, Vec4(1.f), 0.7f * glow, 0.f, false);
    }
};
