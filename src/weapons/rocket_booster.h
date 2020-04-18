#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"
#include "../mesh_renderables.h"

class Flames : public Renderable
{
    SmallVec<glm::mat4, 4> exhausts;
    Mesh* mesh;
    float alpha;

public:
    Flames(SmallVec<glm::mat4, 4> exhausts, Mesh* mesh, float alpha)
        : exhausts(exhausts), mesh(mesh), alpha(alpha) {}

    void onLitPassPriorityTransition(Renderer* renderer) override
    {
        glDisable(GL_POLYGON_OFFSET_FILL);
        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
        glUseProgram(renderer->getShaderProgram("flames"));
        glEnable(GL_CULL_FACE);
        glBindTextureUnit(0, g_res.textures->flames.handle);
        glBindVertexArray(mesh->vao);
    }

    void onLitPass(Renderer* renderer) override
    {
        Mesh* mesh = g_res.getMesh("world.Cone");
        for (u32 i=0; i<exhausts.size(); ++i)
        {
            auto& m = exhausts[i];
            glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m));
            glUniform1f(1, (float)i * 0.3f);
            glUniform1f(2, alpha);
            glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);
        }
    }

    i32 getPriority() const override { return 15000; }
    std::string getDebugString() const override{ return "Flames"; }
};

class WRocketBooster : public Weapon
{
    f32 boostTimer = 0.f;
    SoundHandle boostSound;
    Mesh* mesh;

public:
    WRocketBooster()
    {
        info.name = "Rocket Booster";
        info.description = "It's like nitrous, but better!";
        info.icon = &g_res.textures->icon_rocketbooster;
        info.price = 1000;
        info.maxUpgradeLevel = 5;
        info.weaponType = WeaponInfo::REAR_WEAPON;

        mesh = g_res.getMesh("world.Cone");
    }

    void reset() override
    {
        boostTimer = 0.f;
    }

    void update(Scene* scene, Vehicle* vehicle, bool fireBegin, bool fireHold,
            f32 deltaTime) override
    {
        if (boostTimer > 0.f)
        {
            vehicle->getRigidBody()->addForce(
                    convert(vehicle->getForwardVector() * 9.f),
                    PxForceMode::eACCELERATION);
            boostTimer = glm::max(boostTimer - deltaTime, 0.f);
            g_audio.setSoundPosition(boostSound, vehicle->getPosition());
            return;
        }

        if (fireBegin)
        {
            if (ammo == 0)
            {
                outOfAmmo(vehicle);
                return;
            }

            boostSound = g_audio.playSound3D(&g_res.sounds->rocketboost,
                    SoundType::GAME_SFX, vehicle->getPosition(), false, 1.f, 0.8f);

            boostTimer = 1.4f;
            ammo -= 1;
        }
    }

    void render(class RenderWorld* rw, glm::mat4 const& vehicleTransform,
            VehicleConfiguration const& config, VehicleData const& vehicleData) override
    {
        SmallVec<glm::mat4, 4> exhausts;
        for (auto& p : vehicleData.exhaustHoles)
        {
            exhausts.push_back(vehicleTransform * glm::translate(glm::mat4(1.f), p));
        }
        rw->push(Flames(exhausts, mesh, glm::min(boostTimer * 7.f, 1.f)));
    }

    bool shouldUse(Scene* scene, Vehicle* vehicle) override
    {
        return !vehicle->isInAir && ammo > 0 && vehicle->getForwardSpeed() > 5.f
                && irandom(scene->randomSeries, 0, 100 - (i32)vehicle->isFollowed * 50) < 2;
    }
};
