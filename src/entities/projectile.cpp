#include "projectile.h"
#include "../scene.h"
#include "../renderer.h"
#include "../vehicle.h"
#include "../billboard.h"

void Projectile::onCreate(Scene* scene)
{
    bulletMesh = g_res.getModel("misc")->getMeshByName("world.Bullet");
    switch(projectileType)
    {
        case BLASTER:
            life = 3.f;
            groundFollow = false;
            collisionRadius = 0.4f;
            damage = 50;
            this->velocity -= upVector * 0.7f;
            impactEmitter = ParticleEmitter(&scene->sparks, 5, 5,
                    Vec4(Vec3(0.04f, 1.f, 0.04f) * 2.f, 1.f), 0.5f, 6.f, 10.f);
            environmentImpactSounds = { "blaster_hit" };
            break;
        case PHANTOM:
            life = 2.5f;
            groundFollow = false;
            passThroughVehicles = true;
            collisionRadius = 0.4f;
            damage = 50;
            this->velocity -= upVector * 0.7f;
            impactEmitter = ParticleEmitter(&scene->sparks, 5, 5,
                    Vec4(Vec3(1.f, 0.02f, 0.95f) * 2.f, 1.f), 0.5f, 6.f, 10.f);
            environmentImpactSounds = { "blaster_hit" };
            break;
        case BULLET:
        case BULLET_SMALL:
            life = 2.f;
            groundFollow = false;
            collisionRadius = 0.1f;
            damage = 12;
            this->velocity -= upVector;
            impactEmitter = ParticleEmitter(&scene->sparks, 1, 1,
                    Vec4(Vec3(1.f, 0.6f, 0.02f) * 2.f, 1.f), 0.5f, 6.f, 10.f);
            environmentImpactSounds = {
                "richochet1",
                "richochet2",
                "richochet3",
                "richochet4",
            };
            vehicleImpactSounds = {
                "bullet_impact1",
                "bullet_impact2",
                "bullet_impact3",
            };
            break;
        case MISSILE:
            life = 4.f;
            groundFollow = true;
            collisionRadius = 0.5f;
            damage = 120;
            accel = 14.f;
            maxSpeed = 110.f;
            explosionStrength = 5.f;
            environmentImpactSounds = { "explosion1" };
            vehicleImpactSounds = { "explosion1" };
            break;
        case HOMING_MISSILE:
            life = 4.25f;
            groundFollow = true;
            collisionRadius = 0.5f;
            damage = 150;
            accel = 20.f;
            homingSpeed = 85.f;
            maxSpeed = 100.f;
            explosionStrength = 6.f;
            environmentImpactSounds = { "explosion1" };
            vehicleImpactSounds = { "explosion1" };
            break;
        case BOUNCER:
            life = 4.f;
            groundFollow = true;
            collisionRadius = 0.6f;
            damage = 75;
            bounceOffEnvironment = true;
            impactEmitter = ParticleEmitter(&scene->sparks, 5, 5,
                    Vec4(Vec3(0.3f, 0.3f, 1.f) * 2.f, 1.f), 0.5f, 6.f, 10.f);
            environmentImpactSounds = { "bouncer_bounce" };
            break;
    }

    Vehicle* vehicle = scene->getVehicle(instigator);
    ignoreActors.push(vehicle->getRigidBody());
}

