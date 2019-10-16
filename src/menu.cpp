#include "menu.h"
#include "resources.h"
#include "renderer.h"
#include "game.h"
#include "2d.h"
#include "input.h"
#include "scene.h"
#include "gui.h"
#include "mesh_renderables.h"
#include "weapon.h"

void Menu::mainMenu()
{
    g_gui.beginPanel("Main Menu", { g_game.windowWidth/2, g_game.windowHeight*0.15f },
            0.5f, false, true);

    if (g_gui.button("Championship"))
    {
        menuMode = NEW_CHAMPIONSHIP;
        g_game.state = {};
    }

    if (g_gui.button("Load Game"))
    {
    }

    if (g_gui.button("Quick Play"))
    {
        g_game.state.drivers.clear();
        g_game.state.drivers.push_back(Driver(true,  true,  true,  0, 1));
        g_game.state.drivers.push_back(Driver(false, false, false, 0, 0));
        g_game.state.drivers.push_back(Driver(false, false, false, 0, 1));
        g_game.state.drivers.push_back(Driver(false, false, false, 0, 0));
        g_game.state.drivers.push_back(Driver(false, false, false, 0, 0));
        g_game.state.drivers.push_back(Driver(false, false, false, 0, 1));
        g_game.state.drivers.push_back(Driver(false, false, false, 0, 1));
        g_game.state.drivers.push_back(Driver(false, false, false, 0, 1));
        g_game.state.drivers.push_back(Driver(false, false, false, 0, 0));
        g_game.state.drivers.push_back(Driver(false, false, false, 0, 0));

        g_game.isEditing = false;
        Scene* scene = g_game.changeScene("tracks/saved_scene.dat");
        scene->startRace();
        menuMode = HIDDEN;
    }

    if (g_gui.button("Options"))
    {
        showOptionsMenu();
    }

    if (g_gui.button("Track Editor"))
    {
        g_game.isEditing = true;
        g_game.changeScene(nullptr);
        menuMode = HIDDEN;
    }

    if (g_gui.button("Quit"))
    {
        g_game.shouldExit = true;
    }

    g_gui.end();
}

void Menu::newChampionship()
{
    g_gui.beginPanel("Begin Championship ", { g_game.windowWidth/2, g_game.windowHeight*0.15f },
            0.5f, false, true);

    for (size_t i=0; i<g_game.state.drivers.size(); ++i)
    {
        g_gui.label(tstr("Player ", i + 1,
            g_game.state.drivers[i].useKeyboard ? " (Using Keyboard)" :
            tstr(" (Using Controller ", g_game.state.drivers[i].controllerID, ')')));
        g_gui.textEdit(tstr("Player ", i), g_game.state.drivers[i].playerName);
    }

    if (g_gui.button("Begin", g_game.state.drivers.size() > 0))
    {
        g_game.isEditing = false;
        g_game.changeScene("tracks/saved_scene.dat");
        //scene->startRace();
        menuMode = CHAMPIONSHIP_MENU;
    }

    if (g_gui.button("Back"))
    {
        menuMode = MAIN_MENU;
    }

    g_gui.label("Press keyboard or controller to join");

    if (g_input.isKeyPressed(KEY_SPACE) || g_input.isKeyPressed(KEY_RETURN))
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
            // TODO: ensure all player names are unique
            g_game.state.drivers.push_back(Driver(true, true, true, 0));
        }
    }

    for (auto& controller : g_input.getControllers())
    {
        if (controller.second.isAnyButtonPressed())
        {
            bool controllerPlayerExists = false;
            for (auto& driver : g_game.state.drivers)
            {
                if (!driver.useKeyboard && driver.controllerID == controller.first)
                {
                    controllerPlayerExists = true;
                    break;
                }
            }

            if (!controllerPlayerExists)
            {
                g_game.state.drivers.push_back(Driver(true, true, false, controller.first));
            }
        }
    }


    g_gui.end();
}

