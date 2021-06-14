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

constexpr f32 animationLength = 0.38f;
const gui::FontDescription smallFont = { "font", 20 };
const gui::FontDescription smallFontBold = { "font_bold", 20 };
const gui::FontDescription smallishFont = { "font", 24 };
const gui::FontDescription smallishFontBold = { "font_bold", 24 };
const gui::FontDescription mediumFont = { "font", 30 };
const gui::FontDescription mediumFontBold = { "font_bold", 30 };
const gui::FontDescription bigFont = { "font_bold", 58 };

static const char* helpMessage = "";

gui::Widget* animateButton(gui::Widget* parent, const char* name)
{
    return parent->add(gui::PositionDelay(0.65f, name))->size(0,0)
                 ->add(gui::FadeAnimation(0.f, 1.f, animationLength, true))->size(0,0)
                 ->add(gui::ScaleAnimation(0.7f, 1.f, animationLength, true))->size(0,0);
}

template <typename T>
gui::Button* button(gui::Widget* parent, const char* name, const char* helpText, T const& onPress,
        u32 buttonFlags=0, Texture* icon=nullptr) {
    return (gui::Button*)animateButton(parent, name)
        ->add(gui::TextButton(mediumFont, name, {}, nullptr, buttonFlags, icon))
            ->onPress(onPress)
            ->onGainedSelect([helpText]{
                if (helpText)
                {
                    helpMessage = helpText;
                }
            })
            ->size(Vec2(INFINITY, 70));
};

gui::Button* squareButton(gui::Widget* parent, Texture* tex, const char* name, const char* price,
        Vec2 size, i32 upgradeCount=0, i32 upgradeMax=0, f32 padding=0.f, bool flipImage=false) {
    auto btn = parent
        ->add(gui::PositionDelay(0.7f, name))->size(0,0)
        ->add(gui::FadeAnimation(0.f, 1.f, animationLength, true, name))->size(0,0)
        ->add(gui::ScaleAnimation(0.7f, 1.f, animationLength, true))->size(0,0)
        ->add(gui::Button(gui::ButtonFlags::NO_ACTIVE, name, 0.f))->size(size);
    btn->add(gui::Container(gui::Insets(padding)))->add(gui::Image(tex, false, false, flipImage));
    btn
        ->add(gui::Container(gui::Insets(10), {}, Vec4(0), HAlign::CENTER, VAlign::TOP))
        ->add(gui::Text(smallFont, name, gui::COLOR_TEXT));
    btn
        ->add(gui::Container(gui::Insets(10), {}, Vec4(0), HAlign::CENTER, VAlign::BOTTOM))
        ->add(gui::Text(smallFont, price, gui::COLOR_TEXT));
    if (upgradeMax > 0)
    {
        Texture* tex1 = g_res.getTexture("button");
        Texture* tex2 = g_res.getTexture("button_glow");
        f32 notchSize = 16;

        auto column = btn
            ->add(gui::Container(gui::Insets(10), {}, Vec4(0), HAlign::LEFT, VAlign::CENTER))
            ->add(gui::Column(5, HAlign::LEFT, VAlign::BOTTOM))->size(0, 0);
        for (i32 i=0; i<upgradeMax; ++i)
        {
            auto stack = column->add(gui::Stack())->size(notchSize, notchSize);
            stack->add(gui::Image(tex1, false));
            if (i < upgradeCount)
            {
                stack->add(gui::Image(tex2, false));
            }
        }
    }
    return (gui::Button*)btn;
};

gui::Button* garageButton(Menu* menu, gui::Widget* parent, u32 driverIndex, const char* description, Vec2 size) {
    Driver& driver = g_game.state.drivers[driverIndex];
    const char* name = driver.playerName.data();
    auto btn = parent
        ->add(gui::PositionDelay(0.7f, name))->size(0,0)
        ->add(gui::FadeAnimation(0.f, 1.f, animationLength, true, name))->size(0,0)
        ->add(gui::ScaleAnimation(0.7f, 1.f, animationLength, true))->size(0,0)
        ->add(gui::Button(0, name, 0.f))->size(size);
    // TODO: do the vehicle previews better somehow
    Texture* vehiclePreviewTex = menu->vehiclePreviews[driverIndex].getTexture();
    auto row = btn->add(gui::Row());
    row->add(gui::Image(vehiclePreviewTex, false, false, true))->size(size.y, size.y);
    auto col = row->add(gui::Container(gui::Insets(15)))->size(0,0)->add(gui::Column(15))->size(0,0);
    col->add(gui::Text(mediumFont, tmpStr("%s's Garage", name), gui::COLOR_TEXT));
    col->add(gui::Text(smallishFontBold, tmpStr("Credits: %i", driver.credits), gui::COLOR_TEXT));
    return (gui::Button*)btn;
};

gui::Widget* panel(gui::Widget* parent, Vec2 size, bool outline=true) {
    if (outline)
    {
        return parent->add(gui::Outline(gui::Insets(2), gui::COLOR_OUTLINE_NOT_SELECTED))->size(0,0)
                        ->add(gui::Container(gui::Insets(40), {}, gui::COLOR_BG_PANEL))->size(size);
    }
    return parent->add(gui::Container(gui::Insets(40), {}, gui::COLOR_BG_PANEL))->size(size);
};

