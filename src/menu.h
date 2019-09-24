#pragma once

#include "misc.h"
#include "config.h"

class Menu
{
    enum MenuMode
    {
        HIDDEN,
        MAIN_MENU,
        OPTIONS_MAIN,
        OPTIONS_GAMEPLAY,
        OPTIONS_GRAPHICS,
        OPTIONS_AUDIO,
    } menuMode = MAIN_MENU;

    Config tmpConfig;

    void mainMenu();
    void mainOptions();
    void graphicsOptions();
    void audioOptions();
    void gameplayOptions();

public:
    void showMainMenu() { menuMode = MenuMode::MAIN_MENU; }
    void showOptionsMenu();
    void onUpdate(class Renderer* renderer, f32 deltaTime);
};