void Menu::championshipMenu()
{
    f32 w = glm::floor(g_gui.convertSize(670));

    glm::vec2 menuPos = glm::vec2(g_game.windowWidth/2 - w/2, g_game.windowHeight * 0.1f);
    glm::vec2 menuSize = glm::vec2(w, (f32)g_game.windowHeight * 0.8f);
    drawBox(menuPos, menuSize);

    static RenderWorld renderWorlds[MAX_VIEWPORTS];

    f32 o = glm::floor(g_gui.convertSize(32));

    g_gui.beginPanel("Championship Menu",
            { menuPos.x + w - o, menuPos.y + o },
            1.f, false, true, false);

    i32 vehicleCount = 0;
    i32 playerIndex = 0;
    u32 vehicleIconSize = (u32)g_gui.convertSize(64);
    Mesh* quadMesh = g_resources.getMesh("world.Quad");
    for (auto& driver : g_game.state.drivers)
    {
        if (driver.isPlayer)
        {
            renderWorlds[playerIndex].setName(tstr("Vehicle Icon ", playerIndex));
            renderWorlds[playerIndex].setSize(vehicleIconSize, vehicleIconSize);
            renderWorlds[playerIndex].push(LitRenderable(quadMesh,
                        glm::scale(glm::mat4(1.f), glm::vec3(20.f)), nullptr, glm::vec3(0.02f)));
            if (driver.vehicleIndex != -1)
            {
                g_vehicles[driver.vehicleIndex]->render(&renderWorlds[playerIndex],
                    glm::translate(glm::mat4(1.f),
                        glm::vec3(0, 0, driver.getTuning().getRestOffset())) *
                    glm::rotate(glm::mat4(1.f), (f32)getTime(), glm::vec3(0, 0, 1)),
                    nullptr, *driver.getVehicleConfig());
                ++vehicleCount;
            }
            renderWorlds[playerIndex].setViewportCount(1);
            renderWorlds[playerIndex].addDirectionalLight(glm::vec3(-0.5f, 0.2f, -1.f), glm::vec3(1.0));
            renderWorlds[playerIndex].setViewportCamera(0, glm::vec3(8.f, -8.f, 10.f),
                    glm::vec3(0.f, 0.f, 1.f), 1.f, 50.f, 30.f);
            g_game.renderer->addRenderWorld(&renderWorlds[playerIndex]);

            if (g_gui.vehicleButton(tstr(driver.playerName, "'s Garage"),
                    renderWorlds[playerIndex].getTexture(), &driver))
            {
                menuMode = MenuMode::CHAMPIONSHIP_GARAGE;
                g_game.state.driverContextIndex = playerIndex;
            }

            ++playerIndex;
        }
    }

    g_gui.gap(10);
    if (g_gui.button("Begin Race", playerIndex == vehicleCount))
    {
        Scene* scene = g_game.changeScene("tracks/saved_scene.dat");
        scene->startRace();
        menuMode = HIDDEN;
    }
    g_gui.button("Standings");
    g_gui.button("Player Stats");
    g_gui.gap(10);

    if (g_gui.button("Quit"))
    {
        menuMode = MenuMode::MAIN_MENU;
    }

    g_gui.end();

    Font* bigfont = &g_resources.getFont("font", (u32)g_gui.convertSize(64));
    g_game.renderer->push2D(TextRenderable(bigfont, tstr("League ", (char)('A' + g_game.state.currentLeague)),
                menuPos + o, glm::vec3(1.f)));
    Font* mediumFont = &g_resources.getFont("font", (u32)g_gui.convertSize(32));
    g_game.renderer->push2D(TextRenderable(mediumFont, tstr("Race ", g_game.state.currentRace + 1),
                menuPos + o + glm::vec2(0, glm::floor(g_gui.convertSize(52))), glm::vec3(1.f)));

    u32 trackPreviewSize = (u32)g_gui.convertSize(320);
    g_game.currentScene->drawTrackPreview(g_game.renderer.get(), trackPreviewSize,
            menuPos + glm::vec2(glm::floor(trackPreviewSize/2) + o, menuSize.y / 2));
    g_game.renderer->add2D(&g_game.currentScene->getTrackPreview2D());
}

