#pragma once

#include "math.h"

class Renderable {
public:
    virtual ~Renderable() {}

    virtual i32 getPriority() const { return 0; }

    virtual std::string getDebugString() const { return "Renderable"; };
    virtual void onShadowPassPriorityTransition(class Renderer* renderer) {};
    virtual void onShadowPass(class Renderer* renderer) {};
    virtual void onDepthPrepassPriorityTransition(class Renderer* renderer) {};
    virtual void onDepthPrepass(class Renderer* renderer) {};
    virtual void onLitPassPriorityTransition(class Renderer* renderer) {};
    virtual void onLitPass(class Renderer* renderer) {};
};

class Renderable2D {
public:
    virtual ~Renderable2D() {}

    virtual void on2DPass(class Renderer* renderer) {};
};

