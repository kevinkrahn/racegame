#include "flash.h"
#include "../renderer.h"
#include "../scene.h"
#include "../billboard.h"

void Flash::onCreate(Scene* scene)
{
    angle = random(scene->randomSeries, 0.f, 2 * PI);
}

void Flash::onUpdate(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    Vec3 newPosition = position + velocity * deltaTime;
    f32 dist = distance(position, newPosition);
    f32 collisionRadius = 1.f;
    if (dist > 0.f && scene->raycastStatic(position, normalize(velocity), dist + collisionRadius))
    {
        velocity = { 0, 0, 0 };
    }
    else
    {
        position = newPosition;
    }
    life += deltaTime * 2.f;
    if (life >= 1.f)
    {
        this->destroy();
    }
}

void Flash::onRender(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    f32 t = 1.f - life;
    f32 tt = t * t;
    Texture* tex = g_res.getTexture("flash");
    Vec3 color = Vec3(1, 0.5f, 0.f);
    drawBillboard(rw, tex, position,
                Vec4(color, tt * 0.4f), scale * 1.1f * life * 2.f, angle);
    drawBillboard(rw, tex, position,
                Vec4(color * 5.5f, tt), scale * life, angle);
    rw->addPointLight(position, color * 6.f * t, 10.f, 2.f);
}
