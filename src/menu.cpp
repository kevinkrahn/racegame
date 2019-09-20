#include "menu.h"
#include "resources.h"
#include "renderer.h"
#include "game.h"
#include "2d.h"
#include "input.h"
#include "scene.h"
#include "gui.h"

void Menu::onUpdate(Renderer* renderer, f32 deltaTime)
{
    if (menuMode == HIDDEN)
    {
        return;
    }

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
