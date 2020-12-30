#pragma once

#include "../misc.h"

class ResourceEditor
{
public:
    virtual bool wantsExclusiveScreen() { return false; };
    virtual void init(class Resource* resource) {}
    virtual void onUpdate(class Resource* r, class ResourceManager* rm, class Renderer* renderer, f32 deltaTime)
    {}
};