gui::Animation* animateMenu(gui::Widget* parent, bool animateIn, Vec2 size=Vec2(INFINITY),
        f32 animLength = animationLength) {
    return (gui::Animation*)parent
        ->add(gui::Container({}, {}, Vec4(0), HAlign::CENTER, VAlign::CENTER))->size(size)
        ->add(gui::FadeAnimation(0.f, 1.f, animLength, animateIn))->size(0,0)
        ->add(gui::ScaleAnimation(0.7f, 1.f, animLength, animateIn))->size(0,0);
};

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
        if (d->usedForChampionshipAI && d->vehicles.size() > 0)
        {
            if (!d->vehicles.findIf([](AIVehicleConfiguration const& v) {
                return !g_res.getVehicle(v.vehicleGuid);
            })) {
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

void Menu::resetGarage()
{
    memset(currentStats, 0, sizeof(currentStats));
    memset(upgradeStats, 0, sizeof(upgradeStats));
}

void Menu::openInitialCarLotMenu(u32 playerIndex)
{
    garage.initialCarSelect = true;
    garage.previewVehicle = nullptr;
    garage.driver = &g_game.state.drivers[playerIndex];
    mode = GARAGE_CAR_LOT;
    resetGarage();
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
        if (d->usedForChampionshipAI && d->vehicles.size() > 0)
        {
            if (!d->vehicles.findIf([](AIVehicleConfiguration const& v) {
                return !g_res.getVehicle(v.vehicleGuid);
            })) {
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
    openInitialCarLotMenu(garage.playerIndex);
}

void Menu::updateVehiclePreviews()
{
    Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("Quad");
    for (u32 driverIndex=0; driverIndex<10; ++driverIndex)
    {
        RenderWorld& rw = vehiclePreviews[driverIndex];
        rw.setName(tmpStr("Vehicle Icon %i", driverIndex));
        u32 vehicleIconSize = (u32)(120 * gui::guiScale);
        rw.setSize(vehicleIconSize, vehicleIconSize);
        drawSimple(&rw, quadMesh, &g_res.white, Mat4::scaling(Vec3(20.f)), Vec3(0.02f));
        Driver& driver = g_game.state.drivers[driverIndex];
        if (driver.getVehicleData())
        {
            VehicleTuning t = driver.getTuning();
            driver.getVehicleData()->render(&rw, Mat4::translation(Vec3(0, 0, t.getRestOffset())),
                nullptr, *driver.getVehicleConfig(), &t);
        }
        rw.setViewportCount(1);
        rw.addDirectionalLight(Vec3(-0.5f, 0.2f, -1.f), Vec3(1.0));
        rw.setViewportCamera(0, Vec3(8.f, -8.f, 10.f), Vec3(0.f, 0.f, 1.f), 1.f, 50.f, 30.f);
        g_game.renderer->addRenderWorld(&rw);
    }
}

void Menu::openRaceResults()
{
    mode = RACE_RESULTS;
    updateVehiclePreviews();
    // generate fake race results
#if 0
    Scene* scene = g_game.currentScene.get();
    scene->getRaceResults().clear();
    RandomSeries series = randomSeed();
    g_game.state.drivers.clear();
    for (u32 i=0; i<10; ++i)
    {
        g_game.state.drivers.push(Driver(i==0, i==0, i==0, 0, i%2, 0));
    }
    for (u32 i=0; i<10; ++i)
    {
        RaceResult result;
        result.driver = &g_game.state.drivers[i];
        result.finishedRace = true;
        result.placement = i;
        result.statistics.accidents = irandom(series, 0, 10);
        result.statistics.destroyed = irandom(series, 0, 10);
        result.statistics.frags = irandom(series, 0, 50);
        result.statistics.bonuses.push(RaceBonus{ "SMALL BONUS", 100 });
        result.statistics.bonuses.push(RaceBonus{ "MEDIUM BONUS", 400 });
        result.statistics.bonuses.push(RaceBonus{ "BIG BONUS", 500 });
        scene->getRaceResults().push(result);
    }
#endif
}

void Menu::openChampionshipMenu()
{
    mode = CHAMPIONSHIP_MENU;
    updateVehiclePreviews();
}

void Menu::openVinylMenu(u32 layerIndex)
{
    static VinylPattern none;
    none.name = "None";
    none.guid = 0;
    none.colorTextureGuid = 0;
    vinyls.clear();
    g_res.iterateResourceType(ResourceType::VINYL_PATTERN, [this](Resource* r) {
        vinyls.push((VinylPattern*)r);
    });
    vinyls.sort([](VinylPattern* a, VinylPattern* b){ return a->name < b->name; });
    vinyls.insert(vinyls.begin(), &none);

    for (u32 i=0; i<vinyls.size(); ++i)
    {
        if (vinyls[i]->guid == garage.previewVehicleConfig.cosmetics.vinylGuids[currentVinylLayer])
        {
            vinylIndex = i;
            break;
        }
    }

    currentVinylLayer = layerIndex;

    mode = GARAGE_COSMETICS_VINYL;
}

void Menu::displayRaceResults()
{
    using namespace gui;

    auto inputCapture = gui::root->add(InputCapture("Race Results"));

    auto animation = animateMenu(inputCapture, inputCapture->isEntering());
    animation->onAnimationReverseEnd([&]{
        clearInputCaptures();

        if (g_game.state.gameMode == GameMode::CHAMPIONSHIP)
        {
            for (auto& r : g_game.currentScene->getRaceResults())
            {
                r.driver->lastPlacement = r.placement;
            }

            mode = STANDINGS;
            for (auto& row : g_game.currentScene->getRaceResults())
            {
                row.driver->credits += row.getCreditsEarned();
                row.driver->leaguePoints += row.getLeaguePointsEarned();
            }
            ++g_game.state.currentRace;
        }
        else
        {
            openMainMenu();
            g_game.currentScene->isCameraTourEnabled = true;
        }
    });

    static const f32 columnWidth[] = { 65, 195, 145, 145, 115, 115, 205 };
    static const char* columnTitle[] = {
        "NO.",
        "DRIVER",
        "ACCIDENTS",
        "DESTROYED",
        "FRAGS",
        "BONUS",
        "CREDITS EARNED"
    };
    Vec2 iconSize(58);

    auto column = animation
        ->add(gui::Container(Insets(0, 10), {}, gui::COLOR_BG_PANEL))->size(0, 0)
        ->add(Column(10, HAlign::CENTER))->size(0, 0);

    column
        ->add(Container(Insets(10)))->size(0, 70)
        ->add(Text(bigFont, "RACE RESULTS"));

    auto row = column->add(Row(8))->size(0,0);

    row->add(Container())->size(columnWidth[0], 0)
       ->add(Text(smallishFontBold, columnTitle[0]));
    row->add(Container())->size(iconSize.x, 0);
    row->add(Container())->size(columnWidth[1], 0)
       ->add(Text(smallishFontBold, columnTitle[1]));
    row->add(Container({}, {}, Vec4(0), HAlign::RIGHT))->size(columnWidth[2], 0)
       ->add(Text(smallishFontBold, columnTitle[2]));
    row->add(Container({}, {}, Vec4(0), HAlign::RIGHT))->size(columnWidth[3], 0)
       ->add(Text(smallishFontBold, columnTitle[3]));
    row->add(Container({}, {}, Vec4(0), HAlign::RIGHT))->size(columnWidth[4], 0)
       ->add(Text(smallishFontBold, columnTitle[4]));
    row->add(Container({}, {}, Vec4(0), HAlign::RIGHT))->size(columnWidth[5], 0)
       ->add(Text(smallishFontBold, columnTitle[5]));
    if (g_game.state.gameMode == GameMode::CHAMPIONSHIP)
    {
        row->add(Container({}, {}, Vec4(0), HAlign::RIGHT))->size(columnWidth[6], 0)
           ->add(Text(smallishFontBold, columnTitle[6]));
    }

    for (u32 i=0; i<g_game.state.drivers.size(); ++i)
    {
        auto row = column
            ->add(gui::PositionDelay(0.75f, tmpStr("row %i", i)))->size(0,0)
            ->add(gui::FadeAnimation(0.f, 1.f, animationLength, true))->size(0,0)
            ->add(gui::ScaleAnimation(0.7f, 1.f, animationLength, true))->size(0,0)
            ->add(Container(Insets(20, 0), {}, Vec4(0, 0, 0, 0.8f)))->size(0,0)
            ->add(Row(8, HAlign::LEFT, VAlign::CENTER))->size(0,0);

        auto& results = g_game.currentScene->getRaceResults()[i];
        FontDescription font = results.driver->isPlayer ? smallishFontBold : smallishFont;
        Vec4 color = results.driver->isPlayer ? gui::COLOR_OUTLINE_SELECTED : Vec4(1.f);
        i32 driverIndex = g_game.state.drivers.findIndexIf([&results](Driver& d) {
            return &d == results.driver;
        });

        row->add(Stack())->size(columnWidth[0], 0)
           ->add(Text(font, tmpStr("%i", results.placement + 1), color));
        row->add(Image(vehiclePreviews[driverIndex].getTexture(), false, false, true))->size(iconSize);
        row->add(Stack())->size(columnWidth[1], 0)
           ->add(Text(font, results.driver->playerName.data(), color));
        row->add(Container({}, {}, Vec4(0), HAlign::RIGHT))->size(columnWidth[2], 0)
           ->add(Text(font, tmpStr("%i", results.statistics.accidents), color));
        row->add(Container({}, {}, Vec4(0), HAlign::RIGHT))->size(columnWidth[3], 0)
           ->add(Text(font, tmpStr("%i", results.statistics.destroyed), color));
        row->add(Container({}, {}, Vec4(0), HAlign::RIGHT))->size(columnWidth[4], 0)
           ->add(Text(font, tmpStr("%i", results.statistics.frags), color));
        row->add(Container({}, {}, Vec4(0), HAlign::RIGHT))->size(columnWidth[5], 0)
           ->add(Text(font, tmpStr("%i", results.getBonus()), color));
        if (g_game.state.gameMode == GameMode::CHAMPIONSHIP)
        {
            row->add(Container({}, {}, Vec4(0), HAlign::RIGHT))->size(columnWidth[6], 0)
               ->add(Text(font, tmpStr("%i", results.getCreditsEarned()), color));
        }
    }

    button(column, "OKAY", "", [this, inputCapture]{
        g_audio.playSound(g_res.getSound("close"), SoundType::MENU_SFX);
        inputCapture->setEntering(false);
    }, ButtonFlags::BACK)->size(250, 70);
}

void Menu::displayStandings()
{
    using namespace gui;

    auto inputCapture = gui::root->add(InputCapture("Standings"));

    auto animation = animateMenu(inputCapture, inputCapture->isEntering());
    animation->onAnimationReverseEnd([&]{
        fadeToBlack = false;
        clearInputCaptures();
        openChampionshipMenu();
        RandomSeries series = randomSeed();
        if (g_game.currentScene->guid != g_res.getTrackGuid(championshipTracks[g_game.state.currentRace]))
        {
            for (auto& driver : g_game.state.drivers)
            {
                if (!driver.isPlayer)
                {
                    driver.aiUpgrades(series);
                }
            }
            g_game.saveGame();
            g_game.changeScene(championshipTracks[g_game.state.currentRace]);
        }
    });

    auto column = animation
        ->add(gui::Container(Insets(0, 10), {}, gui::COLOR_BG_PANEL))->size(0, 0)
        ->add(Column(10, HAlign::CENTER))->size(0, 0);

    column
        ->add(Container(Insets(10)))->size(0, 70)
        ->add(Text(bigFont, "STANDINGS"));

    SmallArray<Driver*, 10> sortedDrivers;
    for(auto& driver : g_game.state.drivers)
    {
        sortedDrivers.push(&driver);
    }
    sortedDrivers.sort([](Driver* a, Driver* b) { return a->leaguePoints > b->leaguePoints; });

    for (u32 i=0; i<sortedDrivers.size(); ++i)
    {
        Driver* driver = sortedDrivers[i];
        FontDescription font = driver->isPlayer ? smallishFontBold : smallishFont;
        Vec4 color = driver->isPlayer ? COLOR_OUTLINE_SELECTED : Vec4(1.f);

        auto row = column
            ->add(gui::PositionDelay(0.75f, tmpStr("row %i", i)))->size(0,0)
            ->add(gui::FadeAnimation(0.f, 1.f, animationLength, true))->size(0,0)
            ->add(gui::ScaleAnimation(0.7f, 1.f, animationLength, true))->size(0,0)
            ->add(Container(Insets(20, 0), {}, Vec4(0, 0, 0, 0.8f)))->size(0,0)
            ->add(Row(8, HAlign::LEFT, VAlign::CENTER))->size(0,0);

        row->add(Stack())->size(50, 0)
           ->add(Text(font, tmpStr("%i", i+1), color));

        Vec2 iconSize(64);
        row->add(Image(vehiclePreviews[driver - g_game.state.drivers.begin()].getTexture(), false,
                false, true))->size(iconSize);

        row->add(Stack())->size(200, 0)
           ->add(Text(font, driver->playerName.data(), color));

        row->add(Container({}, {}, Vec4(0), HAlign::RIGHT))->size(220, 0)
           ->add(Text(mediumFontBold, tmpStr("%i", driver->leaguePoints), color));
    }

    button(column, "OKAY", "", [this, inputCapture]{
        g_audio.playSound(g_res.getSound("close"), SoundType::MENU_SFX);
        inputCapture->setEntering(false);
        if (g_game.currentScene->guid != g_res.getTrackGuid(championshipTracks[g_game.state.currentRace]))
        {
            fadeToBlack = true;
        }
    }, ButtonFlags::BACK)->size(250, 70);
}

void Menu::onUpdate(Renderer* renderer, f32 deltaTime)
{
    using namespace gui;

    if (mode == MAIN_MENU)
    {
        auto inputCapture = gui::root->add(InputCapture("Main Menu"));

        static Function<void()> selection = []{ assert(false); };
        auto animation = animateMenu(inputCapture, inputCapture->isEntering())
            ->onAnimationReverseEnd([&]{
                selection();
            });
        auto column = panel(animation, Vec2(600, 0))->add(Column(10))->size(0, 0);

        column
            ->add(Container(Insets(10), {}, Vec4(0), HAlign::CENTER))->size(INFINITY, 70)
            ->add(Text(bigFont, "MAIN MENU"));

        button(column, "NEW CHAMPIONSHIP", "Begin a new championship!", [&]{
            inputCapture->setEntering(false);
            selection = [&]{
                g_game.state = {};
                mode = NEW_CHAMPIONSHIP;
            };
        });
        button(column, "LOAD CHAMPIONSHIP", "Resume a previous championship.", [&]{
            inputCapture->setEntering(false);
            fadeToBlack = true;
            selection = [&] {
                g_game.loadGame();
                clearInputCaptures();
                g_game.changeScene(championshipTracks[g_game.state.currentRace]);
                openChampionshipMenu();
                fadeToBlack = false;
            };
        });
        button(column, "QUICK RACE", "Jump into a race without delay!", [&]{
            inputCapture->setEntering(false);
            fadeToBlack = true;
            selection = [&] {
                gui::clearInputCaptures();
                startQuickRace();
                fadeToBlack = false;
                mode = NONE;
            };
        });
        button(column, "SETTINGS", "Change things.", [&]{
            inputCapture->setEntering(false);
            selection = [&] {
                tmpConfig = g_game.config;
                mode = SETTINGS;
            };
        });
        button(column, "EDITOR", "Edit things.", [&]{
            inputCapture->setEntering(false);
            selection = [&] {
                clearInputCaptures();
                g_game.loadEditor();
                mode = NONE;
            };
        });
        button(column, "CHALLENGES", "Challenge things.", [&]{
            inputCapture->setEntering(false);
            selection = [&] {
                clearInputCaptures();
                openRaceResults();
            };
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
        auto inputCapture = gui::root->add(InputCapture("Settings"));

        static Function<void()> selection = []{ assert(false); };
        auto animation = animateMenu(inputCapture, inputCapture->isEntering())
            ->onAnimationReverseEnd([&]{ selection(); });
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
            inputCapture->setEntering(false);
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
        auto inputCapture = gui::root->add(InputCapture("New Championship"));

        static Function<void()> selection = []{ assert(false); };
        auto animation = animateMenu(inputCapture, inputCapture->isEntering())
            ->onAnimationReverseEnd([&]{ selection(); });
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
            inputCapture->setEntering(false);
            fadeToBlack = true;
            selection = [&] {
                gui::clearInputCaptures();
                beginChampionship();
                fadeToBlack = false;
            };
        })->addFlags(g_game.state.drivers.empty() ? gui::WidgetFlags::DISABLED : 0);

        button(column, "CANCEL", "Return to main menu.", [&]{
            inputCapture->setEntering(false);
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
        auto inputCapture = gui::root->add(InputCapture("Pause Menu", pauseTimer < 0.1f));

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
                ->add(TextButton(mediumFont, name, {}, nullptr, buttonFlags))
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
        });

        pauseMenuButton("QUIT TO DESKTOP", []{
            g_game.shouldExit = true;
        });
    }
    else if (mode >= GARAGE_CAR_LOT)
    {
        const Vec2 VEHICLE_PREVIEW_SIZE = {600,400};
        const f32 SIDE_MENU_WIDTH = 500;
        const f32 SEP = 20;

        static bool animateInGarage = true;

        // blurred background
        gui::root
            ->add(FadeAnimation(0.f, 1.f, animationLength, animateInGarage))
            ->add(Container({}, {}, Vec4(0, 0, 0, 0.2f)));

        Widget* topLevelColumn = gui::root->add(Container({}, {}, Vec4(0), HAlign::CENTER, VAlign::CENTER))
            ->add(Column(20))->size(0,0);
        Widget* topLevelRow = topLevelColumn->add(Row(SEP))->size(0,0);
        Widget* menuContainer = animateMenu(topLevelRow, animateInGarage, Vec2(VEHICLE_PREVIEW_SIZE.x, 0));

        auto helpMessageContainer = topLevelColumn
            ->add(Container({}, {}, Vec4(0), HAlign::CENTER, VAlign::TOP))
                ->size(VEHICLE_PREVIEW_SIZE.x + SIDE_MENU_WIDTH + SEP, 40);

        if (helpMessage && strlen(helpMessage) > 0)
        {
            helpMessageContainer
                ->add(Container(Insets(15), {}, Vec4(0,0,0,0.5f)))->size(0,0)
                ->add(Text(smallFont, helpMessage));
        }

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
        Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("Quad");
        RenderWorld& rw = garage.rw;
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

        auto previewContainer = column->add(Container())->size(0,0);
        previewContainer->add(Image(rw.getTexture(), true, false, true))->size(VEHICLE_PREVIEW_SIZE);
        if (mode == GARAGE_CAR_LOT)
        {
            auto column = previewContainer
                ->add(Container(Insets(20)))->size(0, 0)
                ->add(Column(8))->size(0,0);

            if (!garage.driver->getVehicleData())
            {
                if (garage.previewVehicle)
                {
                    column->add(Text(smallishFontBold, tmpStr("PRICE: %i", garage.previewVehicle->price)));
                }
            }
            else if (garage.driver->getVehicleData() != garage.previewVehicle)
            {
                column->add(Text(smallishFont, tmpStr("PRICE: %i", garage.previewVehicle->price)));
                column->add(Text(smallishFont, tmpStr("TRADE: %i", garage.driver->getVehicleValue())));
                column->add(Text(smallishFontBold, tmpStr("TOTAL: %i",
                                garage.previewVehicle->price - garage.driver->getVehicleValue())));
            }
            else
            {
                column->add(Text(smallishFontBold,
                            tmpStr("VALUE: %i", garage.driver->getVehicleValue())));
            }
        }

        column->add(Container(Insets(10), {}, Vec4(0, 0, 0, 0.92f), HAlign::CENTER))->size(INFINITY, 0)
              ->add(Text(smallishFontBold,
                          garage.previewVehicle ? garage.previewVehicle->name.data() : ""));

        // stats
        const char* statNames[] = {
            "ACCELERATION",
            "TOP SPEED",
            "ARMOR",
            "MASS",
            "GRIP",
            "OFFROAD",
        };
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
        for (u32 i=0; i<ARRAY_SIZE(targetStats); ++i)
        {
            currentStats[i] = smoothMove(currentStats[i], targetStats[i], 10.f, g_game.deltaTime);
            upgradeStats[i] = smoothMove(upgradeStats[i], targetStatsUpgrade[i], 10.f, g_game.deltaTime);

            f32 barHeight = 6;
            f32 maxBarWidth = VEHICLE_PREVIEW_SIZE.x - 40;
            auto row = statsColumn->add(Row(8))->size(0,0);
            row->add(Text(smallFontBold, tmpStr("%s:", statNames[i])));
            row->add(Text(smallFont, tmpStr("%.1f", currentStats[i] * 100)));

            Vec4 green = Vec4(0.01f, 0.7f, 0.01f, 0.9f);
            Vec4 red = Vec4(0.8f, 0.01f, 0.01f, 0.9f);
            if (absolute(currentStats[i] - upgradeStats[i]) > 0.001f)
            {
                row->add(Text(smallFontBold,
                    tmpStr("%s%.1f", currentStats[i] < upgradeStats[i] ? "+" : "-",
                        absolute(currentStats[i] - upgradeStats[i]) * 100),
                    currentStats[i] < upgradeStats[i] ? green : red));
            }
            statsColumn->add(Container())->size(0, 5);
            auto bar = statsColumn
                ->add(Container({}, {}, Vec4(0, 0, 0, 0.9f)))
                ->size(maxBarWidth, barHeight);

            f32 barWidth = maxBarWidth * currentStats[i];
            f32 upgradeBarWidth = maxBarWidth * upgradeStats[i];

            if (upgradeBarWidth - barWidth > 0.001f)
            {
                bar->add(Container({}, {}, green))
                   ->size(upgradeBarWidth, barHeight);
            }

            if (upgradeBarWidth - barWidth < -0.001f)
            {
                bar->add(Container({}, {}, red))
                   ->size(barWidth, barHeight);
            }

            bar->add(Container({}, {}, Vec4(Vec3(0.7f), 1.f)))
               ->size(min(upgradeBarWidth, barWidth), barHeight);
            statsColumn->add(Container())->size(0, 12);
        }

        if (mode == GARAGE_CAR_LOT)
        {
            auto inputCapture =
                (InputCapture*)topLevelRow->add(InputCapture("Garage Car Lot"))->size(0,0);

            static Function<void()> selection = []{ assert(false); };
            menuContainer = animateMenu(inputCapture, inputCapture->isEntering(), Vec2(SIDE_MENU_WIDTH, 0))
                ->onAnimationReverseEnd([&]{ selection(); });

            auto column = menuContainer
                ->add(Container(Insets(0), {}, Vec4(0)))->size(0,0)
                ->add(Column(10))->size(INFINITY, 0);

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

            auto grid = column->add(Grid(3, 4))->size(SIDE_MENU_WIDTH, 0);
            static Array<RenderWorld> carLotRenderWorlds(vehicles.size());
            static bool arePreviewsRendered = false;
            Vec2 size(SIDE_MENU_WIDTH / 3);
            for (i32 i=0; i<(i32)vehicles.size(); ++i)
            {
                RenderWorld& rw = carLotRenderWorlds[i];
                VehicleData* v = vehicles[i];

                if (!arePreviewsRendered)
                {
                    Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("Quad");
                    rw.setName("Garage");
                    rw.setBloomForceOff(true);
                    rw.setSize((u32)size.x * gui::guiScale * 2, (u32)size.y * gui::guiScale * 2);
                    drawSimple(&rw, quadMesh, &g_res.white, Mat4::scaling(Vec3(20.f)), Vec3(0.02f));

                    VehicleConfiguration vehicleConfig;
                    vehicleConfig.cosmetics.color =
                        srgb(hsvToRgb(v->defaultColorHsv.x, v->defaultColorHsv.y, v->defaultColorHsv.z));
                    vehicleConfig.cosmetics.hsv = v->defaultColorHsv;

                    VehicleTuning tuning;
                    v->initTuning(vehicleConfig, tuning);
                    Mat4 vehicleTransform = Mat4::rotationZ(0.f);
                    v->render(&rw, Mat4::translation(Vec3(0, 0, tuning.getRestOffset())) *
                        vehicleTransform, nullptr, vehicleConfig, &tuning);
                    rw.setViewportCount(1);
                    rw.addDirectionalLight(Vec3(-0.5f, 0.2f, -1.f), Vec3(1.0));
                    rw.setViewportCamera(0, Vec3(8.f, -8.f, 10.f), Vec3(0.f, 0.f, 1.f), 1.f, 50.f, 30.f);
                    renderer->addRenderWorld(&rw);
                }

                i32 totalCost = v->price - garage.driver->getVehicleValue();
                bool enabled = v->guid == garage.driver->vehicleGuid ||
                    garage.driver->credits >= totalCost;

                auto btn = squareButton(grid, rw.getTexture(), v->name.data(),
                        garage.driver->getVehicleData() == v ? "OWNED" : tmpStr("%i", v->price),
                        size, 0, 0, 0.f, true);
                btn->addFlags(enabled ? 0 : WidgetFlags::FADED);
                btn->onPress([v, this]{
                    garage.previewVehicle = v;
                    if (garage.previewVehicle->guid == garage.driver->vehicleGuid)
                    {
                        garage.previewVehicleConfig = *garage.driver->getVehicleConfig();
                        garage.previewTuning = garage.driver->getTuning();
                        garage.currentStats = garage.previewTuning.computeVehicleStats();
                        garage.upgradeStats = garage.currentStats;
                    }
                    else
                    {
                        garage.previewVehicleConfig = VehicleConfiguration{};
                        auto vd = garage.previewVehicle;
                        // TODO: this color logic (and this other stuff) should be encapsulated somewhere
                        garage.previewVehicleConfig.cosmetics.color =
                            srgb(hsvToRgb(vd->defaultColorHsv.x, vd->defaultColorHsv.y, vd->defaultColorHsv.z));
                        garage.previewVehicleConfig.cosmetics.hsv = vd->defaultColorHsv;
                        garage.previewVehicleConfig.reloadMaterials();
                        // TODO: cache tuning data
                        garage.previewVehicle->initTuning(garage.previewVehicleConfig, garage.previewTuning);
                        garage.currentStats = garage.previewTuning.computeVehicleStats();
                        garage.upgradeStats = garage.currentStats;
                    }
                });
                btn->onGainedSelect([v]{ helpMessage = v->description.data(); });
            }
            arePreviewsRendered = true;

            column->add(Container())->size(0, 20);

            i32 totalCost = garage.previewVehicle->price - garage.driver->getVehicleValue();
            bool canBuy = garage.previewVehicle->guid != garage.driver->vehicleGuid &&
                garage.driver->credits >= totalCost;
            button(column, "BUY CAR", "", [totalCost, this]{
                garage.driver->credits -= totalCost;
                garage.driver->vehicleGuid = garage.previewVehicle->guid;
                garage.driver->vehicleConfig = garage.previewVehicleConfig;
            })->addFlags(canBuy ? 0 : WidgetFlags::DISABLED);

            button(column, "DONE", "", [&]{
                inputCapture->setEntering(false);
                selection = [&]{
                    garage.previewVehicle = garage.driver->getVehicleData();
                    garage.previewVehicleConfig = *garage.driver->getVehicleConfig();
                    garage.previewTuning = garage.driver->getTuning();
                    garage.currentStats = garage.previewTuning.computeVehicleStats();
                    garage.upgradeStats = garage.currentStats;
                    mode = GARAGE_UPGRADES;
                    gui::popInputCapture();
                };
            }, ButtonFlags::BACK)
                ->addFlags(garage.driver->getVehicleData()
                        ? WidgetFlags::DEFAULT_SELECTION : WidgetFlags::DISABLED);
        }
        else if (mode == GARAGE_UPGRADES)
        {
            auto inputCapture =
                (InputCapture*)topLevelRow->add(InputCapture("Garage Upgrades"))->size(0,0);

            static Function<void()> selection = []{ assert(false); };
            menuContainer = animateMenu(inputCapture, inputCapture->isEntering(), Vec2(SIDE_MENU_WIDTH, 0))
                ->onAnimationReverseEnd([&]{ selection(); });

            auto column = menuContainer
                ->add(Container(Insets(0), {}, Vec4(0)))->size(0,0)
                ->add(Column(10))->size(INFINITY, 0);

            Texture* iconbg = g_res.getTexture("iconbg");

            button(column, "PERFORMANCE", "Upgrades to enhance your vehicle's performance.", [&]{
                inputCapture->setEntering(false);
                selection = [this] {
                    mode = GARAGE_PERFORMANCE;
                };
            }, 0, g_res.getTexture("icon_engine"));

            button(column, "COSMETICS", "Change your vehicle's appearance.", [&]{
                inputCapture->setEntering(false);
                selection = [this] {
                    mode = GARAGE_COSMETICS;
                };
            }, 0, g_res.getTexture("icon_spraycan"));

            for (u32 i=0; i<garage.driver->getVehicleData()->weaponSlots.size(); ++i)
            {
                i32 installedWeapon = garage.previewVehicleConfig.weaponIndices[i];
                WeaponSlot& slot = garage.driver->getVehicleData()->weaponSlots[i];
                animateButton(column, slot.name.data())
                    ->add(TextButton(mediumFont, slot.name.data(), smallFont,
                        installedWeapon == -1 ? "None" : g_weapons[installedWeapon].info.name, 0,
                        installedWeapon == -1 ? iconbg : g_weapons[installedWeapon].info.icon))
                    ->onPress([this, i, inputCapture]{
                        inputCapture->setEntering(false);
                        currentWeaponSlotIndex = i;
                        selection = [this] {
                            mode = GARAGE_WEAPON;
                        };
                    })
                    ->onGainedSelect([]{
                        helpMessage = "Install or upgrade a weapon.";
                    })
                    ->size(INFINITY, 70);
            }

            button(column, "CAR LOT", "Buy a new vehicle!", [&]{
                inputCapture->setEntering(false);
                selection = [this] {
                    mode = GARAGE_CAR_LOT;
                };
            });

            button(column, "DONE", nullptr, [&]{
                animateInGarage = false;
                inputCapture->setEntering(false);
                selection = [this] {
                    animateInGarage = true;
                    gui::popInputCapture();
                    if (garage.initialCarSelect)
                    {
                        garage.playerIndex += 1;
                        i32 playerCount = 0;
                        for (auto& driver : g_game.state.drivers)
                        {
                            if (driver.isPlayer)
                            {
                                ++playerCount;
                            }
                        }
                        if (garage.playerIndex >= playerCount)
                        {
                            garage.initialCarSelect = false;
                            openChampionshipMenu();
                        }
                        else
                        {
                            openInitialCarLotMenu(garage.playerIndex);
                        }
                    }
                    else
                    {
                        openChampionshipMenu();
                    }
                };
            }, ButtonFlags::BACK);
        }
        else if (mode == GARAGE_PERFORMANCE)
        {
            auto inputCapture =
                (InputCapture*)topLevelRow->add(InputCapture("Garage Performance"))->size(0,0);

            static Function<void()> selection = []{ assert(false); };
            menuContainer = animateMenu(inputCapture, inputCapture->isEntering(), Vec2(SIDE_MENU_WIDTH, 0))
                ->onAnimationReverseEnd([&]{ selection(); });

            auto column = menuContainer
                ->add(Container(Insets(0), {}, Vec4(0)))->size(0,0)
                ->add(Column(10))->size(INFINITY, 0);

            auto grid = column->add(Grid(3, 4))->size(SIDE_MENU_WIDTH, 0);
            Vec2 size(SIDE_MENU_WIDTH / 3);

            for (i32 i=0; i<(i32)garage.driver->getVehicleData()->availableUpgrades.size(); ++i)
            {
                auto& upgrade = garage.driver->getVehicleData()->availableUpgrades[i];
                auto currentUpgrade = garage.driver->getVehicleConfig()->performanceUpgrades.findIf(
                        [i](auto& u) { return u.upgradeIndex == i; });
                bool isEquipped = !!currentUpgrade;
                i32 upgradeLevel = 0;
                i32 price = upgrade.price;
                if (isEquipped)
                {
                    upgradeLevel = currentUpgrade->upgradeLevel;
                    price = upgrade.price * (upgradeLevel + 1);
                }

                bool isPurchasable = garage.driver->credits >= price && upgradeLevel < upgrade.maxUpgradeLevel;

                auto btn = squareButton(grid, g_res.getTexture(upgrade.iconGuid), upgrade.name.data(),
                        tmpStr("%i", price), size, upgradeLevel, upgrade.maxUpgradeLevel, 28.f);
                btn->addFlags(isPurchasable ? 0 : WidgetFlags::FADED);

                if (isPurchasable)
                {
                    btn->onGainedSelect([&upgrade, upgradeLevel, this, i]{
                        helpMessage = upgrade.description.data();

                        if (upgradeLevel < upgrade.maxUpgradeLevel)
                        {
                            garage.previewVehicleConfig = *garage.driver->getVehicleConfig();
                            garage.previewVehicleConfig.addUpgrade(i);
                            garage.previewVehicle->initTuning(garage.previewVehicleConfig, garage.previewTuning);
                            garage.upgradeStats = garage.previewTuning.computeVehicleStats();
                        }
                    });

                    btn->onPress([this, price, i, &upgrade]{
                        g_audio.playSound(g_res.getSound("airdrill"), SoundType::MENU_SFX);

                        garage.driver->credits -= price;
                        garage.driver->getVehicleConfig()->addUpgrade(i);
                        garage.previewTuning = garage.driver->getTuning();
                        garage.currentStats = garage.previewTuning.computeVehicleStats();
                        garage.upgradeStats = garage.currentStats;
                    });
                }
                else
                {
                    btn->onPress([]{ g_audio.playSound(g_res.getSound("nono"), SoundType::MENU_SFX); });
                }
            }

            button(column, "BACK", "", [&]{
                inputCapture->setEntering(false);
                selection = [this]{
                    garage.previewVehicleConfig = *garage.driver->getVehicleConfig();
                    garage.previewTuning = garage.driver->getTuning();
                    garage.currentStats = garage.previewTuning.computeVehicleStats();
                    garage.upgradeStats = garage.currentStats;

                    mode = GARAGE_UPGRADES;
                    gui::popInputCapture();
                };
            }, ButtonFlags::BACK);
        }
        else if (mode == GARAGE_COSMETICS)
        {
            auto inputCapture =
                (InputCapture*)topLevelRow->add(InputCapture("Garage Cosmetics"))->size(0,0);

            static Function<void()> selection = []{ assert(false); };
            menuContainer = animateMenu(inputCapture, inputCapture->isEntering(), Vec2(SIDE_MENU_WIDTH, 0))
                ->onAnimationReverseEnd([&]{ selection(); });

            auto column = menuContainer
                ->add(Container(Insets(0), {}, Vec4(0)))->size(0,0)
                ->add(Column(10))->size(INFINITY, 0);

            auto grid = column->add(Grid(3, 4))->size(SIDE_MENU_WIDTH, 0);
            Vec2 size(SIDE_MENU_WIDTH / 3);

            auto reloadColors = [this]{
                garage.previewVehicleConfig.cosmetics.computeColorFromHsv();
                garage.previewVehicleConfig.reloadMaterials();
                garage.driver->getVehicleConfig()->cosmetics.paintShininess
                    = garage.previewVehicleConfig.cosmetics.paintShininess;
                garage.driver->getVehicleConfig()->cosmetics.hsv
                    = garage.previewVehicleConfig.cosmetics.hsv;
                garage.driver->getVehicleConfig()->cosmetics.color
                    = garage.previewVehicleConfig.cosmetics.color;
                garage.driver->getVehicleConfig()->reloadMaterials();
            };

            animateButton(column, "PAINT HUE")->add(Slider("PAINT HUE", mediumFont,
                        &garage.previewVehicleConfig.cosmetics.hsv.x, 0.f, 1.f,
                        g_res.getTexture("hues"), Vec4(1), Vec4(1)))
                ->onChange(reloadColors)->size(INFINITY, 70);

            animateButton(column, "PAINT SATURATION")->add(Slider("PAINT SATURATION", mediumFont,
                        &garage.previewVehicleConfig.cosmetics.hsv.y, 0.f, 1.f,
                        &g_res.white,
                        Vec4(hsvToRgb(
                                garage.previewVehicleConfig.cosmetics.hsv.x, 0.f,
                                garage.previewVehicleConfig.cosmetics.hsv.z), 1.f),
                        Vec4(hsvToRgb(
                                garage.previewVehicleConfig.cosmetics.hsv.x, 1.f,
                                garage.previewVehicleConfig.cosmetics.hsv.z), 1.f)))
                ->onChange(reloadColors)->size(INFINITY, 70);

            Vec4 col = Vec4(srgb(hsvToRgb(
                                    garage.previewVehicleConfig.cosmetics.hsv.x,
                                    garage.previewVehicleConfig.cosmetics.hsv.y, 1.f)), 1.f);
            animateButton(column, "PAINT BRIGHTNESS")->add(Slider("PAINT BRIGHTNESS", mediumFont,
                        &garage.previewVehicleConfig.cosmetics.hsv.z, 0.f, 1.f,
                        g_res.getTexture("brightness_gradient"), col, col))
                ->onChange(reloadColors)->size(INFINITY, 70);

            animateButton(column, "PAINT SHININESS")->add(Slider("PAINT SHININESS", mediumFont,
                        &garage.previewVehicleConfig.cosmetics.paintShininess, 0.f, 1.f,
                        g_res.getTexture("slider"), Vec4(1)))
                ->onChange(reloadColors)->size(INFINITY, 70);

            Texture* iconbg = g_res.getTexture("iconbg");
            for (u32 i=0; i<3; ++i)
            {
                VinylPattern* vinylPattern = (VinylPattern*)g_res.getResource(
                        garage.driver->getVehicleConfig()->cosmetics.vinylGuids[i]);
                Texture* previewTexture = vinylPattern
                    ? g_res.getTexture(vinylPattern->colorTextureGuid) : iconbg;
                const char* name = tmpStr("VINYL LAYER %i", i+1);
                const char* subText = vinylPattern ? vinylPattern->name.data() : "None";

                animateButton(column, name)->add(TextButton(mediumFont, name, smallFont,
                    subText, 0, previewTexture))
                    ->onGainedSelect([]{ helpMessage = "Customize your vehicle with vinyls."; })
                    ->onPress([this, inputCapture, i]{
                        inputCapture->setEntering(false);
                        selection = [this, i]{ openVinylMenu(i); };
                    })
                    ->size(INFINITY, 70);
            }

            button(column, "BACK", "", [&]{
                inputCapture->setEntering(false);
                selection = [this]{
                    garage.previewVehicleConfig = *garage.driver->getVehicleConfig();
                    mode = GARAGE_UPGRADES;
                    gui::popInputCapture();
                };
            }, ButtonFlags::BACK);
        }
        else if (mode == GARAGE_COSMETICS_VINYL)
        {
            auto inputCapture =
                (InputCapture*)topLevelRow->add(InputCapture("Garage Vinyl"))->size(0,0);

            static Function<void()> selection = []{ assert(false); };
            menuContainer = animateMenu(inputCapture, inputCapture->isEntering(), Vec2(SIDE_MENU_WIDTH, 0))
                ->onAnimationReverseEnd([&]{ selection(); });

            auto column = menuContainer
                ->add(Container(Insets(0), {}, Vec4(0)))->size(0,0)
                ->add(Column(10))->size(INFINITY, 0);

            auto grid = column->add(Grid(3, 4))->size(SIDE_MENU_WIDTH, 0);
            Vec2 size(SIDE_MENU_WIDTH / 3);

            animateButton(column, "LAYER IMAGE")
                ->add(Select("LAYER IMAGE", mediumFont, smallFont, &vinylIndex,
                        (i32)vinyls.size(), vinyls[vinylIndex]->name.data()))
                ->onChange([this]{
                    garage.previewVehicleConfig.cosmetics.vinylGuids[currentVinylLayer]
                        = vinyls[vinylIndex]->guid;
                    garage.driver->getVehicleConfig()->cosmetics.vinylGuids[currentVinylLayer]
                        = garage.previewVehicleConfig.cosmetics.vinylGuids[currentVinylLayer];
                })
                ->size(INFINITY, 70);

            auto reloadColors = [this]{
                garage.previewVehicleConfig.cosmetics.computeColorFromHsv();
                garage.previewVehicleConfig.reloadMaterials();
                garage.driver->getVehicleConfig()->cosmetics.vinylColorsHSV[currentVinylLayer]
                    = garage.previewVehicleConfig.cosmetics.vinylColorsHSV[currentVinylLayer];
                garage.driver->getVehicleConfig()->cosmetics.vinylColors[currentVinylLayer]
                    = garage.previewVehicleConfig.cosmetics.vinylColors[currentVinylLayer];
                garage.driver->getVehicleConfig()->reloadMaterials();
            };

            animateButton(column, "LAYER HUE")->add(Slider("LAYER HUE", mediumFont,
                        &garage.previewVehicleConfig.cosmetics.vinylColorsHSV[currentVinylLayer].x,
                        0.f, 1.f, g_res.getTexture("hues"), Vec4(1), Vec4(1)))
                ->onChange(reloadColors)->size(INFINITY, 70);

            animateButton(column, "LAYER SATURATION")->add(Slider("LAYER SATURATION", mediumFont,
                        &garage.previewVehicleConfig.cosmetics.vinylColorsHSV[currentVinylLayer].y,
                        0.f, 1.f, &g_res.white,
                        Vec4(hsvToRgb(
                                garage.previewVehicleConfig.cosmetics.hsv.x, 0.f,
                                garage.previewVehicleConfig.cosmetics.hsv.z), 1.f),
                        Vec4(hsvToRgb(
                                garage.previewVehicleConfig.cosmetics.hsv.x, 1.f,
                                garage.previewVehicleConfig.cosmetics.hsv.z), 1.f)))
                ->onChange(reloadColors)->size(INFINITY, 70);

            Vec4 col = Vec4(srgb(hsvToRgb(
                garage.previewVehicleConfig.cosmetics.vinylColorsHSV[currentVinylLayer].x,
                garage.previewVehicleConfig.cosmetics.vinylColorsHSV[currentVinylLayer].y, 1.f)), 1.f);
            animateButton(column, "LAYER BRIGHTNESS")->add(Slider("LAYER BRIGHTNESS", mediumFont,
                        &garage.previewVehicleConfig.cosmetics.vinylColorsHSV[currentVinylLayer].z,
                        0.f, 1.f, g_res.getTexture("brightness_gradient"), col, col))
                ->onChange(reloadColors)->size(INFINITY, 70);

            animateButton(column, "LAYER OPACITY")->add(Slider("LAYER OPACITY", mediumFont,
                        &garage.previewVehicleConfig.cosmetics.vinylColors[currentVinylLayer].a,
                        0.f, 1.f, g_res.getTexture("brightness_gradient"), Vec4(1), Vec4(1)))
                ->onChange(reloadColors)->size(INFINITY, 70);

            button(column, "BACK", "", [&]{
                inputCapture->setEntering(false);
                selection = [this]{
                    mode = GARAGE_COSMETICS;
                    gui::popInputCapture();
                };
            }, ButtonFlags::BACK);
        }
        else if (mode == GARAGE_WEAPON)
        {
            auto inputCapture =
                (InputCapture*)topLevelRow->add(InputCapture("Garage Performance"))->size(0,0);

            static Function<void()> selection = []{ assert(false); };
            menuContainer = animateMenu(inputCapture, inputCapture->isEntering(), Vec2(SIDE_MENU_WIDTH, 0))
                ->onAnimationReverseEnd([&]{ selection(); });

            auto column = menuContainer
                ->add(Container(Insets(0), {}, Vec4(0)))->size(0,0)
                ->add(Column(10))->size(INFINITY, 0);

            auto grid = column->add(Grid(3, 4))->size(SIDE_MENU_WIDTH, 0);
            Vec2 size(SIDE_MENU_WIDTH / 3);

            i32 weaponIndex = garage.previewVehicleConfig.weaponIndices[currentWeaponSlotIndex];
            i32 upgradeLevel = garage.previewVehicleConfig.weaponUpgradeLevel[currentWeaponSlotIndex];
            WeaponSlot& slot = garage.driver->getVehicleData()->weaponSlots[currentWeaponSlotIndex];

            for (i32 i=0; i<(i32)g_weapons.size(); ++i)
            {
                auto& weapon = g_weapons[i];

                if (weapon.info.weaponType != slot.weaponType || !slot.matchesWeapon(weapon.info))
                {
                    continue;
                }

                u32 weaponUpgradeLevel = (weaponIndex == i) ? upgradeLevel: 0;
                bool isPurchasable = garage.driver->credits >= g_weapons[i].info.price
                    && weaponUpgradeLevel < g_weapons[i].info.maxUpgradeLevel
                    && (weaponIndex == -1 || weaponIndex == i);

                auto btn = squareButton(grid, weapon.info.icon, weapon.info.name,
                        tmpStr("%i", weapon.info.price), size, weaponUpgradeLevel,
                        (i32)weapon.info.maxUpgradeLevel, 28.f);
                btn->addFlags(isPurchasable ? 0 : WidgetFlags::FADED);
                btn->onGainedSelect([this, i]{
                    helpMessage = g_weapons[i].info.description;
                });

                btn->onPress([this, i, isPurchasable]{
                    if (isPurchasable)
                    {
                        // TODO: Play sound that is more appropriate for a weapon
                        g_audio.playSound(g_res.getSound("airdrill"), SoundType::MENU_SFX);
                        garage.previewVehicleConfig.weaponIndices[currentWeaponSlotIndex] = i;
                        garage.previewVehicleConfig.weaponUpgradeLevel[currentWeaponSlotIndex] += 1;

                        garage.driver->credits -= g_weapons[i].info.price;
                    }
                    else
                    {
                        g_audio.playSound(g_res.getSound("nono"), SoundType::MENU_SFX);
                    }
                });
            }

            button(column, "SELL", "Sell the currently equipped item for half the original value.",
                [this]{
                garage.driver->credits += g_weapons[currentWeaponSlotIndex].info.price / 2;
                garage.previewVehicleConfig.weaponUpgradeLevel[currentWeaponSlotIndex] -= 1;
                // TODO: use different sound for selling
                g_audio.playSound(g_res.getSound("airdrill"), SoundType::MENU_SFX);
                if (garage.previewVehicleConfig.weaponUpgradeLevel[currentWeaponSlotIndex] == 0)
                {
                    garage.previewVehicleConfig.weaponIndices[currentWeaponSlotIndex] = -1;
                }
            })->addFlags(upgradeLevel > 0 ? 0 : WidgetFlags::DISABLED);

            button(column, "BACK", "", [&]{
                inputCapture->setEntering(false);
                selection = [this]{
                    garage.driver->vehicleConfig = garage.previewVehicleConfig;
                    mode = GARAGE_UPGRADES;
                    gui::popInputCapture();
                };
            }, ButtonFlags::BACK);
        }
    }
    else if (mode == CHAMPIONSHIP_MENU)
    {
        auto inputCapture = gui::root->add(InputCapture("Championship Menu"));

        static Function<void()> selection = []{ assert(false); };
        auto animation = animateMenu(inputCapture, inputCapture->isEntering())
            ->onAnimationReverseEnd([&]{ selection(); });
        auto column = panel(animation, Vec2(600, 0))->add(Column(10))->size(0, 0);

        column
            ->add(Container(Insets(10), {}, Vec4(0), HAlign::CENTER))->size(INFINITY, 70)
            ->add(Text(bigFont, "CHAMPIONSHIP"));

        i32 playerIndex = 0;
        for (u32 driverIndex = 0; driverIndex < g_game.state.drivers.size(); ++driverIndex)
        {
            Driver& driver = g_game.state.drivers[driverIndex];
            if (driver.isPlayer)
            {
                garageButton(this, column, driverIndex, "Buy, sell, or upgrade your vehicle.", Vec2(0, 120))
                    ->onPress([inputCapture, this, playerIndex]{
                    inputCapture->setEntering(false);
                    selection = [this, playerIndex] {
                        garage.driver = &g_game.state.drivers[playerIndex];
                        garage.previewVehicle = garage.driver->getVehicleData();
                        garage.previewVehicleConfig = *garage.driver->getVehicleConfig();
                        garage.previewTuning = garage.driver->getTuning();
                        garage.currentStats = garage.previewTuning.computeVehicleStats();
                        garage.upgradeStats = garage.currentStats;
                        mode = GARAGE_UPGRADES;
                    };
                });
                ++playerIndex;
            }
        }

        column->add(Container())->size(Vec2(20));

        button(column, "BEGIN RACE", "Start the next race.", [&]{
            inputCapture->setEntering(false);
            fadeToBlack = true;
            selection = [&] {
                gui::clearInputCaptures();
                Scene* scene = g_game.changeScene(championshipTracks[g_game.state.currentRace]);
                scene->startRace();
                fadeToBlack = false;
            };
        });

        button(column, "STANDINGS", "View the current championship standings.", [&]{
            inputCapture->setEntering(false);
            selection = [&] {
                mode = STANDINGS;
            };
        });

        button(column, "QUIT", "Return to main menu.", [&]{
            isDialogOpen = true;
        });

        if (isDialogOpen)
        {
            static bool yes = false;

            auto dialogInputCapture = gui::root->add(ZIndex(200))->add(InputCapture("Dialog"));
            dialogInputCapture
                    ->add(FadeAnimation(0.f, 0.85f, 0.25f, dialogInputCapture->isEntering(),
                        "DimBackground"))
                    ->add(Container(Vec4(0, 0, 0, 1)));
            auto animation = animateMenu(dialogInputCapture, dialogInputCapture->isEntering(),
                    Vec2(INFINITY), 0.25f);
            animation->onAnimationReverseEnd([&]{
                popInputCapture();
                isDialogOpen = false;
                if (yes) {
                    inputCapture->setEntering(false);
                    selection = [this] {
                        mode = MAIN_MENU;
                    };
                }
            });

            auto column = animation
                ->add(Container(Insets(30), {}, Vec4(0, 0, 0, 1.f)))->size(750, 0)
                ->add(Column(10, HAlign::CENTER))->size(INFINITY, 0);

            column
                ->add(Container(Insets(10)))->size(0, 70)
                ->add(Text(mediumFont, "Are you sure you want to quit?"));

            auto buttonRow = column->add(Row(50))->size(0, 0);
            button(buttonRow, "NO", nullptr, [dialogInputCapture]{
                dialogInputCapture->setEntering(false);
                yes = false;
            }, ButtonFlags::BACK)->size(200, 70);
            button(buttonRow, "YES", nullptr, [dialogInputCapture]{
                dialogInputCapture->setEntering(false);
                yes = true;
            })->size(200, 70);
        }

        // track preview
        u32 trackPreviewSize = 380 * gui::guiScale;
        g_game.currentScene->updateTrackPreview(g_game.renderer.get(), trackPreviewSize);
        Texture* trackTex = g_game.currentScene->getTrackPreview2D().getTexture();
        // TODO: draw the track preview
    }
    else if (mode == RACE_RESULTS)
    {
        displayRaceResults();
    }
    else if (mode == STANDINGS)
    {
        displayStandings();
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
        ui::rectUV(100, tex, Vec2(0), {w,h}, Vec2(0.f, 0.999f),
                    Vec2(g_game.windowWidth/(tex->width * gui::guiScale * 0.5f), 0.02f));
        ui::rectUV(100, tex, Vec2(0, g_game.windowHeight-h), {w,h},
                    Vec2(0.f, 0.02f),
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
