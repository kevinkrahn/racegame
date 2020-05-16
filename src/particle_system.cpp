#include "particle_system.h"
#include "game.h"
#include "renderer.h"

void ParticleSystem::update(f32 deltaTime)
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

void ParticleSystem::onLitPass(Renderer* renderer)
{
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0);

    glUseProgram(renderer->getShaderProgram(lit ? "billboard" : "billboard_unlit"));
    glBindVertexArray(emptyVAO);
    glBindTextureUnit(0, texture->handle);

    for (auto p = particles.begin(); p != particles.end(); ++p)
    {
        f32 t = p->life / p->totalLife;
        f32 alphaCurveValue = getCurveValue(alphaCurve, t);
        f32 scaleCurveValue = getCurveValue(scaleCurve, t);

        //auto scale = glm::vec3(p->life * p->scale * 0.5f + 0.5f);
        auto scale = glm::vec3(scaleCurveValue * p->scale);
        auto color = p->color * glm::vec4(1, 1, 1, alphaCurveValue * p->alphaMultiplier);

        glUniform4fv(0, 1, (GLfloat*)&color);
        glm::mat4 translation = glm::translate(glm::mat4(1.f), p->position);
        glm::mat4 rotation = glm::rotate(glm::mat4(1.f), p->angle, { 0, 0, 1 });
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(translation));
        glUniform3f(2, scale.x, scale.y, scale.z);
        glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(rotation));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}
