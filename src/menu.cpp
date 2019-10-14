#include "menu.h"
#include "resources.h"
#include "renderer.h"
#include "game.h"
#include "2d.h"
#include "input.h"
#include "scene.h"
#include "gui.h"
#include "mesh_renderables.h"

void Menu::mainMenu()
{
    g_gui.beginPanel("Main Menu", { g_game.windowWidth/2, g_game.windowHeight*0.1f },
            0.5f, false, true);

    if (g_gui.button("Championship"))
    {
        menuMode = NEW_CHAMPIONSHIP;
        g_game.state.drivers.clear();
        g_game.state.currentLeague = 0;
    }

    if (g_gui.button("Load Game"))
    {
    }

    if (g_gui.button("Quick Play"))
    {
        g_game.state.drivers.clear();
        g_game.state.drivers.push_back(Driver(true,  true,  true,  1, 1, 0));
        g_game.state.drivers.push_back(Driver(false, false, false, 0, 2, 0));
        g_game.state.drivers.push_back(Driver(false, false, false, 1, 3));
        g_game.state.drivers.push_back(Driver(false, false, false, 0, 4));
        g_game.state.drivers.push_back(Driver(false, false, false, 0, 5));
        g_game.state.drivers.push_back(Driver(false, false, false, 1, 6));
        g_game.state.drivers.push_back(Driver(false, false, false, 1, 7));
        g_game.state.drivers.push_back(Driver(false, false, false, 1, 8));
        g_game.state.drivers.push_back(Driver(false, false, false, 0, 1));
        g_game.state.drivers.push_back(Driver(false, false, false, 0, 2));

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
    g_gui.beginPanel("Begin Championship ", { g_game.windowWidth/2, g_game.windowHeight*0.1f },
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
            g_game.state.drivers.push_back(Driver(true, true, true, 1, 0, 0));
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
                g_game.state.drivers.push_back(Driver(true, true, false,
                            0, 0, controller.first));
            }
        }
    }


    g_gui.end();
}

void Menu::championshipMenu()
{
    f32 w = g_gui.convertSize(280);
    g_game.renderer->push2D(QuadRenderable(g_resources.getTexture("white"),
                glm::vec2(0, 0),
                w, (f32)g_game.windowHeight,
                glm::vec3(0.f), 0.8f, true));

    static RenderWorld renderWorlds[MAX_VIEWPORTS];

    g_gui.beginPanel("Championship Menu",
            { (w - g_gui.convertSize(220)) / 2, g_game.windowHeight*0.1f },
            0.f, false, true, false);

    bool allPlayersHaveVehicle = true;
    u32 playerIndex = 0;
    u32 vehicleIconSize = (u32)g_gui.convertSize(64);
    Mesh* quadMesh = g_resources.getMesh("world.Quad");
    for (auto& driver : g_game.state.drivers)
    {
        if (driver.isPlayer)
        {
            renderWorlds[playerIndex].setSize(vehicleIconSize, vehicleIconSize);
            renderWorlds[playerIndex].push(LitRenderable(quadMesh,
                        glm::scale(glm::mat4(1.f), glm::vec3(20.f)), nullptr, glm::vec3(0.02f)));
            driver.updateTuning();
            g_vehicles[driver.vehicleIndex]->render(&renderWorlds[playerIndex],
                glm::translate(glm::mat4(1.f),
                    glm::vec3(0, 0, driver.vehicleTuning.getRestOffset())) *
                glm::rotate(glm::mat4(1.f), (f32)getTime(), glm::vec3(0, 0, 1)),
                nullptr, &driver);
            renderWorlds[playerIndex].setViewportCount(1);
            renderWorlds[playerIndex].addDirectionalLight(glm::vec3(-0.5f, 0.2f, -1.f), glm::vec3(1.0));
            renderWorlds[playerIndex].setViewportCamera(0, glm::vec3(8.f, -8.f, 10.f),
                    glm::vec3(0.f, 0.f, 1.f), 1.f, 50.f, 30.f);
            renderWorlds[playerIndex].render(g_game.renderer.get(), g_game.deltaTime);
            g_game.renderer->addRenderWorld(&renderWorlds[playerIndex]);

            if (g_gui.vehicleButton(tstr(driver.playerName, "'s Garage"),
                    renderWorlds[playerIndex].getTexture()))
            {
                menuMode = MenuMode::CHAMPIONSHIP_GARAGE;
            }
            ++playerIndex;
        }
    }

    g_gui.label("");

    g_gui.button("View Standings");
    g_gui.button("Begin Race", allPlayersHaveVehicle);

    g_gui.label("");
    if (g_gui.button("Quit"))
    {
        menuMode = MenuMode::MAIN_MENU;
    }

    g_gui.end();

    Font* bigfont = &g_resources.getFont("font", (u32)g_gui.convertSize(64));
    g_game.renderer->push2D(TextRenderable(bigfont, tstr("League ", (char)('A' + g_game.state.currentLeague)),
                glm::vec2(glm::floor(w + g_gui.convertSize(32)),
                    glm::floor(g_gui.convertSize(32))), glm::vec3(1.f)));
}

void Menu::championshipGarage()
{
}

void Menu::showOptionsMenu()
{
    menuMode = MenuMode::OPTIONS_MAIN;
    tmpConfig = g_game.config;
}

void Menu::mainOptions()
{
    g_gui.beginPanel("Options", { g_game.windowWidth/2, g_game.windowHeight*0.1f },
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
    g_gui.beginPanel("Audio Options", { g_game.windowWidth/2, g_game.windowHeight*0.1f },
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
    g_gui.beginPanel("Gameplay Options", { g_game.windowWidth/2, g_game.windowHeight*0.1f },
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
    g_gui.beginPanel("Graphics Options", { g_game.windowWidth/2, g_game.windowHeight*0.1f },
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
        renderer->push2D(QuadRenderable(g_resources.getTexture("white"),
                    glm::vec2(g_game.windowWidth/2 - w/2, 0),
                    w, (f32)g_game.windowHeight,
                    glm::vec3(0.f), 0.8f, true));
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
