#pragma once

#include "misc.h"
#include "config.h"
#include "font.h"
#include "vehicle_data.h"
#include "input.h"

struct GarageData
{
    Driver* driver = nullptr;

    VehicleData* previewVehicle = nullptr;
    VehicleConfiguration previewVehicleConfig;
    VehicleTuning previewTuning;

    VehicleStats currentStats;
    VehicleStats upgradeStats;

    bool initialCarSelect = false;
    i32 playerIndex = -1;
};

class Menu
{
    enum
    {
        NONE,
        MAIN_MENU,
        SETTINGS,
        PAUSE_MENU,
        NEW_CHAMPIONSHIP,
        CHAMPIONSHIP_MENU,
        GARAGE_CAR_LOT,
        GARAGE_UPGRADES,
        GARAGE_PERFORMANCE,
        GARAGE_COSMETICS,
        GARAGE_WEAPON,
    };

    u32 currentWeaponSlotIndex = 0;

    struct Stat
    {
        const char* name = nullptr;
        f32 value = 0.5f;
    };

    f32 currentStats[6];
    f32 upgradeStats[6];

    Config tmpConfig;
    GarageData garage;

    f32 pauseTimer = 0.f;
    bool fadeToBlack = false;
    f32 blackFadeAlpha = 1.f;

    RenderWorld vehiclePreviews[10];

    void beginChampionship();
    void startQuickRace();
    void resetGarage();
    void showInitialCarLotMenu(u32 playerIndex);
    void updateVehiclePreviews();

public:
    void showPauseMenu()
    {
        pauseTimer = 0.f;
        mode = PAUSE_MENU;
    }
    void showMainMenu()
    {
        mode = MAIN_MENU;
    }
    void showRaceResults()
    {

    }
    void onUpdate(class Renderer* renderer, f32 deltaTime);

    u32 mode = NONE;
};