void Menu::championshipGarage()
{
    f32 w = glm::floor(g_gui.convertSize(600));

    glm::vec2 menuPos = glm::vec2(g_game.windowWidth/2 - w/2, g_game.windowHeight * 0.1f);
    glm::vec2 menuSize = glm::vec2(w, (f32)g_game.windowHeight * 0.8f);
    drawBox(menuPos, menuSize);

    f32 o = glm::floor(g_gui.convertSize(32));
    f32 oy = glm::floor(g_gui.convertSize(48));

    Texture* white = g_resources.getTexture("white");
    u32 vehicleIconWidth = (u32)g_gui.convertSize(290);
    u32 vehicleIconHeight = (u32)g_gui.convertSize(256);
    Font* smallfont = &g_resources.getFont("font", (u32)g_gui.convertSize(18));
    glm::vec2 vehiclePreviewPos = menuPos + glm::vec2(o, oy);

    Driver& driver = g_game.state.drivers[g_game.state.driverContextIndex];

    static u32 mode = 0;
    static i32 currentVehicleIndex = driver.vehicleIndex;

    const char* messageStr = nullptr;

    if (driver.vehicleIndex == -1)
    {
        mode = 1;
    }
    VehicleConfiguration vehicleConfig = driver.vehicleIndex == -1
        ? VehicleConfiguration{} : *driver.getVehicleConfig();

    glm::vec2 panelPos = menuPos + glm::vec2(w - o, oy);
    g_gui.beginPanel(tstr("Garage ", mode), panelPos, 1.f, false, true, false);

    if (mode == 0)
    {
        if (g_gui.button("Choose Vehicle"))
        {
            mode = 1;
        }
        g_gui.label("Vehicle Upgrades");
        if (g_gui.button("Performance", driver.vehicleIndex != -1, "iconbg"))
        {
            mode = 2;
        }
        if (g_gui.button("Cosmetics", driver.vehicleIndex != -1, "iconbg"))
        {
            mode = 3;
        }
        g_gui.label("Equipment");
        for (u32 i=0; i<3; ++i)
        {
            i32 weaponIndex = vehicleConfig.frontWeaponIndices[i];
            if (g_gui.button(tstr("Front Weapon ", i + 1), driver.vehicleIndex != -1,
                        weaponIndex == -1 ? "iconbg" : g_weapons[weaponIndex]->icon))
            {
                mode = i + 4;
            }
        }
        for (u32 i=0; i<2; ++i)
        {
            i32 weaponIndex = vehicleConfig.rearWeaponIndices[i];
            if (g_gui.button(tstr("Rear Weapon ", i + 1), driver.vehicleIndex != -1,
                        weaponIndex == -1 ? "iconbg" : g_weapons[weaponIndex]->icon))
            {
                mode = i + 7;
            }
        }
        if (g_gui.button("Special Ability", driver.vehicleIndex != -1, "iconbg"))
        {
            mode = 9;
        }
    }
    else if (mode == 1)
    {
        std::vector<std::string> carNames;
        for (auto& v : g_vehicles)
        {
            carNames.push_back(v->name);
        }

        if (currentVehicleIndex == -1)
        {
            currentVehicleIndex = 0;
        }

        auto ownedVehicle = std::find_if(driver.ownedVehicles.begin(),
                        driver.ownedVehicles.end(),
                        [&](auto& e) { return e.vehicleIndex == currentVehicleIndex; });
        bool isOwned = ownedVehicle != driver.ownedVehicles.end();
        VehicleData* vehicleData = g_vehicles[currentVehicleIndex].get();

        if (isOwned)
        {
            driver.vehicleIndex = currentVehicleIndex;
        }
        else
        {
            driver.vehicleIndex = -1;
            vehicleConfig.colorIndex = driver.lastColorIndex;
        }

        messageStr = vehicleData->description;

        if (g_gui.select("Vehicle", carNames.data(), carNames.size(), currentVehicleIndex))
        {
        }

        g_gui.gap(20);
        if (g_gui.button("Purchase", !isOwned && driver.credits >= vehicleData->price))
        {
            driver.ownedVehicles.push_back({
                currentVehicleIndex,
                vehicleConfig
            });
            driver.credits -= vehicleData->price;
        }
        if (g_gui.button("Sell", isOwned))
        {
            driver.ownedVehicles.erase(ownedVehicle);
            driver.credits += vehicleData->price / 2;
            // TODO: make the vehicle worth more when it has upgrades
        }
        g_gui.gap(20);

        if (g_gui.button("Done", isOwned))
        {
            mode = 0;
        }

        g_game.renderer->push2D(QuadRenderable(white,
                    vehiclePreviewPos + glm::vec2(0, vehicleIconHeight - g_gui.convertSize(26)),
                    g_gui.convertSize(120), g_gui.convertSize(26),
                    glm::vec3(0.f), 0.25f, false), 1);
        g_game.renderer->push2D(TextRenderable(smallfont,
                    tstr(isOwned ? "Value: " : "Price: ", isOwned ? vehicleData->price/2 : vehicleData->price),
                    vehiclePreviewPos + glm::vec2(g_gui.convertSize(8), vehicleIconHeight - g_gui.convertSize(18)),
                    glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::LEFT), 2);
    }
    else if (mode >= 4 && mode <= 6)
    {
        u32 weaponNumber = mode - 4;
        g_gui.label(tstr("Front Weapon ", weaponNumber + 1));
        i32 equippedWeaponIndex = vehicleConfig.frontWeaponIndices[weaponNumber];
        for (i32 i = 0; i<(i32)g_weapons.size(); ++i)
        {
            Weapon* weapon = g_weapons[i].get();
            bool isSelected = false;

            const char* extraText = nullptr;
            u32 upgradeLevel = vehicleConfig.frontWeaponUpgradeLevel[weaponNumber];
            bool isEquipped = equippedWeaponIndex != -1 && equippedWeaponIndex == i;
            if (isEquipped)
            {
                extraText =
                    tstr(upgradeLevel, "/", weapon->maxUpgradeLevel);
            }
            if (g_gui.itemButton(weapon->name, tstr("Price: ", weapon->price), extraText,
                        driver.credits >= weapon->price &&
                            ((upgradeLevel < weapon->maxUpgradeLevel && isEquipped) || !isEquipped),
                        weapon->icon, &isSelected))
            {
                driver.credits -= weapon->price;
                if (driver.getVehicleConfig()->frontWeaponIndices[weaponNumber] != i)
                {
                    driver.getVehicleConfig()->frontWeaponIndices[weaponNumber] = i;
                    driver.getVehicleConfig()->frontWeaponUpgradeLevel[weaponNumber] = 1;
                }
                else
                {
                    ++driver.getVehicleConfig()->frontWeaponUpgradeLevel[weaponNumber];
                }
                // TODO: Play upgrade sound
            }
            if (isSelected)
            {
                messageStr = weapon->description;
            }
        }
    }
    else if (mode >= 7 && mode <= 9)
    {
        u32 weaponNumber = mode - 7;
        g_gui.label(tstr("Rear Weapon ", weaponNumber + 1));
        i32 equippedWeaponIndex = vehicleConfig.rearWeaponIndices[weaponNumber];
        for (i32 i = 0; i<(i32)g_weapons.size(); ++i)
        {
            Weapon* weapon = g_weapons[i].get();
            bool isSelected = false;

            const char* extraText = nullptr;
            u32 upgradeLevel = vehicleConfig.rearWeaponUpgradeLevel[weaponNumber];
            bool isEquipped = equippedWeaponIndex != -1 && equippedWeaponIndex == i;
            if (isEquipped)
            {
                extraText =
                    tstr(upgradeLevel, "/", weapon->maxUpgradeLevel);
            }
            if (g_gui.itemButton(weapon->name, tstr("Price: ", weapon->price), extraText,
                        driver.credits >= weapon->price &&
                            ((upgradeLevel < weapon->maxUpgradeLevel && isEquipped) || !isEquipped),
                        weapon->icon, &isSelected))
            {
                driver.credits -= weapon->price;
                if (driver.getVehicleConfig()->rearWeaponIndices[weaponNumber] != i)
                {
                    driver.getVehicleConfig()->rearWeaponIndices[weaponNumber] = i;
                    driver.getVehicleConfig()->rearWeaponUpgradeLevel[weaponNumber] = 1;
                }
                else
                {
                    ++driver.getVehicleConfig()->rearWeaponUpgradeLevel[weaponNumber];
                }
                // TODO: Play upgrade sound
            }
            if (isSelected)
            {
                messageStr = weapon->description;
            }
        }
    }

    g_gui.gap(20);
    if (mode != 1)
    {
        if (g_gui.button("Done"))
        {
            if (mode > 0)
            {
                mode = 0;
            }
            else
            {
                menuMode = MenuMode::CHAMPIONSHIP_MENU;
            }
        }
    }

    g_gui.end();

    static RenderWorld renderWorld;
    Mesh* quadMesh = g_resources.getMesh("world.Quad");
    renderWorld.setName("Garage");
    renderWorld.setSize(vehicleIconWidth, vehicleIconHeight);
    renderWorld.push(LitRenderable(quadMesh,
            glm::scale(glm::mat4(1.f), glm::vec3(20.f)), nullptr, glm::vec3(0.02f)));
    VehicleTuning tuning;
    g_vehicles[currentVehicleIndex]->initTuning(vehicleConfig, tuning);
    g_vehicles[currentVehicleIndex]->render(&renderWorld,
        glm::translate(glm::mat4(1.f),
            glm::vec3(0, 0, tuning.getRestOffset())) *
        glm::rotate(glm::mat4(1.f), (f32)getTime(), glm::vec3(0, 0, 1)),
        nullptr, vehicleConfig);
    renderWorld.setViewportCount(1);
    renderWorld.addDirectionalLight(glm::vec3(-0.5f, 0.2f, -1.f), glm::vec3(1.0));
    renderWorld.setViewportCamera(0, glm::vec3(8.f, -8.f, 10.f),
            glm::vec3(0.f, 0.f, 1.f), 1.f, 50.f, 30.f);
    g_game.renderer->addRenderWorld(&renderWorld);

    g_game.renderer->push2D(QuadRenderable(renderWorld.getTexture(),
                vehiclePreviewPos, vehicleIconWidth, vehicleIconHeight,
                glm::vec3(1.f), 1.f, false,
                true, "texArray2D"));

    g_game.renderer->push2D(QuadRenderable(white,
                vehiclePreviewPos + glm::vec2(0, vehicleIconHeight),
                vehicleIconWidth, g_gui.convertSize(26),
                glm::vec3(0.f), 0.9f, true));

    g_game.renderer->push2D(TextRenderable(smallfont,
                currentVehicleIndex != -1 ? g_vehicles[currentVehicleIndex]->name : "No Vehicle",
                vehiclePreviewPos + glm::vec2((u32)vehicleIconWidth / 2,
                    glm::floor(vehicleIconHeight + g_gui.convertSize(8))),
                glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::CENTER));

    g_game.renderer->push2D(QuadRenderable(white,
                menuPos, menuSize.x, g_gui.convertSize(26),
                glm::vec3(0.f), 0.9f, true));
    g_game.renderer->push2D(TextRenderable(smallfont,
                tstr(driver.playerName, "'s Garage       Credits: ", driver.credits),
                menuPos + glm::vec2(o + g_gui.convertSize(8), glm::floor(g_gui.convertSize(8))),
                glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::LEFT));

    struct Stat
    {
        const char* name;
        f32 value = 0.f;
    };
    static Stat stats[] = {
        { "Acceleration" },
        { "Top Speed" },
        { "Armor" },
        { "Mass" },
        { "Handling" },
    };
    f32 targetStats[] = {
        tuning.specs.acceleration,
        tuning.topSpeed / 100.f,
        tuning.maxHitPoints / 300.f,
        tuning.chassisDensity / 350.f, // TODO: calculate the actual mass
        tuning.specs.handling,
    };

    const f32 maxBarWidth = vehicleIconWidth;
    glm::vec2 statsPos = vehiclePreviewPos
        + glm::vec2(0, vehicleIconHeight + glm::floor(g_gui.convertSize(48)));
    f32 barHeight = glm::floor(g_gui.convertSize(6));
    Font* tinyfont = &g_resources.getFont("font", (u32)g_gui.convertSize(14));
    for (u32 i=0; i<ARRAY_SIZE(stats); ++i)
    {
        stats[i].value = smoothMove(stats[i].value, targetStats[i], 8.f, g_game.deltaTime);

        g_game.renderer->push2D(TextRenderable(tinyfont, stats[i].name,
                    statsPos + glm::vec2(0, glm::floor(g_gui.convertSize(i * 30))),
                    glm::vec3(1.f)));

        g_game.renderer->push2D(QuadRenderable(white,
                    statsPos + glm::vec2(0, glm::floor(g_gui.convertSize(i * 30 + 12))),
                    maxBarWidth, barHeight, glm::vec3(0.f), 0.9f, true));

        f32 barWidth = maxBarWidth * stats[i].value;
        g_game.renderer->push2D(QuadRenderable(white,
                    statsPos + glm::vec2(0, glm::floor(g_gui.convertSize(i * 30 + 12))),
                    barWidth, barHeight, glm::vec3(0.8f)));
    }

    if (messageStr)
    {
        g_game.renderer->push2D(TextRenderable(smallfont, messageStr,
                    statsPos + glm::vec2(0, glm::floor(g_gui.convertSize(ARRAY_SIZE(stats) * 30 + 12))),
                    glm::vec3(1.f)));
    }
}

