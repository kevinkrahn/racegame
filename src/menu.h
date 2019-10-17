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
        CHAMPIONSHIP_STANDINGS,
        RACE_RESULTS,
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
    void championshipStandings();
    void raceResults();
    void mainOptions();
    void graphicsOptions();
    void audioOptions();
    void gameplayOptions();

    void drawBox(glm::vec2 pos, glm::vec2 size);

    RandomSeries series;

public:
    void showMainMenu() { menuMode = MenuMode::MAIN_MENU; }
    void showChampionshipMenu() { menuMode = MenuMode::CHAMPIONSHIP_MENU; }
    void showRaceResults() { menuMode = MenuMode::RACE_RESULTS; }
    void showOptionsMenu();
    void onUpdate(class Renderer* renderer, f32 deltaTime);
};
