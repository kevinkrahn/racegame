#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"
#include "../mesh_renderables.h"
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
            glow = glm::min(glow + deltaTime * 3.f, 1.f);
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

        glm::mat4 transform = vehicle->getTransform();
        f32 minSpeed = 34.f;
        glm::vec3 vel = convert(vehicle->getRigidBody()->getLinearVelocity())
            + vehicle->getForwardVector() * minSpeed;
        if (glm::length2(vel) < square(minSpeed))
        {
            vel = glm::normalize(vel) * minSpeed;
        }
        glm::vec3 pos = transform * mountTransform * glm::vec4(projectileSpawnPoints[0], 1.f);
        scene->addEntity(new Projectile(pos,
                vel, zAxisOf(transform), vehicle->vehicleIndex, Projectile::BOUNCER));
        g_audio.playSound3D(g_res.getSound("bouncer_fire"),
                SoundType::GAME_SFX, vehicle->getPosition(), false,
                random(scene->randomSeries, 0.95f, 1.05f), 0.9f);

        ammo -= 1;
        glow = 0.f;
    }

    void render(class RenderWorld* rw, glm::mat4 const& vehicleTransform,
            VehicleConfiguration const& config, VehicleData const& vehicleData) override
    {
        rw->push(LitRenderable(mesh, vehicleTransform * mountTransform));
        glm::vec3 pos = vehicleTransform * mountTransform *
            glm::vec4(projectileSpawnPoints[0] + glm::vec3(0.01f, 0.f, 0.05f), 1.f);
        rw->push(BillboardRenderable(g_res.getTexture("bouncer_projectile"),
                    pos, glm::vec4(1.f), 0.7f * glow, 0.f, false));
    }
};
