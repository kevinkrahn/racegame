#pragma once

#include "misc.h"
#include "config.h"

class Menu
{
    enum MenuMode
    {
        HIDDEN,
        MAIN_MENU,
        NEW_CHAMPIONSHIP,
        CHAMPIONSHIP_MENU,
        CHAMPIONSHIP_GARAGE,
        OPTIONS_MAIN,
        OPTIONS_GAMEPLAY,
        OPTIONS_GRAPHICS,
        OPTIONS_AUDIO,
    } menuMode = MAIN_MENU;

    Config tmpConfig;

    void mainMenu();
    void newChampionship();
    void championshipMenu();
    void championshipGarage();
    void mainOptions();
    void graphicsOptions();
    void audioOptions();
    void gameplayOptions();

    void drawBox(glm::vec2 pos, glm::vec2 size);

public:
    void showMainMenu() { menuMode = MenuMode::MAIN_MENU; }
    void showOptionsMenu();
    void onUpdate(class Renderer* renderer, f32 deltaTime);
};
