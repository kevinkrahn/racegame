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

void ParticleSystem::draw(RenderWorld* rw)
{
    static ShaderHandle shaderLit = getShaderHandle("billboard", { {"LIT"} });
    static ShaderHandle shaderUnlit = getShaderHandle("billboard", {});

    auto render = [](void* renderData){
        ParticleSystem* ps = (ParticleSystem*)renderData;

        glBindVertexArray(emptyVAO);
        glBindTextureUnit(0, ps->texture->handle);

        for (auto& p : ps->particles)
        {
            f32 t = p.life / p.totalLife;
            f32 alphaCurveValue = getCurveValue(ps->alphaCurve, t);
            f32 scaleCurveValue = getCurveValue(ps->scaleCurve, t);

            auto scale = Vec3(scaleCurveValue * p.scale);
            auto color = p.color * Vec4(1, 1, 1, alphaCurveValue * p.alphaMultiplier);

            glUniform4fv(0, 1, (GLfloat*)&color);
            Mat4 translation = Mat4::translation(p.position);
            Mat4 rotation = Mat4::rotationZ(p.angle);
            glUniformMatrix4fv(1, 1, GL_FALSE, translation.valuePtr());
            glUniform3f(2, scale.x, scale.y, scale.z);
            glUniformMatrix4fv(3, 1, GL_FALSE, rotation.valuePtr());
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    };
    rw->transparentPass({ lit ? shaderLit : shaderUnlit, TransparentDepth::PARTICLE_SYSTEM, this, render });
}
