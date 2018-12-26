#pragma once

#include "misc.h"

class Scene
{
public:
    void onStart();
    void onEnd();
    void onUpdate(f32 deltaTime);
};
