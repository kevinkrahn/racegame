#include "particle_system.h"
#include "game.h"

void ParticleSystem::update(f32 deltaTime)
{
    for (auto p = particles.begin(); p != particles.end();)
    {
        f32 t = p->life / p->totalLife;
        if (t >= 1.f)
        {
            particles.erase(p);
            continue;
        }

        p->scale += deltaTime * 0.5f;
        p->position += p->velocity * deltaTime;
        p->life += deltaTime;

        ++p;
    }
}

void ParticleSystem::draw()
{
    for (auto p = particles.begin(); p != particles.end(); ++p)
    {
        f32 t = p->life / p->totalLife;
        for (u32 i = 0; i < alphaCurve.size(); ++i)
        {
            auto p2 = alphaCurve[i];
            if (t < p2.t)
            {
                auto p1 = alphaCurve[i - 1];
                f32 v = p1.v + (p2.v - p1.v) * ((t - p1.t) / (p2.t - p1.t));
                game.renderer.drawBillboard(0, p->position, glm::vec3(p->life * p->scale * 0.5f + 0.5f),
                            p->color * glm::vec4(1, 1, 1, v * p->alphaMultiplier), p->angle);
                break;
            }
        }
    }
}
