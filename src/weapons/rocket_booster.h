#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"

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
        info.icon = g_res.getTexture("icon_rocketbooster");
        info.price = 1000;
        info.maxUpgradeLevel = 4;
        info.weaponType = WeaponType::REAR_WEAPON;

        mesh = g_res.getModel("exhaust_cone")->getMeshByName("world.Cone");
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
            boostTimer = max(boostTimer - deltaTime, 0.f);
            g_audio.setSoundPosition(boostSound, vehicle->getPosition());
            vehicle->setMotionBlur(min(boostTimer, 1.f), 0.1f);
            return;
        }

        if (fireBegin)
        {
            if (ammo == 0)
            {
                outOfAmmo(vehicle);
                return;
            }

            boostSound = g_audio.playSound3D(g_res.getSound("rocketboost"),
                    SoundType::GAME_SFX, vehicle->getPosition(), false, 1.f, 0.8f);

            boostTimer = 1.4f;
            ammo -= 1;
        }
    }

    void render(class RenderWorld* rw, Mat4 const& vehicleTransform,
            VehicleConfiguration const& config, VehicleData const& vehicleData) override
    {
        struct Flames
        {
            Mat4 exhausts[4];
            u32 exhaustCount;
            GLuint vao;
            u32 indexCount;
            float alpha;
        };

        Flames* renderData = g_game.tempMem.bump<Flames>();
        renderData->alpha = min(boostTimer * 7.f, 1.f);
        renderData->exhaustCount = 0;
        renderData->vao = mesh->vao;
        renderData->indexCount = mesh->numIndices;
        for (auto& p : vehicleData.exhaustHoles)
        {
            renderData->exhausts[renderData->exhaustCount] = vehicleTransform * glm::translate(Mat4(1.f), p);
            renderData->exhaustCount++;
            if (renderData->alpha > 0.f)
            {
                rw->addPointLight(vehicleTransform * Vec4(p + Vec3(-0.25f, 0, 0.25f), 1.f),
                        Vec3(1.f, 0.6f, 0.05f) * renderData->alpha, 3.f, 2.f);
            }
        }

        auto render = [](void* renderData) {
            Flames* flames = (Flames*)renderData;
            glBindTextureUnit(0, g_res.getTexture("flames")->handle);
            glBindVertexArray(flames->vao);
            for (u32 i=0; i<flames->exhaustCount; ++i)
            {
                auto& m = flames->exhausts[i];
                glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m));
                glUniform1f(1, (float)i * 0.3f);
                glUniform1f(2, flames->alpha);
                glDrawElements(GL_TRIANGLES, flames->indexCount, GL_UNSIGNED_INT, 0);
            }
        };

        static ShaderHandle shader = getShaderHandle("flames");
        rw->transparentPass({ shader, 1000, renderData, render });
    }

    bool shouldUse(Scene* scene, Vehicle* vehicle) override
    {
        return !vehicle->isInAir && ammo > 0 && vehicle->getForwardSpeed() > 5.f
                && irandom(scene->randomSeries, 0, 100 - (i32)vehicle->isFollowed * 50) < 2;
    }
};
