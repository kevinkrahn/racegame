#pragma once

#include "misc.h"

class Menu
{
    enum MenuMode
    {
        HIDDEN,
        MAIN_MENU,
    } menuMode = MAIN_MENU;

public:
    void showMainMenu() { menuMode = MenuMode::MAIN_MENU; }
    void onUpdate(class Renderer* renderer, f32 deltaTime);
};
