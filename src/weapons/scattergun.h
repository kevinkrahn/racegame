#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"

class WScatterGun : public Weapon
{
    Mesh* mesh;
    Mesh* meshBarrel;

public:
    WScatterGun()
    {
        info.name = "Scatter Gun";
        info.description = "Wide area of effect. Hard to miss!";
        info.icon = g_res.getTexture("icon_scattergun");
        info.price = 900;
        info.maxUpgradeLevel = 5;
        info.weaponType = WeaponType::FRONT_WEAPON;
        info.tags[0] = "hood-1";
        info.tags[1] = "narrow";

        ammoUnitCount = 1;
        fireMode = FireMode::ONE_SHOT;

        loadModelData("weapon_scattergun");
        Model* model = g_res.getModel("weapon_scattergun");
        mesh = model->getMeshByName("Cube");
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

        Mat4 transform = vehicle->getTransform();
        f32 minSpeed = 25.f;
        Vec3 vel = convert(vehicle->getRigidBody()->getLinearVelocity())
            + vehicle->getForwardVector() * minSpeed;
        if (lengthSquared(vel) < square(minSpeed))
        {
            vel = normalize(vel) * minSpeed;
        }
        for (u32 i=0; i<12; ++i)
        {
            f32 s = 10.f;
            Vec3 v = vel + vehicle->getRightVector() * random(scene->randomSeries, -s, s);
            v += vel + vehicle->getUpVector() * random(scene->randomSeries, -4.f, 4.f);
            v += vehicle->getForwardVector() * random(scene->randomSeries, 0, 10.f);
            Vec3 pos = Vec3(transform * mountTransform * Vec4(projectileSpawnPoints[0], 1.f));
            scene->addEntity(new Projectile(pos,
                    v, transform.zAxis(), vehicle->vehicleIndex, Projectile::BULLET_SMALL));
        }
        g_audio.playSound3D(g_res.getSound("scattergun"),
                SoundType::GAME_SFX, vehicle->getPosition(), false,
                random(scene->randomSeries, 0.95f, 1.05f), 1.f,
                random(scene->randomSeries, -0.05f, 0.05f));

        ammo -= 1;
    }

    void render(class RenderWorld* rw, Mat4 const& vehicleTransform,
            VehicleConfiguration const& config, VehicleData const& vehicleData) override
    {
        Material* mat = g_res.getMaterial("plastic");
        mat->draw(rw, vehicleTransform * mountTransform, mesh, 2);
    }
};