void Projectile::onUpdate(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    Vec3 prevPosition = position;

    if (groundFollow)
    {
        f32 speed = length(velocity);
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
                f32 velAgainstHit = dot(-velocity, convert(rayHit.block.normal));
                if (velAgainstHit > 0.f)
                {
                    velocity += convert(rayHit.block.normal) *
                        min(velAgainstHit * deltaTime * 20.f, velAgainstHit);
                }
                velocity = normalize(velocity) * min(speed + accel * deltaTime, maxSpeed);
            }
        }
    }

    if (homingSpeed > 0.f)
    {
        f32 lowestTargetPriority = FLT_MAX;
        Vec3 targetPosition;
        for (auto& v : scene->getVehicles())
        {
            if (v->getRigidBody() == ignoreActors.front())
            {
                continue;
            }

            Vec3 dir = normalize(velocity);
            Vec3 diff = v->getPosition() - position;
            f32 vDot = dot(dir, normalize(diff));
            f32 targetPriority = lengthSquared(diff);
            if (vDot > 0.25f && targetPriority < lowestTargetPriority)
            {
                targetPosition = v->getPosition();
                lowestTargetPriority = targetPriority;
            }
        }

        if (lowestTargetPriority != FLT_MAX)
        {
            f32 speed = length(velocity);
            velocity += normalize(targetPosition - position) * (homingSpeed * deltaTime);
            velocity = normalize(velocity) * speed;
        }
    }

    position += velocity * deltaTime;

    if (projectileType == MISSILE || projectileType == HOMING_MISSILE)
    {
        // TODO: play sound while missile is traveling
        if (((u32)(life * 100.f) & 1) == 0)
        {
            scene->smoke.spawn(position, Vec3(0,0,1), 0.8f,
                    Vec4(Vec3(0.6f), 1.f), 1.75f);
        }
    }

    Vec3 sweepDir = normalize(position - prevPosition);
    PxSweepHit hitBuffer[8];
    PxSweepBuffer hit(hitBuffer, ARRAY_SIZE(hitBuffer));
    PxQueryFilterData filter;
    filter.flags |= PxQueryFlag::ePREFILTER;
    filter.data = PxFilterData(COLLISION_FLAG_CHASSIS |
            COLLISION_FLAG_TRACK | COLLISION_FLAG_TERRAIN | COLLISION_FLAG_OBJECT, 0, 0, 0);
    PxTransform initialPose(convert(prevPosition), PxQuat(PxIdentity));
    scene->getPhysicsScene()->sweep(PxSphereGeometry(collisionRadius), initialPose, convert(sweepDir),
            length(position - prevPosition), hit, PxHitFlags(PxHitFlag::eDEFAULT), filter, this);
    for (u32 i=0; i<hit.nbTouches; ++i)
    {
        onHit(scene, &hit.touches[i]);
        if (ignoreActors.size() < ignoreActors.maximumSize())
        {
            ignoreActors.push(hit.touches[i].actor);
        }
    }
    if (hit.hasBlock)
    {
        onHit(scene, &hit.block);
    }

    life -= deltaTime;
    if (life <= 0.f)
    {
        createImpactParticles(scene, nullptr);
        this->destroy();
    }
}

