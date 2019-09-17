#pragma once

#include "misc.h"

class Menu
{
    i32 selectIndex = 0;
    enum MenuMode
    {
        HIDDEN,
        MAIN_MENU,
    } menuMode = MAIN_MENU;

public:
    void onUpdate(class Renderer* renderer, f32 deltaTime);
};
