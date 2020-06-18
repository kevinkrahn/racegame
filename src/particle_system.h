#pragma once

#include "misc.h"

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

class ParticleSystem
{
private:
    struct Particle
    {
        Vec3 position;
        Vec3 velocity;
        f32 scale = 1.f;
        f32 angle = 0.f;
        f32 life = 0.f;
        f32 totalLife = 0.f;
        Vec4 color = Vec4(1.f);
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

    void spawn(Vec3 const& position, Vec3 const& velocity, f32 alpha,
            Vec4 const& color = Vec4(1.f), f32 scale = 1.f)
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

    void update(f32 deltaTime);
    void clear() { particles.clear(); }
    void draw(class RenderWorld* rw);
};

struct ParticleEmitter
{
    RandomSeries series;

    ParticleSystem* particleSystem;
    i32 minCount = 0;
    i32 maxCount = 0;
    Vec4 color;
    f32 velocityVariance;
    f32 minVel = 4.f;
    f32 maxVel = 8.f;

    ParticleEmitter(ParticleSystem* particleSystem, i32 minCount, i32 maxCount,
            Vec4 const& color, f32 velocityVariance, f32 minVel, f32 maxVel) :
        particleSystem(particleSystem),
        minCount(minCount),
        maxCount(maxCount),
        color(color),
        velocityVariance(velocityVariance)
    {}

    ParticleEmitter() {}

    void emit(Vec3 const& position, Vec3 const& inheritedVelocity)
    {
        i32 count = irandom(series, minCount, maxCount);
        for (i32 i=0; i<count; ++i)
        {
            Vec3 vel = inheritedVelocity + normalize(Vec3(
                random(series, -velocityVariance, velocityVariance),
                random(series, -velocityVariance, velocityVariance),
                random(series, -velocityVariance, velocityVariance))) * random(series, minVel, maxVel);
            particleSystem->spawn(position, vel, 1.f, color);
        }
    }
};