void Projectile::onRender(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    Mat4 m = Mat4::faceDirection(normalize(velocity), upVector);

    switch (projectileType)
    {
        case BLASTER:
        {
            Mat4 transform = Mat4::translation(position) * m * Mat4::scaling(Vec3(0.75f));
            drawSimple(rw, bulletMesh, &g_res.white, transform,
                Vec3(0.2f, 0.9f, 0.2f), Vec3(0.01f, 1.5f, 0.01f));
            drawBillboard(rw, g_res.getTexture("flare"), position+Vec3(0,0,0.2f),
                    {0.01f,1.f,0.01f,0.2f}, 1.5f, 0.f, false);
            rw->addPointLight(position, Vec3(0.2f, 0.9f, 0.2f) * 2.f, 4.f, 2.f);
        } break;
        case BULLET:
        {
            Vec3 color = Vec3(1.f, 0.5f, 0.01f);
            Vec3 emit = Vec3(1.f, 0.5f, 0.01f) * 2.f;
            Mat4 transform = Mat4::translation(position) * m * Mat4::scaling(Vec3(0.35f));
            drawSimple(rw, bulletMesh, &g_res.white, transform, color, emit);
            drawBillboard(rw, g_res.getTexture("flare"),
                        position, Vec4(emit, 0.8f), 0.75f, 0.f, false);
            rw->addPointLight(position, color * 2.f, 4.f, 2.f);
        } break;
        case BULLET_SMALL:
        {
            Vec3 color = Vec3(1.f, 0.5f, 0.01f);
            Vec3 emit = Vec3(1.f, 0.5f, 0.01f) * 2.f;
            Mat4 transform = Mat4::translation(position) * m * Mat4::scaling(Vec3(0.35f));
            drawSimple(rw, bulletMesh, &g_res.white, transform, color, emit);
            drawBillboard(rw, g_res.getTexture("flare"),
                        position, Vec4(emit, 0.8f), 0.5f, 0.f, false);
        } break;
        case MISSILE:
        {
            Mesh* mesh = g_res.getModel("weapon_missile")->getMeshByName("missile.Missile");
            Mat4 transform = Mat4::translation(position) * m;
            drawSimple(rw, mesh, &g_res.white, transform, Vec3(1.f));
            drawBillboard(rw, g_res.getTexture("flare"), position,
                        Vec4(1.f, 0.5f, 0.03f, 0.8f), 1.8f, 0.f, false);
            rw->addPointLight(position, Vec3(1.f, 0.5f, 0.03f) * 5.f, 5.f, 2.f);
        } break;
        case HOMING_MISSILE:
        {
            Mesh* mesh = g_res.getModel("weapon_missile")->getMeshByName("missile.Missile");
            Mat4 transform = Mat4::translation(position) * m;
            drawSimple(rw, mesh, &g_res.white, transform, Vec3(1.f));
            drawBillboard(rw, g_res.getTexture("flare"), position,
                        Vec4(1.f, 0.2f, 0.03f, 0.8f), 1.8f, 0.f, false);
            rw->addPointLight(position, Vec3(1.f, 0.2f, 0.03f) * 5.f, 5.f, 2.f);
        } break;
        case BOUNCER:
        {
            Mesh* mesh = g_res.getModel("misc")->getMeshByName("world.Sphere");
            Mat4 transform = Mat4::translation(position) * Mat4::scaling(Vec3(0.4f));
            drawSimple(rw, mesh, &g_res.white, transform, Vec3(1.f), Vec3(0.5f));
            drawBillboard(rw, g_res.getTexture("flare"), position,
                        Vec4(0.1f, 0.12f, 1.f, 0.8f), 1.75f, 0.f, false);
            rw->addPointLight(position, Vec3(0.1f, 0.15f, 1.f) * 7.f, 6.5f, 2.f);
        } break;
        case PHANTOM:
        {
            Vec3 color = Vec3(1.f, 0.01f, 0.95f);
            Vec3 emit = Vec3(1.f, 0.01f, 0.95f) * 1.5f;
            Mat4 transform = Mat4::translation(position) * m * Mat4::scaling(Vec3(0.75f));
            drawSimple(rw, bulletMesh, &g_res.white, transform, color, emit);
            drawBillboard(rw, g_res.getTexture("flare"),
                    position+Vec3(0,0,0.2f), Vec4(color, 0.4f), 1.5f, 0.f, false);
            rw->addPointLight(position, color * 2.f, 4.f, 2.f);
        } break;
    }
}

void Projectile::onHit(Scene* scene, PxSweepHit* hit)
{
    ActorUserData* data = (ActorUserData*)hit->actor->userData;
    Vec3 hitPos = convert(hit->position);

    createImpactParticles(scene, hit);

    if (explosionStrength > 0.f)
    {
        scene->createExplosion(hitPos, velocity * 0.5f, explosionStrength);
    }

    // hit vehicle
    if (data && data->entityType == ActorUserData::VEHICLE)
    {
        data->vehicle->applyDamage((f32)damage, instigator);

        if (!vehicleImpactSounds.empty())
        {
            u32 index = irandom(scene->randomSeries, 0, vehicleImpactSounds.size());
            g_audio.playSound3D(g_res.getSound(vehicleImpactSounds[index]),
                    SoundType::GAME_SFX, hitPos, false, 0.9f,
                    random(scene->randomSeries, 0.75f, 0.9f));
        }

        if (!passThroughVehicles)
        {
            this->destroy();
        }
    }
    // hit something else
    else
    {
        if (!environmentImpactSounds.empty())
        {
            u32 index = irandom(scene->randomSeries, 0, environmentImpactSounds.size());
            g_audio.playSound3D(g_res.getSound(environmentImpactSounds[index]),
                    SoundType::GAME_SFX, hitPos, false, 0.9f,
                    random(scene->randomSeries, 0.75f, 0.9f));
        }

        if (bounceOffEnvironment)
        {
            Vec3 n = convert(hit->normal);
            velocity = -2.f * dot(velocity, n) * n + velocity;
            position = convert(hit->position) + n * (collisionRadius + 0.001f);
        }
        else
        {
            this->destroy();
        }
    }
}

void Projectile::createImpactParticles(Scene* scene, PxSweepHit* hit)
{
    Vec3 normal = hit ? convert(hit->normal) : Vec3(0, 0, 1);
    Vec3 pos = hit ? convert(hit->position) : position;
    impactEmitter.emit(pos, normal);
}
