#pragma once

#include "misc.h"
#include "math.h"
#include "renderable.h"
#include "entity.h"

class Terrain : public Renderable, public Entity
{
    f32 tileSize = 3.f;
    f32 x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    std::unique_ptr<f32[]> heightBuffer = nullptr;

public:
    Terrain()
    {
        resize(-64, -64, 64, 64);
    }
    Terrain(f32 x1, f32 y1, f32 x2, f32 y2)
    {
        resize(x1, y1, x2, y2);
    }

    void render();
    void resize(f32 x1, f32 y1, f32 x2, f32 y2);
    void raise(glm::vec2 pos, f32 radius, f32 falloff, f32 amount);
    f32 getZ(glm::vec2 pos) const;
    f32 getMinZ() const { return 0.f; };
    //bool containsPoint(glm::vec2 p) const { return p.x >= x1 && p.y >= y1 && p.x <= x2 && p.y <= y2; };
    u32 getCellX(f32 x) const;
    u32 getCellY(f32 y) const;

    i32 getPriority() const override { return 0; }

    // entity
    void onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime) override {}

    // renderable
    void onBeforeRender(f32 deltaTime) override {};
    void onShadowPass(class Renderer* renderer) override {};
    void onDepthPrepass(class Renderer* renderer) override {};
    void onLitPass(class Renderer* renderer) override {};
};
