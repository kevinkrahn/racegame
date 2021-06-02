#include "menu.h"
#include "resources.h"
#include "renderer.h"
#include "game.h"
#include "2d.h"
#include "input.h"
#include "scene.h"
#include "weapon.h"
#include "vehicle.h"
#include "widgets.h"

const char* championshipTracks[] = {
    // a
    "race1",
    "race2",
    "race3",
    "race4",
    "race11",
    "race1",
    "race2",
    "race3",
    "race4",
    "race11",

    // b
    "race5",
    "race6",
    "race7",
    "race8",
    "race9",
    "race10",
    "race5",
    "race6",
    "race7",
    "race8",
    "race9",
    "race10",
};

void Menu::startQuickRace()
{
    g_game.state.drivers.clear();
    RandomSeries series = randomSeed();
    i32 driverCredits = irandom(series, 10000, 50000);
    println("Starting quick race with driver budget: %i", driverCredits);
    g_game.state.drivers.clear();
    Array<AIDriverData*> aiDrivers;
    g_res.iterateResourceType(ResourceType::AI_DRIVER_DATA, [&](Resource* r) {
        AIDriverData* d = (AIDriverData*)r;
        if (d->vehicles.size() > 0)
        {
            bool valid = true;
            for (auto& v : d->vehicles)
            {
                if (!g_res.getVehicle(v.vehicleGuid))
                {
                    valid = false;
                }
            }
            if (valid)
            {
                aiDrivers.push(d);
            }
        }
    });
    assert(aiDrivers.size() >= 10);
    for (u32 i=0; i<10; ++i)
    {
        i32 aiIndex;
        do { aiIndex = irandom(series, 0, (i32)aiDrivers.size()); }
        while (g_game.state.drivers.findIf([&](Driver const& d){
            return d.aiDriverGUID == aiDrivers[aiIndex]->guid;
        }));
        g_game.state.drivers.push(Driver(false, false, false, 0, 0, aiDrivers[aiIndex]->guid));
        g_game.state.drivers.back().credits = driverCredits;
        g_game.state.drivers.back().aiUpgrades(series);
    }
    Driver& playerDriver = g_game.state.drivers[irandom(series, 0, (i32)g_game.state.drivers.size())];
    playerDriver.isPlayer = true;
    playerDriver.hasCamera = true;
    playerDriver.useKeyboard = true;

#if 0
    g_game.state.drivers[0].hasCamera = true;
    g_game.state.drivers[1].hasCamera = true;
    g_game.state.drivers[2].hasCamera = true;
    //g_game.state.drivers[3].hasCamera = true;
#endif

    g_game.state.gameMode = GameMode::QUICK_RACE;
    g_game.isEditing = false;
#if 0
    Scene* scene = g_game.changeScene("race11");
#else
    Scene* scene = g_game.changeScene(
            championshipTracks[irandom(series, 0, (i32)ARRAY_SIZE(championshipTracks))]);
#endif
    scene->startRace();
}

void Menu::showInitialCarLotMenu(u32 playerIndex)
{
    garage.initialCarSelect = true;
    garage.previewVehicle = nullptr;
    garage.driver = &g_game.state.drivers[playerIndex];
    mode = GARAGE_CAR_LOT;
}

void Menu::beginChampionship()
{
    g_game.isEditing = false;
    g_game.state.currentLeague = 0;
    g_game.state.currentRace = 0;
    g_game.state.gameMode = GameMode::CHAMPIONSHIP;
    g_game.changeScene(championshipTracks[g_game.state.currentRace]);

    // add AI drivers
    RandomSeries series = randomSeed();
    Array<AIDriverData*> aiDrivers;
    g_res.iterateResourceType(ResourceType::AI_DRIVER_DATA, [&](Resource* r) {
        AIDriverData* d = (AIDriverData*)r;
        if (d->vehicles.size() > 0)
        {
            bool valid = true;
            for (auto& v : d->vehicles)
            {
                if (!g_res.getVehicle(v.vehicleGuid))
                {
                    valid = false;
                }
            }
            if (valid)
            {
                aiDrivers.push(d);
            }
        }
    });
    for (i32 i=(i32)g_game.state.drivers.size(); i<10; ++i)
    {
        i32 aiIndex;
        do { aiIndex = irandom(series, 0, (i32)aiDrivers.size()); }
        while (g_game.state.drivers.findIf([&](Driver const& d){
            return d.aiDriverGUID == aiDrivers[aiIndex]->guid;
        }));
        g_game.state.drivers.push(Driver(false, false, false, 0, 0, aiDrivers[aiIndex]->guid));
        g_game.state.drivers.back().aiUpgrades(series);
    }
    garage.playerIndex = 0;
    showInitialCarLotMenu(garage.playerIndex);
}

