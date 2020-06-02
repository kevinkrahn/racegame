#pragma once

#include "misc.h"
#include "config.h"

namespace WidgetFlags
{
    enum
    {
        SELECTABLE          = 1 << 0, // can be navigated to
        NAVIGATE_VERTICAL   = 1 << 0, // can navigate up or down from this widget
        NAVIGATE_HORIZONTAL = 1 << 1, // can navigate left or right from this widget
        FADE_TO_BLACK       = 1 << 2, // screen will fade to black when widget is selected
        FADE_OUT            = 1 << 3, // all widgets will animate away when this widget is selected
        DISABLED            = 1 << 4,
    };
}

struct Widget
{
    const char* helpText = nullptr;
    glm::vec2 pos = {0,0};
    glm::vec2 size = {0,0};
    f32 hover = 0.f;
    f32 hoverTimer = 0.f;
    std::function<void()> onSelect;
    std::function<void(Widget& widget, bool isSelected)> onRender;
    f32 fadeInScale = 1.f;
    f32 fadeInAlpha = 0.f;
    u32 flags = 0;
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

    void championshipMenu();
    void championshipGarage();
    void championshipStandings();
    void raceResults();

    void drawBox(glm::vec2 pos, glm::vec2 size);

    Widget* selectedWidget = nullptr;
    SmallArray<Widget, 32> widgets;

    f32 repeatTimer = 0.f;
    f32 fadeInTimer = 0.f;
    bool fadeIn = true;
    f32 blackFadeAlpha = 0.f;

    void reset()
    {
        selectedWidget = nullptr;
        widgets.clear();
        repeatTimer = 0.f;
        fadeIn = true;
        blackFadeAlpha = 0.f;
    }

    i32 didChangeSelectionY();
    i32 didChangeSelectionX();

    Widget* addButton(const char* text, const char* helpText, glm::vec2 pos, glm::vec2 size,
            std::function<void()> onSelect, u32 flags=0);
    Widget* addHelpMessage(glm::vec2 pos);
    Widget* addLabel(const char* text, glm::vec2 pos, f32 width, class Font* font);

public:
    void startQuickRace();
    void showMainMenu();
    void showNewChampionshipMenu();
    void showRaceResults() { menuMode = MenuMode::RACE_RESULTS; }
    void showOptionsMenu();
    void onUpdate(class Renderer* renderer, f32 deltaTime);
};
