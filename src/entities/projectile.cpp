#include "projectile.h"
#include "../scene.h"
#include "../renderer.h"
#include "../vehicle.h"
#include "../mesh_renderables.h"
#include "../billboard.h"

void Projectile::onUpdate(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    glm::vec3 prevPosition = position;
    position += velocity * deltaTime;

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

    Vehicle* vehicle = scene->getVehicle(instigator);
    PxRigidBody* ignoreBody = vehicle ? vehicle->getRigidBody() : nullptr;
    PxSweepBuffer sweepHit;
    if (scene->sweep(0.3f, prevPosition,
                glm::normalize(position - prevPosition),
                glm::length(position - prevPosition), &sweepHit, ignoreBody))
    {
        ActorUserData* data = (ActorUserData*)sweepHit.block.actor->userData;
        if (data && data->entityType == ActorUserData::VEHICLE)
        {
            data->vehicle->applyDamage(50.f, instigator);
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
    settings.worldTransform = glm::translate(glm::mat4(1.f), position)
        * m * glm::scale(glm::mat4(1.f), glm::vec3(0.75f));
    settings.color = glm::vec3(0.2f, 0.9f, 0.2f);
    settings.emit = glm::vec3(0.01f, 1.5f, 0.01f);
    rw->push(LitRenderable(settings));

    rw->push(BillboardRenderable(g_resources.getTexture("flare"),
                position+glm::vec3(0,0,0.2f), {0.01f,1.f,0.01f,0.2f}, 1.5f));
}