void Menu::onUpdate(Renderer* renderer, f32 deltaTime)
{
    using namespace gui;

    constexpr f32 animationLength = 0.4f;

    static const char* helpMessage = "";
    static Function<void()> selection = []{ assert(false); };

    FontDescription smallFont = { "font", 20 };
    FontDescription mediumFont = { "font", 30 };
    FontDescription mediumFontBold = { "font_bold", 30 };
    FontDescription bigFont = { "font_bold", 60 };

    auto button = [&](gui::Widget* parent, const char* name, const char* helpText, auto onPress,
            u32 buttonFlags=0) {
        Vec2 btnSize(INFINITY, 60);
        return parent
            ->add(FadeAnimation(0.f, 1.f, animationLength, true, name))->size(btnSize)
            ->add(ScaleAnimation(0.7f, 1.f, animationLength, true))
            ->add(TextButton(mediumFont, name, buttonFlags))
                ->onPress(onPress)
                ->onSelect([helpText]{ helpMessage = helpText; });
    };

    auto panel = [&](gui::Widget* parent, Vec2 size, bool outline=true) {
        if (outline)
        {
            return parent->add(Outline(Insets(2), COLOR_OUTLINE_NOT_SELECTED))->size(0,0)
                         ->add(Container(Insets(40), {}, COLOR_BG_PANEL))->size(size);
        }
        return parent->add(Container(Insets(40), {}, COLOR_BG_PANEL))->size(size);
    };

    auto animateMenu = [](gui::Widget* parent, bool animateIn) {
        return parent
            ->add(Container({}, {}, Vec4(0), HAlign::CENTER, VAlign::CENTER))
            ->add(FadeAnimation(0.f, 1.f, animationLength, animateIn))
                ->onAnimationReverseEnd([&]{
                    selection();
                    animateIn = true;
                })
                ->size(0,0)
            ->add(ScaleAnimation(0.7f, 1.f, animationLength, animateIn))
                ->size(0, 0);
    };

    if (mode == MAIN_MENU)
    {
        static bool animateIn = true;
        auto inputCapture = gui::root->add(InputCapture(!animateIn, "Main Menu"));
        if (makeActive(inputCapture))
        {
            animateIn = true;
        }

        auto animation = animateMenu(inputCapture, animateIn);
        auto column = panel(animation, Vec2(600, 0))->add(Column(10))->size(0, 0);

        column
            ->add(Container(Insets(10), {}, Vec4(0), HAlign::CENTER))->size(INFINITY, 70)
            ->add(Text(bigFont, "MAIN MENU"));

        button(column, "NEW CHAMPIONSHIP", "Begin a new championship!", [&]{
            animateIn = false;
            selection = [&]{
                g_game.state = {};
                mode = NEW_CHAMPIONSHIP;
            };
        });
        button(column, "LOAD CHAMPIONSHIP", "Resume a previous championship.", [&]{
            /*
            animateIn = false;
            selection = [&] {
                g_game.loadGame();
                showChampionshipMenu();
                g_game.changeScene(championshipTracks[g_game.state.currentRace]);
            };
            */
        });
        button(column, "QUICK RACE", "Jump into a race without delay!", [&]{
            animateIn = false;
            fadeToBlack = true;
            selection = [&] {
                gui::clearInputCaptures();
                startQuickRace();
                fadeToBlack = false;
                mode = NONE;
            };
        });
        button(column, "SETTINGS", "Change things.", [&]{
            animateIn = false;
            selection = [&] {
                tmpConfig = g_game.config;
                mode = SETTINGS;
            };
        });
        button(column, "EDITOR", "Edit things.", [&]{
            animateIn = false;
            selection = [&] {
                clearInputCaptures();
                g_game.loadEditor();
                mode = NONE;
            };
        });
        button(column, "CHALLENGES", "Challenge things.", [&]{
        });
        button(column, "EXIT", "Terminate your racing experience.", [&]{
            g_game.shouldExit = true;
        });

        column
            ->add(Container(Insets(10), {}, Vec4(0,0,0,0.8f), HAlign::CENTER, VAlign::CENTER))
                ->size(INFINITY, 40)
            ->add(Text(smallFont, helpMessage));
    }
    else if (mode == SETTINGS)
    {
        static bool animateIn = true;
        auto inputCapture = gui::root->add(InputCapture(!animateIn, "Settings"));
        if (makeActive(inputCapture))
        {
            animateIn = true;
        }

        auto animation = animateMenu(inputCapture, animateIn);
        auto column = panel(animation, Vec2(600, 0))->add(Column(10))->size(Vec2(0));

        column
            ->add(Container(Insets(10), {}, Vec4(0), HAlign::CENTER))->size(INFINITY, 70)
            ->add(Text(bigFont, "SETTINGS"));

        button(column, "GRAPHICS SETTINGS", "Tweak graphics for performance or visual quality.", [&]{
        });
        button(column, "AUDIO SETTINGS", "Tweak audio settings.", [&]{
        });
        button(column, "CONTROLS", "Change controller or keyboard layout.", [&]{
        });
        button(column, "BACK", "Return to main menu.", [&]{
            animateIn = false;
            selection = [&] {
                popInputCapture();
                mode = MAIN_MENU;
            };
        }, ButtonFlags::BACK);

        column
            ->add(Container(Insets(10), {}, Vec4(0,0,0,0.8f), HAlign::CENTER, VAlign::CENTER))
                ->size(INFINITY, 40)
            ->add(Text(smallFont, helpMessage));
    }
    else if (mode == NEW_CHAMPIONSHIP)
    {
        static bool animateIn = true;
        auto inputCapture = gui::root->add(InputCapture(!animateIn, "New Championship"));
        if (makeActive(inputCapture))
        {
            animateIn = true;
        }

        auto animation = animateMenu(inputCapture, animateIn);
        auto column = panel(animation, Vec2(600, 0))->add(Column(10))->size(0, 0);

        column
            ->add(Container(Insets(10), {}, Vec4(0), HAlign::CENTER))->size(INFINITY, 70)
            ->add(Text(bigFont, "NEW CHAMPIONSHIP"));

        for (u32 i=0; i<4; ++i)
        {
            bool enabled = g_game.state.drivers.size() > i;
            auto c = column
                ->add(Border(Insets(2), COLOR_OUTLINE_NOT_SELECTED))->size(0,0)
                ->add(Container(Insets(10), {}, Vec4(0), HAlign::LEFT, VAlign::CENTER))
                    ->size(INFINITY, 70);
            if (enabled)
            {
                const char* icon = g_game.state.drivers[i].useKeyboard
                        ? "icon_keyboard" : "icon_controller";
                auto row = c->add(Row(10))->size(INFINITY, 0);
                row->add(Image(g_res.getTexture(icon)))->size(45, 45);
                row
                    ->add(Container({}, {}, Vec4(0), HAlign::LEFT, VAlign::CENTER))->size(0, INFINITY)
                    ->add(Text(mediumFont, tmpStr(g_game.state.drivers[i].playerName.data())));
            }
            else
            {
                c->add(Text(smallFont, "Empty Player Slot"));
            }
        }

        if (g_game.state.drivers.size() < 4)
        {
            if (g_input.isKeyPressed(KEY_SPACE))
            {
                bool keyboardPlayerExists = false;
                for (auto& driver : g_game.state.drivers)
                {
                    if (driver.useKeyboard)
                    {
                        keyboardPlayerExists = true;
                        break;
                    }
                }
                if (!keyboardPlayerExists)
                {
                    g_game.state.drivers.push(Driver(true, true, true, 0));
                    g_game.state.drivers.back().playerName = tmpStr("Player %u", g_game.state.drivers.size());
                }
            }

            for (auto& controller : g_input.getControllers())
            {
                if (controller.value.isAnyButtonPressed())
                {
                    bool controllerPlayerExists = false;
                    for (auto& driver : g_game.state.drivers)
                    {
                        if (driver.isPlayer && !driver.useKeyboard && driver.controllerID == controller.key)
                        {
                            controllerPlayerExists = true;
                            break;
                        }
                    }

                    if (!controllerPlayerExists)
                    {
                        g_game.state.drivers.push(Driver(true, true, false, controller.key));
                        g_game.state.drivers.back().controllerGuid = controller.value.getGuid();
                        g_game.state.drivers.back().playerName = tmpStr("Player %u", g_game.state.drivers.size());
                    }
                }
            }
        }

        // gap
        column->add(Container())->size(INFINITY, 20);

        // TODO: ensure all player names are unique before starting championship
        button(column, "BEGIN", "Begin the championship!", [&]{
            animateIn = false;
            fadeToBlack = true;
            selection = [&] {
                gui::clearInputCaptures();
                beginChampionship();
                fadeToBlack = false;
            };
        })->addFlags(g_game.state.drivers.empty() ? gui::WidgetFlags::DISABLED : 0);

        button(column, "CANCEL", "Return to main menu.", [&]{
            animateIn = false;
            selection = [&] {
                popInputCapture();
                mode = MAIN_MENU;
            };
        }, ButtonFlags::BACK);

        column
            ->add(Container(Insets(10), {}, Vec4(0,0,0,0.8f), HAlign::CENTER, VAlign::CENTER))
                ->size(INFINITY, 40)
            ->add(Text(smallFont, helpMessage));
    }
    else if (mode == PAUSE_MENU)
    {
        auto inputCapture = gui::root->add(InputCapture(pauseTimer < 0.1f, "Pause Menu"));
        makeActive(inputCapture);

        auto column = inputCapture
            ->add(Container({}, {}, Vec4(0), HAlign::CENTER, VAlign::CENTER))
            ->add(FadeAnimation(0.f, 1.f, animationLength, true))->size(0,0)
            ->add(Container(Insets(30), {}, Vec4(0, 0, 0, 0.8f)))->size(Vec2(450, 0))
            ->add(Column(12))->size(Vec2(0));

        column
            ->add(Container(Insets(0), {}, Vec4(0), HAlign::CENTER))->size(INFINITY, 55)
            ->add(Text(bigFont, "PAUSED"));

        auto pauseMenuButton = [&](const char* name, auto onPress, u32 buttonFlags=0) {
            Vec2 btnSize(INFINITY, 60);
            return column
                ->add(FadeAnimation(0.f, 1.f, animationLength, true, name))->size(btnSize)
                ->add(ScaleAnimation(0.7f, 1.f, animationLength, true))
                ->add(TextButton(mediumFont, name, buttonFlags))
                    ->onPress(onPress);
        };

        pauseMenuButton("RESUME", [&]{
            clearInputCaptures();
            g_game.currentScene->setPaused(false);
            mode = NONE;
        }, ButtonFlags::BACK);

        pauseMenuButton("FORFEIT RACE", [&]{
            clearInputCaptures();
            g_game.currentScene->setPaused(false);
            g_game.currentScene->forfeitRace();
            mode = NONE;
        });

        pauseMenuButton("QUIT TO DESKTOP", []{
            g_game.shouldExit = true;
        });
    }
    else if (mode == GARAGE_CAR_LOT || mode == GARAGE_UPGRADES)
    {
        const Vec2 VEHICLE_PREVIEW_SIZE = {600,400};
        const f32 SIDE_MENU_WIDTH = 400;

        static bool animateInGarage = true;
        auto menuContainer = animateMenu(gui::root, animateInGarage);

        auto column = menuContainer
            ->add(Container(Insets(0), {}, COLOR_BG_PANEL))
                ->size(VEHICLE_PREVIEW_SIZE.x, 0)
            ->add(Column())->size(INFINITY, 0);

        auto topContainer = column->add(Container())->size(0, 0);

        topContainer
            ->add(Container(Insets(20)))->size(INFINITY, 0)
            ->add(Text(mediumFont, tmpStr("%s's Garage", garage.driver->playerName.data())));
        topContainer
            ->add(Container({}, {}, Vec4(0), HAlign::RIGHT))->size(INFINITY, 0)
            ->add(Container(Insets(20), {}, Vec4(Vec3(0), 0.92f), HAlign::LEFT, VAlign::CENTER))
                ->size(0,0)
            ->add(Text(mediumFontBold,
                        tmpStr("CREDITS: %i", garage.driver->credits), COLOR_OUTLINE_SELECTED));

        // vehicle preview
        static RenderWorld rw;

        Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("Quad");
        rw.setName("Garage");
        rw.setSize(
                (u32)VEHICLE_PREVIEW_SIZE.x * gui::guiScale,
                (u32)VEHICLE_PREVIEW_SIZE.y * gui::guiScale);
        drawSimple(&rw, quadMesh, &g_res.white, Mat4::scaling(Vec3(20.f)), Vec3(0.02f));

        if (garage.previewVehicle)
        {
            Mat4 vehicleTransform = Mat4::rotationZ((f32)getTime());
            garage.previewVehicle->render(&rw, Mat4::translation(
                    Vec3(0, 0, garage.previewTuning.getRestOffset())) *
                vehicleTransform, nullptr, garage.previewVehicleConfig, &garage.previewTuning);
        }
        rw.setViewportCount(1);
        rw.addDirectionalLight(Vec3(-0.5f, 0.2f, -1.f), Vec3(1.0));
        rw.setViewportCamera(0, Vec3(8.f, -8.f, 10.f), Vec3(0.f, 0.f, 1.f), 1.f, 50.f, 30.f);
        g_game.renderer->addRenderWorld(&rw);

        column->add(Image(rw.getTexture(), true, true))->size(VEHICLE_PREVIEW_SIZE);

        // stats
        struct Stat
        {
            const char* name = nullptr;
            f32 value = 0.5f;
        };
        Stat stats[] = {
            { "ACCELERATION" },
            { "TOP SPEED" },
            { "ARMOR" },
            { "MASS" },
            { "GRIP" },
            { "OFFROAD" },
        };
        static f32 statsUpgrade[ARRAY_SIZE(stats)] = { 0 };
        f32 targetStats[] = {
            garage.currentStats.acceleration,
            garage.currentStats.topSpeed,
            garage.currentStats.armor,
            garage.currentStats.mass,
            garage.currentStats.grip,
            garage.currentStats.offroad,
        };
        f32 targetStatsUpgrade[] = {
            garage.upgradeStats.acceleration,
            garage.upgradeStats.topSpeed,
            garage.upgradeStats.armor,
            garage.upgradeStats.mass,
            garage.upgradeStats.grip,
            garage.upgradeStats.offroad,
        };

        auto statsColumn = column
            ->add(Container(Insets(20)))->size(INFINITY, 0)
            ->add(Column())->size(INFINITY, 0);
        for (u32 i=0; i<ARRAY_SIZE(stats); ++i)
        {
            stats[i].value = smoothMove(stats[i].value, targetStats[i], 8.f, g_game.deltaTime);
            statsUpgrade[i] = smoothMove(statsUpgrade[i], targetStatsUpgrade[i], 8.f, g_game.deltaTime);

            f32 barHeight = 10;
            f32 maxBarWidth = VEHICLE_PREVIEW_SIZE.x - 40;
            statsColumn->add(Text(smallFont, stats[i].name));
            statsColumn->add(Container())->size(0, 5);
            auto bar = statsColumn
                ->add(Container({}, {}, Vec4(0, 0, 0, 0.9f)))
                ->size(maxBarWidth, barHeight);

            f32 barWidth = maxBarWidth * stats[i].value;
            f32 upgradeBarWidth = maxBarWidth * statsUpgrade[i];

            if (upgradeBarWidth - barWidth > 0.001f)
            {
                bar->add(Container({}, {}, Vec4(0.01f, 0.7f, 0.01f, 0.9f)))
                   ->size(upgradeBarWidth, barHeight);
            }

            if (upgradeBarWidth - barWidth < -0.001f)
            {
                bar->add(Container({}, {}, Vec4(0.8f, 0.01f, 0.01f, 0.9f)))
                   ->size(barWidth, barHeight);
            }

            bar->add(Container({}, {}, Vec4(Vec3(0.7f), 1.f)))
               ->size(min(upgradeBarWidth, barWidth), barHeight);
            statsColumn->add(Container())->size(0, 12);
        }

        if (mode == GARAGE_CAR_LOT)
        {
            static bool animateIn = true;
            auto inputCapture = gui::root->add(InputCapture(!animateIn, "Garage Car Lot"));
            if (makeActive(inputCapture))
            {
                animateIn = true;
            }

            auto animation = animateMenu(inputCapture, animateIn);

	        Array<VehicleData*> vehicles;
	        g_res.iterateResourceType(ResourceType::VEHICLE, [&](Resource* r){
                auto v = (VehicleData*)r;
                if (v->showInCarLot)
                {
                    vehicles.push(v);
                }
	        });
	        vehicles.stableSort([](VehicleData* a, VehicleData* b) { return a->price < b->price; });

            if (!garage.previewVehicle)
            {
                garage.previewVehicle = vehicles[0];
                garage.previewVehicleConfig = VehicleConfiguration{};
                auto vd = garage.previewVehicle;
                garage.previewVehicleConfig.cosmetics.color =
                    srgb(hsvToRgb(vd->defaultColorHsv.x, vd->defaultColorHsv.y, vd->defaultColorHsv.z));
                garage.previewVehicleConfig.cosmetics.hsv = vd->defaultColorHsv;
                garage.previewVehicleConfig.reloadMaterials();
                vd->initTuning(garage.previewVehicleConfig, garage.previewTuning);
                garage.currentStats = garage.previewTuning.computeVehicleStats();
                garage.upgradeStats = garage.currentStats;
            }
        }
        else if (mode == GARAGE_UPGRADES)
        {
            //auto inputCapture = container->add(InputCapture(!animateIn, "GarageUpgrades"))->size(0,0);
            //makeActive(inputCapture);
        }
    }

    /*
    gui::root->add(FadeAnimation(1.f, 0.f, animationLength, fadeIn, "FadeToBlack"))
             ->add(Container(Vec4(0, 0, 0, 1)));
             */

    if (g_game.currentScene && !g_game.currentScene->isRaceInProgress && !g_game.isEditing)
    {
        Texture* tex = g_res.getTexture("checkers_fade");
        f32 w = (f32)g_game.windowWidth;
        f32 h = tex->height * 0.5f * gui::guiScale;
        ui::rectUVBlur(-200, tex, Vec2(0), {w,h}, Vec2(0.f, 0.999f),
                    Vec2(g_game.windowWidth/(tex->width * gui::guiScale * 0.5f), 0.001f));
        ui::rectUVBlur(-200, tex, Vec2(0, g_game.windowHeight-h), {w,h},
                    Vec2(0.f, 0.001f),
                    Vec2(g_game.windowWidth/(tex->width * gui::guiScale * 0.5f), 0.999f));
    }

    if (!fadeToBlack)
    {
        blackFadeAlpha = max(blackFadeAlpha - g_game.deltaTime * (1.f / (animationLength - 0.02f)), 0.f);
    }
    else
    {
        blackFadeAlpha = min(blackFadeAlpha + g_game.deltaTime * (1.f / (animationLength - 0.02f)), 1.f);
    }

    if (blackFadeAlpha > 0.f)
    {
        ui::rectBlur(10000, &g_res.white, {0,0}, Vec2(g_game.windowWidth, g_game.windowHeight),
                    Vec4(0,0,0,1), blackFadeAlpha);
    }

    pauseTimer += g_game.deltaTime;
}
