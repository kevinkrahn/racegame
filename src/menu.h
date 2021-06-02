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
        GARAGE_CAR_LOT,
        GARAGE_UPGRADES,
    };

    Config tmpConfig;
    GarageData garage;

    RenderWorld vehiclePreviews[10];

    f32 pauseTimer = 0.f;
    bool fadeToBlack = false;
    f32 blackFadeAlpha = 1.f;

    void beginChampionship();
    void startQuickRace();
    void showInitialCarLotMenu(u32 playerIndex);

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
