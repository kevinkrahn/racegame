#pragma once

#include "math.h"

class Renderable {
public:
    virtual ~Renderable() {}

    virtual i32 getPriority() const { return 0; }

    virtual void onUpdate(f32 deltaTime) {};
    virtual void onShadowPass(class Renderer* renderer) {};
    virtual void onDepthPrepass(class Renderer* renderer) {};
    virtual void onLitPass(class Renderer* renderer) {};
};

class Renderable2D {
public:
    virtual ~Renderable2D() {}

    virtual i32 getPriority() const { return 0; }

    virtual void on2DPass(class Renderer* renderer) {};
};

