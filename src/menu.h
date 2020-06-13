#pragma once

#include "misc.h"
#include "config.h"
#include "font.h"
#include "vehicle_data.h"
#include "input.h"

bool didSelect()
{
    bool result = g_input.isKeyPressed(KEY_RETURN);
    for (auto& pair : g_input.getControllers())
    {
        if (pair.value.isButtonPressed(BUTTON_A))
        {
            result = true;
            break;
        }
    }
    return result;
}

bool didGoBack()
{
    if (g_input.isKeyPressed(KEY_ESCAPE))
    {
        return true;
    }
    for (auto& c : g_input.getControllers())
    {
        if (c.value.isButtonPressed(BUTTON_B))
        {
            return true;
        }
    }
    return false;
}

namespace WidgetFlags
{
    enum
    {
        SELECTABLE          = 1 << 0, // can be navigated to
        NAVIGATE_VERTICAL   = 1 << 1, // can navigate up or down from this widget
        NAVIGATE_HORIZONTAL = 1 << 2, // can navigate left or right from this widget
        FADE_TO_BLACK       = 1 << 3, // screen will fade to black when widget is selected
        FADE_OUT            = 1 << 4, // all widgets will animate away when this widget is selected
        DISABLED            = 1 << 5, // it's as if the widget does not exist
        BACK                = 1 << 6, // triggered by escape or "X" or "O" on controller
        CANNOT_ACTIVATE     = 1 << 7, // clicking on this widget will play nono sound and do nothing
        TRANSIENT           = 1 << 8,
        FADE_OUT_TRANSIENT  = 1 << 9,
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

struct ImageButtonInfo
{
    bool isEnabled;
    bool isHighlighted;
    i32 maxUpgradeLevel;
    i32 upgradeLevel;
    const char* bottomText;
    bool flipImage = false;
};

struct SliderInfo
{
    glm::vec3 color1;
    glm::vec3 color2;
    f32 val;
    Texture* tex;
    f32 min;
    f32 max;
};

struct GarageData
{
    Driver* driver = nullptr;

    i32 previewVehicleIndex = -1;
    VehicleConfiguration previewVehicleConfig;
    VehicleTuning previewTuning;

    VehicleStats currentStats;
    VehicleStats upgradeStats;

    bool initialCarSelect = false;
    i32 playerIndex = -1;
};

class Menu
{
    enum MenuMode
    {
        VISIBLE,
        CHAMPIONSHIP_STANDINGS,
        RACE_RESULTS,
    } menuMode;

    Config tmpConfig;

    void championshipGarage();
    void championshipStandings();
    void raceResults();

    void drawBox(glm::vec2 pos, glm::vec2 size);

    SmallArray<Widget, 32> widgets;
    Widget* selectedWidget = nullptr;
    f32 repeatTimer = 0.f;
    f32 fadeInTimer = 0.f;
    bool fadeIn = true;
    f32 blackFadeAlpha = 0.f;
    GarageData garage;

    void reset()
    {
        selectedWidget = nullptr;
        widgets.clear();
        repeatTimer = 0.f;
        fadeIn = true;
        fadeInTimer = 0.f;
        menuMode = MenuMode::VISIBLE;
    }

    void resetTransient()
    {
        selectedWidget = nullptr;
        repeatTimer = 0.f;
        fadeIn = true;
        fadeInTimer = 0.f;
        for (auto it = widgets.begin(); it != widgets.end();)
        {
            if (it->flags & WidgetFlags::TRANSIENT)
            {
                widgets.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    i32 didChangeSelectionY();
    i32 didChangeSelectionX();

    Widget* addBackgroundBox(glm::vec2 pos, glm::vec2 size, f32 alpha=0.3f, bool scaleOut=true);
    Widget* addLogic(std::function<void()> onUpdate);
    Widget* addLogic(std::function<void(Widget&)> onUpdate);
    Widget* addButton(const char* text, const char* helpText, glm::vec2 pos, glm::vec2 size,
            std::function<void()> onSelect, u32 flags=0, Texture* image=nullptr,
            std::function<bool()> isEnabled=[]{ return true; });
    Widget* addImageButton(const char* text, const char* helpText, glm::vec2 pos, glm::vec2 size,
            std::function<void()> onSelect, u32 flags, Texture* image, f32 imageMargin,
            std::function<ImageButtonInfo(bool isSelected)> getInfo);
    Widget* addSlider(const char* text, glm::vec2 pos, glm::vec2 size,
            u32 flags, std::function<void(f32)> onValueChanged,
            std::function<SliderInfo()> getInfo);
    Widget* addSelector(const char* text, const char* helpText, glm::vec2 pos, glm::vec2 size,
            SmallArray<std::string> values, i32 valueIndex,
            std::function<void(i32 valueIndex)> onValueChanged);
    Widget* addHelpMessage(glm::vec2 pos);
    Widget* addLabel(std::function<const char*()> getText, glm::vec2 pos, class Font* font,
            HAlign halign=HAlign::CENTER, VAlign valign=VAlign::CENTER, glm::vec3 const& color=glm::vec3(1.f),
            u32 flags=0);
    Widget* addTitle(const char* text, glm::vec2 pos={0,-400});

    void createVehiclePreview();
    void createMainGarageMenu();
    void createPerformanceMenu();
    void createCosmeticsMenu();
    void createCarLotMenu();
    void createWeaponsMenu(WeaponType weaponType, i32& weaponSlot, u32& upgradeLevel);

public:
    void startQuickRace();
    void showMainMenu();
    void showNewChampionshipMenu();
    void showChampionshipMenu();
    void showGarageMenu();
    void showRaceResults() { menuMode = MenuMode::RACE_RESULTS; }
    void showSettingsMenu();
    void showGraphicsSettingsMenu();
    void showAudioSettingsMenu();
    void showGameplaySettingsMenu();
    void showControlsSettingsMenu();
    void showPauseMenu();
    void showInitialCarLotMenu(u32 playerIndex);
    void onUpdate(class Renderer* renderer, f32 deltaTime);
};
