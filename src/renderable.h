#pragma once

class Renderable2D {
public:
    virtual ~Renderable2D() {}

    virtual void on2DPass(class Renderer* renderer) {}
};

