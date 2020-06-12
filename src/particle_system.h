#pragma once

#include "misc.h"
#include "renderable.h"

template <typename T>
f32 getCurveValue(T const& curve, f32 t)
{
    for (u32 i=0; i<curve.size(); ++i)
    {
        auto& p2 = curve[i];
        if (t < p2.t)
        {
            auto& p1 = curve[i - 1];
            f32 result = p1.v + (p2.v - p1.v) * ((t - p1.t) / (p2.t - p1.t));
            return result;
        }
    }
    return 0.f;
}

class ParticleSystem : public Renderable
{
private:
    struct Particle
    {
        glm::vec3 position;
        glm::vec3 velocity;
        f32 scale = 1.f;
        f32 angle = 0.f;
        f32 life = 0.f;
        f32 totalLife = 0.f;
        glm::vec4 color = glm::vec4(1.f);
        f32 alphaMultiplier = 1.f;
    };

    Array<Particle> particles;

    RandomSeries series;

public:
    f32 minLife = 1.5f;
    f32 maxLife = 1.8f;
    f32 minAngle = 0.f;
    f32 maxAngle = PI2;
    f32 minScale = 0.9f;
    f32 maxScale = 1.1f;
    struct Texture* texture;
    bool lit = true;

    struct CurvePoint
    {
        f32 t;
        f32 v;
    };
    SmallArray<CurvePoint, 4> alphaCurve = {
        { 0.f,  0.f   },
        { 0.1f, 0.25f },
        { 1.f,  0.f   }
    };
    SmallArray<CurvePoint, 4> scaleCurve = {
        { 0.f, 0.5f },
        { 1.f, 1.f },
    };

    void spawn(glm::vec3 const& position, glm::vec3 const& velocity, f32 alpha,
            glm::vec4 const& color = glm::vec4(1.f), f32 scale = 1.f)
    {
        particles.push_back({
            position,
            velocity,
            random(series, minScale, maxScale) * scale,
            random(series, minAngle, maxAngle) * scale,
            0.f,
            random(series, minLife, maxLife),
            color,
            alpha
        });
    }

    i32 getPriority() const override { return 10000; }
    void update(f32 deltaTime);
    void clear() { particles.clear(); }
    void onLitPass(Renderer* renderer) override;

    std::string getDebugString() const override { return "Particle System"; }
};

struct ParticleEmitter
{
    RandomSeries series;

    ParticleSystem* particleSystem;
    i32 minCount = 0;
    i32 maxCount = 0;
    glm::vec4 color;
    f32 velocityVariance;
    f32 minVel = 4.f;
    f32 maxVel = 8.f;

    ParticleEmitter(ParticleSystem* particleSystem, i32 minCount, i32 maxCount,
            glm::vec4 const& color, f32 velocityVariance, f32 minVel, f32 maxVel) :
        particleSystem(particleSystem),
        minCount(minCount),
        maxCount(maxCount),
        color(color),
        velocityVariance(velocityVariance)
    {}

    ParticleEmitter() {}

    void emit(glm::vec3 const& position, glm::vec3 const& inheritedVelocity)
    {
        i32 count = irandom(series, minCount, maxCount);
        for (i32 i=0; i<count; ++i)
        {
            glm::vec3 vel = inheritedVelocity + glm::normalize(glm::vec3(
                random(series, -velocityVariance, velocityVariance),
                random(series, -velocityVariance, velocityVariance),
                random(series, -velocityVariance, velocityVariance))) * random(series, minVel, maxVel);
            particleSystem->spawn(position, vel, 1.f, color);
        }
    }
};
