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
            damage = 50;
            break;
        case BULLET:
            life = 2.f;
            groundFollow = false;
            collisionRadius = 0.1f;
            damage = 12;
            break;
        case MISSILE:
            life = 4.f;
            groundFollow = true;
            collisionRadius = 0.5f;
            damage = 120;
            accel = 4.f;
            break;
        case BOUNCER:
            life = 10.f;
            groundFollow = true;
            collisionRadius = 0.6f;
            damage = 90;
            break;
    }
}


void Projectile::onUpdate(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    glm::vec3 prevPosition = position;

    if (groundFollow)
    {
        f32 speed = glm::length(velocity);
        PxRaycastBuffer rayHit;
        f32 dist = 1.4f;
        velocity.z -= deltaTime * 15.f;
        if (scene->raycastStatic(position, { 0, 0, -1 }, 4.f, &rayHit))
        {
            velocity.z -= deltaTime * 20.f;
            if (rayHit.block.distance <= dist)
            {
                f32 compression = (dist - rayHit.block.distance) / dist;
                velocity.z += compression * 400.f * deltaTime;
                velocity.z = smoothMove(velocity.z, 0.f, 8.f, deltaTime);
                f32 velAgainstHit = glm::dot(-velocity, convert(rayHit.block.normal));
                if (velAgainstHit > 0.f)
                {
                    velocity += convert(rayHit.block.normal) *
                        glm::min(velAgainstHit * deltaTime * 20.f, velAgainstHit);
                }
                velocity = glm::normalize(velocity) * (speed + accel * deltaTime);
            }
        }
    }

    position += velocity * deltaTime;

    if (projectileType == MISSILE)
    {
        // TODO: play sound while missile is traveling
        if (((u32)(life * 100.f) & 1) == 0)
        {
            scene->smoke.spawn(position, glm::vec3(0,0,1), 0.8f,
                    glm::vec4(glm::vec3(0.6f), 1.f), 1.75f);
        }
    }

    Vehicle* vehicle = scene->getVehicle(instigator);
    PxRigidBody* ignoreBody = vehicle ? vehicle->getRigidBody() : nullptr;
    PxSweepBuffer sweepHit;
    glm::vec3 sweepDir = glm::normalize(position - prevPosition);
    if (scene->sweep(collisionRadius, prevPosition,
                sweepDir, glm::length(position - prevPosition), &sweepHit, ignoreBody))
    {
        ActorUserData* data = (ActorUserData*)sweepHit.block.actor->userData;
        glm::vec3 hitPos = convert(sweepHit.block.position);

        // hit vehicle
        if (data && data->entityType == ActorUserData::VEHICLE)
        {
            data->vehicle->applyDamage((f32)damage, instigator);
            switch (projectileType)
            {
                case BLASTER:
                {
                    // TODO: sound
                    this->destroy();
                } break;
                case BULLET:
                {
                    const char* impacts[] = {
                        "bullet_impact1",
                        "bullet_impact2",
                        "bullet_impact3",
                    };
                    u32 index = irandom(scene->randomSeries, 0, ARRAY_SIZE(impacts));
                    g_audio.playSound3D(g_resources.getSound(impacts[index]),
                            SoundType::GAME_SFX, hitPos, false,
                            random(scene->randomSeries, 0.8f, 1.2f),
                            random(scene->randomSeries, 0.8f, 1.f));
                    this->destroy();
                } break;
                case MISSILE:
                {
                    scene->createExplosion(hitPos, glm::vec3(0.f), 5.f);
                    g_audio.playSound3D(g_resources.getSound("explosion1"),
                            SoundType::GAME_SFX, hitPos, false, 1.f, 0.7f);
                    this->destroy();
                } break;
                case BOUNCER:
                {
                    // TODO: sound
                    this->destroy();
                } break;
            }
        }
        // hit something else
        else
        {
            switch (projectileType)
            {
                case BLASTER:
                {
                    g_audio.playSound3D(g_resources.getSound("blaster_hit"),
                            SoundType::GAME_SFX, hitPos, false, 1.f, 0.8f);
                    this->destroy();
                } break;
                case BULLET:
                {
                    const char* impacts[] = {
                        "richochet1",
                        "richochet2",
                        "richochet3",
                        "richochet4",
                    };
                    u32 index = irandom(scene->randomSeries, 0, ARRAY_SIZE(impacts));
                    g_audio.playSound3D(g_resources.getSound(impacts[index]),
                            SoundType::GAME_SFX, hitPos, false, 0.9f,
                            random(scene->randomSeries, 0.75f, 0.9f));
                    this->destroy();
                } break;
                case MISSILE:
                {
                    scene->createExplosion(hitPos, glm::vec3(0.f), 5.f);
                    g_audio.playSound3D(g_resources.getSound("explosion1"),
                            SoundType::GAME_SFX, hitPos, false, 1.f, 0.7f);
                    this->destroy();
                } break;
                case BOUNCER:
                {
                    glm::vec3 n = convert(sweepHit.block.normal);
                    velocity = -2.f * glm::dot(velocity, n) * n + velocity;
                    position = (prevPosition + sweepHit.block.distance * sweepDir) + n * 0.1f;
                    g_audio.playSound3D(g_resources.getSound("bouncer_bounce"),
                            SoundType::GAME_SFX, hitPos, false, 1.f, 0.4f);
                } break;
            }
        }
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
        case MISSILE:
            settings.color = glm::vec3(0.1f);
            settings.worldTransform = glm::translate(glm::mat4(1.f), position)
                * m * glm::scale(glm::mat4(1.f), glm::vec3(0.8f));
            rw->push(LitRenderable(settings));
            rw->push(BillboardRenderable(g_resources.getTexture("flare"), position,
                        glm::vec4(1.f, 0.5f, 0.03f, 0.8f), 1.8f));
            break;
        case BOUNCER:
            settings.mesh = g_resources.getMesh("world.Sphere");
            settings.color = glm::vec3(1.f);
            settings.emit = glm::vec3(0.5f);
            settings.worldTransform = glm::translate(glm::mat4(1.f), position)
                 * glm::scale(glm::mat4(1.f), glm::vec3(0.4f));
            rw->push(LitRenderable(settings));
            // TODO: use unlit billboard
            rw->push(BillboardRenderable(g_resources.getTexture("bouncer_projectile"),
                        position, glm::vec4(1.f), 1.75f));
            break;
    }
}
