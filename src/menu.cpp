#include "menu.h"
#include "resources.h"
#include "renderer.h"
#include "game.h"
#include "2d.h"
#include "input.h"
#include "scene.h"

const char* menuItems[] = {
    "Championship",
    "Load Game",
    "Quick Play",
    "Options",
    "Track Editor",
    //"About",
    "Quit"
};

void Menu::onUpdate(Renderer* renderer, f32 deltaTime)
{
    if (menuMode == HIDDEN)
    {
        return;
    }

    Texture* white = g_resources.getTexture("white");
    f32 height = g_game.windowHeight;
    f32 menuWidth = height * 0.42f;
    f32 menuStartY = height * 0.1f;
    f32 menuHeight = height - menuStartY * 2;

    Font* font = &g_resources.getFont("font", height * 0.04f);

    renderer->push(QuadRenderable(white,
                { 0, menuStartY }, menuWidth, menuHeight, glm::vec3(0, 0, 0), 0.9f, true));

    if (g_input.isKeyPressed(KEY_UP, true))
    {
        --selectIndex;
        if (selectIndex < 0)
        {
            selectIndex = ARRAY_SIZE(menuItems) - 1;
        }
    }

    if (g_input.isKeyPressed(KEY_DOWN, true))
    {
        ++selectIndex;
        if (selectIndex >= ARRAY_SIZE(menuItems))
        {
            selectIndex = 0;
        }
    }

    f32 buttonHeight = height * 0.07f;
    f32 buttonSpacing = height * 0.01f;
    for (u32 i=0; i<ARRAY_SIZE(menuItems); ++i)
    {
        f32 x = height * 0.05f;
        f32 y = menuStartY + buttonHeight + i * buttonHeight + i * buttonSpacing;
        if (selectIndex == i)
        {
            renderer->push(QuadRenderable(white,
                        { 0, y - buttonHeight/2 }, menuWidth, buttonHeight,
                        glm::vec3(1.0f), 0.04f));
        }
        renderer->push(TextRenderable(font, menuItems[i], { x, y },
                    glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::LEFT, VerticalAlign::CENTER));
    }

    if (g_input.isKeyPressed(KEY_RETURN) || g_input.isKeyPressed(KEY_SPACE))
    {
        switch (selectIndex)
        {
            case 0:
            {
                g_game.isEditing = false;
                Scene* scene = g_game.changeScene("saved_scene.dat");
                scene->startRace();
                menuMode = HIDDEN;
            } break;
            case 1:
            {
            } break;
            case 2:
            {
            } break;
            case 3:
            {
            } break;
            case 4:
            {
                g_game.isEditing = true;
                g_game.changeScene(nullptr);
                menuMode = HIDDEN;
                break;
            } break;
            case 5:
            {
                g_game.shouldExit = true;
            } break;
        }
    }
}
