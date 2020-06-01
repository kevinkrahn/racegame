#pragma once

#include "misc.h"
#include "config.h"

struct Button
{
    const char* helpText;
    glm::vec2 pos;
    glm::vec2 size;
    f32 hover = 0.f;
    f32 hoverTimer = 0.f;
    std::function<void()> onSelect;
    std::function<void(Button& button, bool isSelected)> onRender;
    f32 fadeInScale = 1.f;
    f32 fadeInAlpha = 0.f;

    bool fadeToBlackWhenSelected = false;
    bool fadeOutMenuWhenSelected = false;
};

class Menu
{
    enum MenuMode
    {
        MAIN_MENU,
        OPTIONS,
        NEW_CHAMPIONSHIP,
        HIDDEN,
        CHAMPIONSHIP_MENU,
        CHAMPIONSHIP_GARAGE,
        CHAMPIONSHIP_STANDINGS,
        RACE_RESULTS,
    } menuMode;

    Config tmpConfig;

    void newChampionship();
    void championshipMenu();
    void championshipGarage();
    void championshipStandings();
    void raceResults();

    void drawBox(glm::vec2 pos, glm::vec2 size);

    Button* selectedButton = nullptr;
    SmallArray<Button, 32> buttons;

    f32 repeatTimer = 0.f;
    f32 fadeInTimer = 0.f;
    bool fadeIn = true;
    f32 blackFadeAlpha = 0.f;
    bool fadeToBlack = false;

    i32 didChangeSelectionY();
    i32 didChangeSelectionX();

    Button* addButton(const char* text, const char* helpText, glm::vec2 pos, glm::vec2 size,
            std::function<void()> onSelect, bool fadeToBlackWhenSelected);

public:
    void startQuickRace();
    void showMainMenu();
    //void showChampionshipMenu() { menuMode = MenuMode::CHAMPIONSHIP_MENU; }
    void showRaceResults() { menuMode = MenuMode::RACE_RESULTS; }
    void showOptionsMenu();
    void onUpdate(class Renderer* renderer, f32 deltaTime);
};