void Menu::showOptionsMenu()
{
    menuMode = MenuMode::OPTIONS_MAIN;
    tmpConfig = g_game.config;
}

void Menu::mainOptions()
{
    g_gui.beginPanel("Options", { g_game.windowWidth/2, g_game.windowHeight*0.15f },
            0.5f, false, true);

    if (g_gui.button("Graphics Options"))
    {
        menuMode = OPTIONS_GRAPHICS;
    }

    if (g_gui.button("Audio Options"))
    {
        menuMode = OPTIONS_AUDIO;
    }

    if (g_gui.button("Gameplay Options"))
    {
        menuMode = OPTIONS_GAMEPLAY;
    }

    if (g_gui.button("Back"))
    {
        showMainMenu();
    }

    g_gui.end();
}

void Menu::audioOptions()
{
    g_gui.beginPanel("Audio Options", { g_game.windowWidth/2, g_game.windowHeight*0.15f },
            0.5f, false, true);

    g_gui.slider("Master Volume", 0.f, 1.f, tmpConfig.audio.masterVolume);
    g_gui.slider("Vehicle Volume", 0.f, 1.f, tmpConfig.audio.vehicleVolume);
    g_gui.slider("SFX Volume", 0.f, 1.f, tmpConfig.audio.sfxVolume);
    g_gui.slider("Music Volume", 0.f, 1.f, tmpConfig.audio.musicVolume);

    if (g_gui.button("Save"))
    {
        g_game.config.audio = tmpConfig.audio;
        showMainMenu();
    }

    if (g_gui.button("Reset to Defaults"))
    {
        Config defaultConfig;
        tmpConfig.audio = defaultConfig.audio;
    }

    if (g_gui.button("Cancel"))
    {
        showOptionsMenu();
    }

    g_gui.end();
}

