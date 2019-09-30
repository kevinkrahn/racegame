#include "flash.h"
#include "../renderer.h"
#include "../billboard.h"
#include "../scene.h"

void Flash::onCreate(Scene* scene)
{
    angle = random(scene->randomSeries, 0.f, 2 * M_PI);
}

void Flash::onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime)
{
    position += velocity * deltaTime;
    life += deltaTime * 2.f;
    if (life >= 1.f)
    {
        this->destroy();
    }
}

void Flash::onRender(Renderer* renderer, Scene* scene, f32 deltaTime)
{
    f32 t = 1.f - life;
    f32 tt = t * t;
    Texture* tex = g_resources.getTexture("flash");
    renderer->push(BillboardRenderable(tex, position,
                glm::vec4(glm::vec3(1, 0.5f, 0.f), tt * 0.4f), scale * 1.1f * life * 2.f, angle));
    renderer->push(BillboardRenderable(tex, position,
                glm::vec4(glm::vec3(1, 0.5f, 0.f) * 5.5f, tt), scale * life, angle));
}
