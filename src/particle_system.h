#pragma once

#include "math.h"
#include "smallvec.h"
#include <vector>

class ParticleSystem
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

    std::vector<Particle> particles;

    RandomSeries series;

public:
    f32 minLife = 1.5f;
    f32 maxLife = 1.8f;
    f32 minAngle = 0.f;
    f32 maxAngle = M_PI * 2;
    f32 minScale = 0.9f;
    f32 maxScale = 1.1f;

    struct CurvePoint
    {
        f32 t;
        f32 v;
    };
    SmallVec<CurvePoint> alphaCurve = {
        { 0.f,  0.f   },
        { 0.1f, 0.25f },
        { 1.f,  0.f   }
    };

    void spawn(glm::vec3 const& position, glm::vec3 const& velocity, f32 alpha=1.f)
    {
        particles.push_back({
            position,
            velocity,
            random(series, minScale, maxScale),
            random(series, minAngle, maxAngle),
            0.f,
            random(series, minLife, maxLife),
            glm::vec4(1.f),
            alpha
        });
    }
    void update(f32 deltaTime);
    void draw();
};