void Menu::gameplayOptions()
{
    g_gui.beginPanel("Gameplay Options", { g_game.windowWidth/2, g_game.windowHeight*0.15f },
            0.5f, false, true);

    g_gui.slider("HUD Track Scale", 0.5f, 2.f, tmpConfig.gameplay.hudTrackScale);
    g_gui.slider("HUD Text Scale", 0.5f, 2.f, tmpConfig.gameplay.hudTextScale);

    if (g_gui.button("Save"))
    {
        g_game.config.gameplay = tmpConfig.gameplay;
        showMainMenu();
    }

    if (g_gui.button("Reset to Defaults"))
    {
        Config defaultConfig;
        tmpConfig.gameplay = defaultConfig.gameplay;
    }

    if (g_gui.button("Cancel"))
    {
        showOptionsMenu();
    }

    g_gui.end();
}

void Menu::graphicsOptions()
{
    g_gui.beginPanel("Graphics Options", { g_game.windowWidth/2, g_game.windowHeight*0.15f },
            0.5f, false, true);

    i32 resolutionIndex = 0;
    glm::ivec2 resolutions[] = {
        { 960, 540 },
        { 1024, 576 },
        { 1280, 720 },
        { 1366, 768 },
        { 1600, 900 },
        { 1920, 1080 },
        { 2560, 1440 },
        { 3840, 2160 },
    };
    SmallVec<std::string> resolutionNames;
    for (i32 i=0; i<(i32)ARRAY_SIZE(resolutions); ++i)
    {
        if (resolutions[i].x == (i32)tmpConfig.graphics.resolutionX &&
            resolutions[i].y == (i32)tmpConfig.graphics.resolutionY)
        {
            resolutionIndex = i;
        }
        resolutionNames.push_back(str(resolutions[i].x, " x ", resolutions[i].y));
    }
    g_gui.select("Resolution", &resolutionNames[0],
                ARRAY_SIZE(resolutions), resolutionIndex);
    tmpConfig.graphics.resolutionX = resolutions[resolutionIndex].x;
    tmpConfig.graphics.resolutionY = resolutions[resolutionIndex].y;

    g_gui.toggle("Fullscreen", tmpConfig.graphics.fullscreen);
    g_gui.toggle("V-Sync", tmpConfig.graphics.vsync);
    /*
    f32 maxFPS = tmpConfig.graphics.maxFPS;
    g_gui.slider("Max FPS", 30, 300, maxFPS);
    tmpConfig.graphics.maxFPS = (u32)maxFPS;
    */

    u32 shadowMapResolutions[] = { 0, 1024, 2048, 4096 };
    std::string shadowQualityNames[] = { "Off", "Low", "Medium", "High" };
    i32 shadowQualityIndex = 0;
    for (i32 i=0; i<(i32)ARRAY_SIZE(shadowMapResolutions); ++i)
    {
        if (shadowMapResolutions[i] == tmpConfig.graphics.shadowMapResolution)
        {
            shadowQualityIndex = i;
            break;
        }
    }
    g_gui.select("Shadow Quality", shadowQualityNames, ARRAY_SIZE(shadowQualityNames),
            shadowQualityIndex);
    tmpConfig.graphics.shadowsEnabled = shadowQualityIndex > 0;
    tmpConfig.graphics.shadowMapResolution = shadowMapResolutions[shadowQualityIndex];

    std::string ssaoQualityNames[] = { "Off", "Normal", "High" };
    i32 ssaoQualityIndex = 0;
    if (tmpConfig.graphics.ssaoEnabled)
    {
        ssaoQualityIndex = 1;
        if (tmpConfig.graphics.ssaoHighQuality)
        {
            ssaoQualityIndex = 2;
        }
    }
    g_gui.select("SSAO Quality", ssaoQualityNames, ARRAY_SIZE(ssaoQualityNames),
            ssaoQualityIndex);
    tmpConfig.graphics.ssaoEnabled = ssaoQualityIndex > 0;
    tmpConfig.graphics.ssaoHighQuality = ssaoQualityIndex == 2;

    g_gui.toggle("Bloom", tmpConfig.graphics.bloomEnabled);

    i32 aaIndex = 0;
    u32 aaLevels[] = { 0, 2, 4, 8 };
    std::string aaLevelNames[] = { "Off", "2x MSAA", "4x MSAA", "8x MSAA" };
    for (i32 i=0; i<(i32)ARRAY_SIZE(aaLevels); ++i)
    {
        if (aaLevels[i] == tmpConfig.graphics.msaaLevel)
        {
            aaIndex = i;
            break;
        }
    }
    g_gui.select("Anti-Aliasing", aaLevelNames, ARRAY_SIZE(aaLevelNames), aaIndex);
    tmpConfig.graphics.msaaLevel = aaLevels[aaIndex];

    if (g_gui.button("Save"))
    {
        g_game.config = tmpConfig;
        g_game.renderer->updateSettingsVersion();
        SDL_SetWindowFullscreen(g_game.window, g_game.config.graphics.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        SDL_GL_SetSwapInterval(g_game.config.graphics.vsync ? 1 : 0);
        showMainMenu();
    }

    if (g_gui.button("Reset to Defaults"))
    {
        Config defaultConfig;
        tmpConfig.graphics = defaultConfig.graphics;
    }

    if (g_gui.button("Cancel"))
    {
        showOptionsMenu();
    }

    g_gui.end();
}

void Menu::onUpdate(Renderer* renderer, f32 deltaTime)
{
    if (menuMode != MenuMode::HIDDEN &&
        menuMode != MenuMode::CHAMPIONSHIP_MENU &&
        menuMode != MenuMode::CHAMPIONSHIP_GARAGE)
    {
        f32 w = g_gui.convertSize(280);
        drawBox(glm::vec2(g_game.windowWidth/2 - w/2, g_game.windowHeight * 0.1f),
                glm::vec2(w, (f32)g_game.windowHeight * 0.8f));
    }

    if (menuMode != MenuMode::HIDDEN)
    {
        Texture* tex = g_resources.getTexture("checkers_fade");
        f32 w = g_game.windowWidth;
        f32 h = g_gui.convertSize(tex->height * 0.5f);
        renderer->push2D(QuadRenderable(tex, glm::vec2(0), w, h, glm::vec2(0.f, 0.999f),
                    glm::vec2(g_game.windowWidth/g_gui.convertSize(tex->width * 0.5f), 0.001f)));
        renderer->push2D(QuadRenderable(tex, glm::vec2(0, g_game.windowHeight-h), w, h,
                    glm::vec2(0.f, 0.001f),
                    glm::vec2(g_game.windowWidth/g_gui.convertSize(tex->width * 0.5f), 0.999f)));
    }

    switch (menuMode)
    {
        case MenuMode::HIDDEN:
            break;
        case MenuMode::MAIN_MENU:
            mainMenu();
            break;
        case MenuMode::NEW_CHAMPIONSHIP:
            newChampionship();
            break;
        case MenuMode::CHAMPIONSHIP_MENU:
            championshipMenu();
            break;
        case MenuMode::CHAMPIONSHIP_GARAGE:
            championshipGarage();
            break;
        case MenuMode::OPTIONS_MAIN:
            mainOptions();
            break;
        case MenuMode::OPTIONS_GRAPHICS:
            graphicsOptions();
            break;
        case MenuMode::OPTIONS_AUDIO:
            audioOptions();
            break;
        case MenuMode::OPTIONS_GAMEPLAY:
            gameplayOptions();
            break;
    }
}

void Menu::drawBox(glm::vec2 pos, glm::vec2 size)
{
    f32 border = glm::floor(g_gui.convertSize(3));
    g_game.renderer->push2D(QuadRenderable(g_resources.getTexture("white"),
                pos - border, size.x + border * 2, size.y + border * 2,
                glm::vec3(1.f), 0.4f, false));
    g_game.renderer->push2D(QuadRenderable(g_resources.getTexture("white"),
                pos, size.x, size.y, glm::vec3(0.f), 0.8f, true));
}
