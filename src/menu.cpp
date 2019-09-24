#include "menu.h"
#include "resources.h"
#include "renderer.h"
#include "game.h"
#include "2d.h"
#include "input.h"
#include "scene.h"
#include "gui.h"

void Menu::mainMenu()
{
    g_gui.beginPanel("Main Menu", { g_game.windowWidth/2, g_game.windowHeight*0.3f }, 300,
            0.5f, true, 6);

    if (g_gui.button("Championship"))
    {
        g_game.isEditing = false;
        Scene* scene = g_game.changeScene("saved_scene.dat");
        scene->startRace();
        menuMode = HIDDEN;
    }

    if (g_gui.button("Load Game"))
    {
    }

    if (g_gui.button("Quick Play"))
    {
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

void Menu::showOptionsMenu()
{
    menuMode = MenuMode::OPTIONS_MAIN;
    tmpConfig = g_game.config;
}

void Menu::mainOptions()
{
    g_gui.beginPanel("Options", { g_game.windowWidth/2, g_game.windowHeight*0.35f }, 240,
            0.5f, true, 4);

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
    g_gui.beginPanel("Audio Options", { g_game.windowWidth/2, g_game.windowHeight*0.28f }, 330,
            0.5f, true, 7);

    g_gui.slider("Master Volume", 0.f, 1.f, tmpConfig.audio.masterVolume);
    g_gui.slider("Engine Volume", 0.f, 1.f, tmpConfig.audio.engineVolume);
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
        g_game.config.audio = defaultConfig.audio;
        showMainMenu();
    }

    if (g_gui.button("Cancel"))
    {
        showOptionsMenu();
    }

    g_gui.end();
}

void Menu::gameplayOptions()
{
    g_gui.beginPanel("Gameplay Options", { g_game.windowWidth/2, g_game.windowHeight*0.28f }, 280,
            0.5f, true, 5);

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
        g_game.config.gameplay = defaultConfig.gameplay;
        showMainMenu();
    }

    if (g_gui.button("Cancel"))
    {
        showOptionsMenu();
    }

    g_gui.end();
}

void Menu::graphicsOptions()
{
    g_gui.beginPanel("Graphics Options", { g_game.windowWidth/2, g_game.windowHeight*0.19f }, 470,
            0.5f, true, 10);

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
    for (i32 i=0; i<ARRAY_SIZE(resolutions); ++i)
    {
        if (resolutions[i].x == tmpConfig.graphics.resolutionX &&
            resolutions[i].y == tmpConfig.graphics.resolutionY)
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
    for (i32 i=0; i<ARRAY_SIZE(shadowMapResolutions); ++i)
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
    for (i32 i=0; i<ARRAY_SIZE(aaLevels); ++i)
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
        g_game.renderer->initShaders();
        g_game.renderer->updateFramebuffers();
        SDL_SetWindowFullscreen(g_game.window, g_game.config.graphics.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        showMainMenu();
    }

    if (g_gui.button("Reset to Defaults"))
    {
        Config defaultConfig;
        tmpConfig.graphics = defaultConfig.graphics;
        g_game.config.graphics = defaultConfig.graphics;
        g_game.renderer->initShaders();
        g_game.renderer->updateFramebuffers();
        SDL_SetWindowFullscreen(g_game.window, g_game.config.graphics.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    }

    if (g_gui.button("Cancel"))
    {
        showOptionsMenu();
    }

    g_gui.end();
}

void Menu::onUpdate(Renderer* renderer, f32 deltaTime)
{
    switch (menuMode)
    {
        case MenuMode::HIDDEN:
            break;
        case MenuMode::MAIN_MENU:
            mainMenu();
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
