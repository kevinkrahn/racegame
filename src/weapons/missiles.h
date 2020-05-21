#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"
#include "../mesh_renderables.h"

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
        info.weaponType = WeaponInfo::FRONT_WEAPON;

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

        glm::mat4 transform = vehicle->getTransform();
        f32 minSpeed = 10.f;
        glm::vec3 vel = convert(vehicle->getRigidBody()->getLinearVelocity())
            + vehicle->getForwardVector() * minSpeed;
        if (glm::length2(vel) < square(minSpeed))
        {
            vel = glm::normalize(vel) * minSpeed;
        }
        glm::vec3 pos = transform * mountTransform * glm::vec4(missileSpawnPoint(ammo - 1), 1.f);
        scene->addEntity(new Projectile(pos,
                vel, zAxisOf(transform), vehicle->vehicleIndex, Projectile::MISSILE));
        g_audio.playSound3D(g_res.getSound("missile"),
                SoundType::GAME_SFX, vehicle->getPosition(), false,
                random(scene->randomSeries, 0.95f, 1.05f), 0.9f);

        //ammo -= 1;
    }

    glm::vec3 missileSpawnPoint(u32 i)
    {
        f32 separation = 0.35f;
        f32 xPos[] = { -0.1f, -0.f, 0.1f, -0.f, -0.1f };
        return glm::vec3(xPos[i] * 1.35f, -(separation * (info.maxUpgradeLevel - 1)) * 0.5f + separation * i, 0.15f);
    }

    void render(class RenderWorld* rw, glm::mat4 const& vehicleTransform,
            VehicleConfiguration const& config, VehicleData const& vehicleData) override
    {
        for (u32 i=0; i<info.maxUpgradeLevel; ++i)
        {
            glm::mat4 t = vehicleTransform * mountTransform
                * glm::translate(glm::mat4(1.f), missileSpawnPoint(i));
            rw->push(LitRenderable(mountMesh, t));
        }
        for (u32 i=0; i<ammo; ++i)
        {
            glm::mat4 t = vehicleTransform * mountTransform
                * glm::translate(glm::mat4(1.f), missileSpawnPoint(i));
            rw->push(LitRenderable(mesh, t));
        }
    }
};
