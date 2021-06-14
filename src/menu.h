#pragma once

#include "misc.h"
#include "config.h"
#include "font.h"
#include "vehicle_data.h"
#include "input.h"

struct GarageData
{
    RenderWorld rw;

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
        RACE_RESULTS,
        STANDINGS,
        CHAMPIONSHIP_MENU,
        GARAGE_CAR_LOT,
        GARAGE_UPGRADES,
        GARAGE_PERFORMANCE,
        GARAGE_COSMETICS,
        GARAGE_COSMETICS_VINYL,
        GARAGE_WEAPON,
    };

    u32 mode = NONE;

    u32 currentWeaponSlotIndex = 0;
    u32 currentVinylLayer = 0;
    i32 vinylIndex = 0;
    Array<VinylPattern*> vinyls;

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
    bool isDialogOpen = false;

    void beginChampionship();
    void startQuickRace();
    void resetGarage();
    void openInitialCarLotMenu(u32 playerIndex);
    void updateVehiclePreviews();
    void openChampionshipMenu();
    void openVinylMenu(u32 layerIndex);

    void displayRaceResults();
    void displayStandings();

public:
    void openPauseMenu()
    {
        pauseTimer = 0.f;
        mode = PAUSE_MENU;
    }
    void openMainMenu()
    {
        mode = MAIN_MENU;
    }
    void openRaceResults();

    void onUpdate(class Renderer* renderer, f32 deltaTime);

    // TODO: don't do vehicle previews like this
    RenderWorld vehiclePreviews[10];
};
