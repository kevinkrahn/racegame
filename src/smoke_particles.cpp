#include "smoke_particles.h"
#include "game.h"
#include "renderer.h"

void SmokeParticles::onBeforeRender(f32 deltaTime)
{
    for (auto p = particles.begin(); p != particles.end();)
    {
        f32 t = p->life / p->totalLife;
        if (t >= 1.f)
        {
            p = particles.erase(p);
            continue;
        }

        p->scale += deltaTime * 0.5f;
        p->position += p->velocity * deltaTime;
        p->life += deltaTime;

        ++p;
    }
}

void SmokeParticles::onLitPass(Renderer* renderer)
{
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glUseProgram(renderer->getShaderProgram("billboard"));
    glBindVertexArray(emptyVAO);
    Texture* tex = g_resources.getTexture("smoke");
    glBindTextureUnit(0, tex->handle);

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
                auto scale = glm::vec3(p->life * p->scale * 0.5f + 0.5f);
                auto color = p->color * glm::vec4(1, 1, 1, v * p->alphaMultiplier);

                glUniform4fv(0, 1, (GLfloat*)&color);
                glm::mat4 translation = glm::translate(glm::mat4(1.f), p->position);
                glm::mat4 rotation = glm::rotate(glm::mat4(1.f), p->angle, { 0, 0, 1 });
                glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(translation));
                glUniform3f(2, scale.x, scale.y, scale.z);
                glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(rotation));
                glDrawArrays(GL_TRIANGLES, 0, 6);

                break;
            }
        }
    }
}
