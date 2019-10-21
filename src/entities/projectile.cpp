#include "projectile.h"
#include "../scene.h"
#include "../renderer.h"
#include "../vehicle.h"
#include "../mesh_renderables.h"
#include "../billboard.h"

Projectile::Projectile(glm::vec3 const& position, glm::vec3 const& velocity,
        glm::vec3 const& upVector, u32 instigator, ProjectileType projectileType)
    : position(position), velocity(velocity), upVector(upVector),
        instigator(instigator), projectileType(projectileType)
{
    bulletMesh = g_resources.getMesh("world.Bullet");
    switch(projectileType)
    {
        case BLASTER:
            life = 3.f;
            groundFollow = false;
            collisionRadius = 0.3f;
            damage = 60.f;
            break;
        case BULLET:
            life = 2.f;
            groundFollow = false;
            collisionRadius = 0.1f;
            damage = 12.f;
            break;
    }
}


void Projectile::onUpdate(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    glm::vec3 prevPosition = position;
    position += velocity * deltaTime;

    if (groundFollow)
    {
        f32 speed = glm::length(velocity);
        PxRaycastBuffer rayHit;
        f32 dist = 3.f;
        if (scene->raycastStatic(position, { 0, 0, -1 }, dist, &rayHit))
        {
            velocity.z = smoothMove(velocity.z, 0.f, 5.f, deltaTime);
            f32 compression = 1.f - rayHit.block.distance;
            if (rayHit.block.distance < 0.8f && velocity.z < 0.f) velocity.z = 0.f;
            if (rayHit.block.distance > 1.2f && velocity.z > 0.f) velocity.z = 0.f;
            velocity.z += compression * 90.f * deltaTime;
            velocity = glm::normalize(velocity) * speed;
        }
    }

    Vehicle* vehicle = scene->getVehicle(instigator);
    PxRigidBody* ignoreBody = vehicle ? vehicle->getRigidBody() : nullptr;
    PxSweepBuffer sweepHit;
    if (scene->sweep(collisionRadius, prevPosition,
                glm::normalize(position - prevPosition),
                glm::length(position - prevPosition), &sweepHit, ignoreBody))
    {
        ActorUserData* data = (ActorUserData*)sweepHit.block.actor->userData;
        if (data && data->entityType == ActorUserData::VEHICLE)
        {
            data->vehicle->applyDamage(damage, instigator);
            switch (projectileType)
            {
                case BLASTER:
                    break;
                case BULLET:
                    const char* impacts[] = {
                        "bullet_impact1",
                        "bullet_impact2",
                        "bullet_impact3",
                    };
                    u32 index = irandom(scene->randomSeries, 0, ARRAY_SIZE(impacts));
                    g_audio.playSound3D(g_resources.getSound(impacts[index]),
                            SoundType::GAME_SFX, position, false,
                            random(scene->randomSeries, 0.8f, 1.2f),
                            random(scene->randomSeries, 0.8f, 1.f));
                    break;
            }
        }
        else
        {
            switch (projectileType)
            {
                case BLASTER:
                    g_audio.playSound3D(g_resources.getSound("blaster_hit"),
                            SoundType::GAME_SFX, position, false, 1.f, 0.8f);
                    break;
                case BULLET:
                    const char* impacts[] = {
                        "richochet1",
                        "richochet2",
                        "richochet3",
                        "richochet4",
                    };
                    u32 index = irandom(scene->randomSeries, 0, ARRAY_SIZE(impacts));
                    g_audio.playSound3D(g_resources.getSound(impacts[index]),
                            SoundType::GAME_SFX, position, false, 0.9f,
                            random(scene->randomSeries, 0.75f, 0.9f));
                    break;
            }
        }
        this->destroy();
    }

    life -= deltaTime;
    if (life <= 0.f)
    {
        this->destroy();
    }
}

void Projectile::onRender(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    glm::mat4 m(1.f);
    m[0] = glm::vec4(glm::normalize(velocity), m[0].w);
    m[1] = glm::vec4(glm::normalize(
                glm::cross(upVector, glm::vec3(m[0]))), m[1].w);
    m[2] = glm::vec4(glm::normalize(
                glm::cross(glm::vec3(m[0]), glm::vec3(m[1]))), m[2].w);

    LitSettings settings;
    settings.mesh = bulletMesh;
    settings.fresnelScale = 0.3f;
    settings.fresnelPower = 2.0f;
    settings.fresnelBias = 0.1f;

    switch (projectileType)
    {
        case BLASTER:
            settings.color = glm::vec3(0.2f, 0.9f, 0.2f);
            settings.emit = glm::vec3(0.01f, 1.5f, 0.01f);
            settings.worldTransform = glm::translate(glm::mat4(1.f), position)
                * m * glm::scale(glm::mat4(1.f), glm::vec3(0.75f));
            rw->push(LitRenderable(settings));
            rw->push(BillboardRenderable(g_resources.getTexture("flare"),
                        position+glm::vec3(0,0,0.2f), {0.01f,1.f,0.01f,0.2f}, 1.5f));
            break;
        case BULLET:
            settings.color = glm::vec3(1.f, 0.5f, 0.01f);
            settings.emit = glm::vec3(1.f, 0.5f, 0.01f) * 2.f;
            settings.worldTransform = glm::translate(glm::mat4(1.f), position)
                * m * glm::scale(glm::mat4(1.f), glm::vec3(0.35f));
            rw->push(LitRenderable(settings));
            rw->push(BillboardRenderable(g_resources.getTexture("flare"),
                        position, glm::vec4(settings.emit, 0.8f), 0.75f));
            break;
    }
}
