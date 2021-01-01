#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"

class WMissiles : public Weapon
{
    Mesh* mesh;
    Mesh* mountMesh;

public:
    WMissiles()
    {
        info.name = "Missiles";
        info.description = "Deals high damage and follows slopes!";
        info.icon = g_res.getTexture("icon_missile");
        info.price = 1000;
        info.maxUpgradeLevel = 5;
        info.weaponType = WeaponType::FRONT_WEAPON;
        info.tags[0] = "hood-1";
        info.tags[1] = "roof-1";

        Model* model = g_res.getModel("weapon_missile");
        mesh = model->getMeshByName("missile.Missile");
        mountMesh = model->getMeshByName("missile.Mount");
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
        Vec3 pos = Vec3(transform * mountTransform * Vec4(missileSpawnPoint(ammo - 1), 1.f));
        scene->addEntity(new Projectile(pos,
                vel, transform.zAxis(), vehicle->vehicleIndex, Projectile::MISSILE));
        g_audio.playSound3D(g_res.getSound("missile"),
                SoundType::GAME_SFX, vehicle->getPosition(), false,
                random(scene->randomSeries, 0.95f, 1.05f), 0.9f);

        ammo -= 1;
    }

    Vec3 missileSpawnPoint(u32 i)
    {
        f32 separation = 0.35f;
        f32 xPos[] = { -0.1f, -0.f, 0.1f, -0.f, -0.1f };
        return Vec3(xPos[i] * 1.35f, -(separation * (info.maxUpgradeLevel - 1)) * 0.5f + separation * i, 0.15f);
    }

    void render(class RenderWorld* rw, Mat4 const& vehicleTransform,
            VehicleConfiguration const& config, VehicleData const& vehicleData) override
    {
        Material* mat = g_res.getMaterial("plastic");
        for (u32 i=0; i<info.maxUpgradeLevel; ++i)
        {
            Mat4 t = vehicleTransform * mountTransform
                * Mat4::translation(missileSpawnPoint(i));
            mat->draw(rw, t, mountMesh, 2);
        }
        for (u32 i=0; i<ammo; ++i)
        {
            Mat4 t = vehicleTransform * mountTransform
                * Mat4::translation(missileSpawnPoint(i));
            mat->draw(rw, t, mesh, 2);
        }
    }
};
