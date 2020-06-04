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
#include "vehicle.h"

const glm::vec4 COLOR_SELECTED = glm::vec4(1.f, 0.6f, 0.05f, 1.f);
const glm::vec4 COLOR_NOT_SELECTED = glm::vec4(1.f);

const char* championshipTracks[] = {
    "race1",
    "race2",
    "race3",
    "race4",
    "race5",
    "race6",
    "race7",
    "race8",
    "race9",
    "race10",

    "race1",
    "race2",
    "race3",
    "race4",
    "race5",
    "race6",
    "race7",
    "race8",
    "race9",
    "race10",

    "race1",
    "race2",
    "race3",
    "race4",
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
    print("Starting quick race with driver budget: ", driverCredits, '\n');
    Array<Driver> drivers;
    const u32 driverCount = 10;
    i32 driverIndexOffset = irandom(series, 0, (i32)g_ais.size());
    for (u32 i=0; i<driverCount; ++i)
    {
        drivers.push_back(Driver(i==0, i==0, i==0, 0, -1,
                    0, (driverIndexOffset + i) % g_ais.size()));
        drivers.back().credits = driverCredits;
        drivers.back().aiUpgrades(series);
    }
#if 0
    drivers[0].hasCamera = true;
    drivers[1].hasCamera = true;
    drivers[2].hasCamera = true;
    drivers[3].hasCamera = true;
#endif

    // shuffle
    for (u32 i=0; i<driverCount; ++i)
    {
        i32 index = irandom(series, 0, (i32)drivers.size());
        Driver driver = std::move(drivers[index]);
        drivers.erase(drivers.begin() + index);
        g_game.state.drivers.push_back(std::move(driver));
    }

    g_game.state.gameMode = GameMode::QUICK_RACE;
    g_game.isEditing = false;
#if 0
    Scene* scene = g_game.changeScene("race7");
#else
    Scene* scene = g_game.changeScene(
            championshipTracks[irandom(series, 0, (i32)ARRAY_SIZE(championshipTracks))]);
#endif
    scene->startRace();
    menuMode = HIDDEN;
}

constexpr f32 REFERENCE_HEIGHT = 1080.f;
f32 convertSize(f32 size)
{
    return glm::floor(size * (g_game.windowHeight / REFERENCE_HEIGHT));
}

glm::vec2 convertSize(glm::vec2 size)
{
    return glm::vec2(convertSize(size.x), convertSize(size.y));
}

bool didSelect()
{
    bool result = g_input.isKeyPressed(KEY_RETURN);
    for (auto& pair : g_input.getControllers())
    {
        if (pair.second.isButtonPressed(BUTTON_A))
        {
            result = true;
            break;
        }
    }
    return result;
}

bool didGoBack()
{
    if (g_input.isKeyPressed(KEY_ESCAPE))
    {
        return true;
    }
    for (auto& c : g_input.getControllers())
    {
        if (c.second.isButtonPressed(BUTTON_B))
        {
            return true;
        }
    }
    return false;
}

i32 Menu::didChangeSelectionX()
{
    i32 result = (i32)(g_input.isKeyPressed(KEY_RIGHT, true) || didSelect())
                - (i32)g_input.isKeyPressed(KEY_LEFT, true);

    for (auto& pair : g_input.getControllers())
    {
        i32 tmpResult = pair.second.isButtonPressed(BUTTON_DPAD_RIGHT) -
                        pair.second.isButtonPressed(BUTTON_DPAD_LEFT);
        if (!tmpResult)
        {
            if (repeatTimer == 0.f)
            {
                f32 xaxis = pair.second.getAxis(AXIS_LEFT_X);
                if (xaxis < -0.2f)
                {
                    tmpResult = -1;
                    repeatTimer = 0.1f;
                }
                else if (xaxis > 0.2f)
                {
                    tmpResult = 1;
                    repeatTimer = 0.1f;
                }
            }
        }
        if (tmpResult)
        {
            result = tmpResult;
            break;
        }
    }

    return result;
}

i32 Menu::didChangeSelectionY()
{
    i32 result = (i32)g_input.isKeyPressed(KEY_DOWN, true)
                - (i32)g_input.isKeyPressed(KEY_UP, true);

    for (auto& pair : g_input.getControllers())
    {
        i32 tmpResult = pair.second.isButtonPressed(BUTTON_DPAD_DOWN) -
                        pair.second.isButtonPressed(BUTTON_DPAD_UP);
        if (!tmpResult)
        {
            if (repeatTimer == 0.f)
            {
                f32 yaxis = pair.second.getAxis(AXIS_LEFT_Y);
                if (yaxis < -0.2f)
                {
                    tmpResult = -1;
                    repeatTimer = 0.1f;
                }
                else if (yaxis > 0.2f)
                {
                    tmpResult = 1;
                    repeatTimer = 0.1f;
                }
            }
        }
        if (tmpResult)
        {
            result = tmpResult;
            break;
        }
    }

    return result;
}

Widget* Menu::addBackgroundBox(glm::vec2 pos, glm::vec2 size, f32 alpha)
{
    Widget w;
    w.pos = pos;
    w.size = size;
    w.onRender = [alpha](Widget& w, bool){
        glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        glm::vec2 size = convertSize(w.size) * w.fadeInScale;
        glm::vec2 pos = convertSize(w.pos) + center - size * 0.5f;
        g_game.renderer->push2D(Quad(&g_res.white, pos, size.x, size.y,
                    glm::vec4(glm::vec3(0.f), alpha), w.fadeInAlpha), -100);
    };
    w.flags = 0;
    widgets.push_back(w);
    return &widgets.back();
}

Widget* Menu::addLogic(std::function<void()> onUpdate)
{
    Widget w;
    w.flags = 0;
    w.onRender = [onUpdate](Widget& w, bool isSelected){ onUpdate(); };
    widgets.push_back(w);
    return &widgets.back();
}

Widget* Menu::addButton(const char* text, const char* helpText, glm::vec2 pos, glm::vec2 size,
        std::function<void()> onSelect, u32 flags)
{
    Font* font = &g_res.getFont("font", (u32)convertSize(38));

    Widget button;
    button.helpText = helpText;
    button.pos = pos;
    button.size = size;
    button.onSelect = std::move(onSelect);
    button.onRender = [text, font](Widget& btn, bool isSelected){
        f32 borderSize = convertSize(isSelected ? 5 : 2);
        glm::vec4 borderColor = glm::mix(COLOR_NOT_SELECTED, COLOR_SELECTED, btn.hover);
        glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        glm::vec2 size = convertSize(btn.size) * btn.fadeInScale;
        glm::vec2 pos = convertSize(btn.pos) + center - size * 0.5f;
        g_game.renderer->push2D(Quad(&g_res.white,
                    pos - borderSize, size.x + borderSize * 2, size.y + borderSize * 2,
                    borderColor, (0.5f + btn.hover * 0.5f) * btn.fadeInAlpha));
        g_game.renderer->push2D(Quad(&g_res.white, pos, size.x, size.y,
                glm::vec4(glm::vec3((sinf(btn.hoverTimer * 4.f) + 1.f)*0.5f*btn.hover*0.04f), 0.8f),
                btn.fadeInAlpha));
        g_game.renderer->push2D(TextRenderable(font, text, pos + size * 0.5f,
                    glm::vec3(1.f), (isSelected ? 1.f : 0.5f) * btn.fadeInAlpha, btn.fadeInScale,
                    HorizontalAlign::CENTER, VerticalAlign::CENTER));
    };
    button.fadeInScale = 0.7f;
    button.flags = WidgetFlags::SELECTABLE |
                   WidgetFlags::NAVIGATE_VERTICAL |
                   WidgetFlags::NAVIGATE_HORIZONTAL | flags;
    widgets.push_back(button);
    return &widgets.back();
}

Widget* Menu::addSelector(const char* text, const char* helpText, glm::vec2 pos, glm::vec2 size,
        SmallArray<std::string> values, i32 valueIndex,
        std::function<void(i32 valueIndex)> onValueChanged)
{
    Font* font = &g_res.getFont("font", (u32)convertSize(38));

    Widget w;
    w.helpText = helpText;
    w.pos = pos;
    w.size = size;
    w.onSelect = []{};
    w.onRender = [=](Widget& w, bool isSelected) mutable {
        glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        glm::vec2 size = convertSize(w.size) * w.fadeInScale;
        glm::vec2 pos = convertSize(w.pos) + center - size * 0.5f;
        if (isSelected)
        {
            f32 borderSize = convertSize(isSelected ? 5 : 2);
            glm::vec4 borderColor = glm::mix(COLOR_NOT_SELECTED, COLOR_SELECTED, w.hover);
            g_game.renderer->push2D(Quad(&g_res.white,
                        pos - borderSize, size.x + borderSize * 2, size.y + borderSize * 2,
                        borderColor, (0.5f + w.hover * 0.5f) * w.fadeInAlpha));
        }
        g_game.renderer->push2D(Quad(&g_res.white, pos, size.x, size.y,
                glm::vec4(glm::vec3((sinf(w.hoverTimer * 4.f) + 1.f)*0.5f*w.hover*0.04f), 0.8f),
                w.fadeInAlpha));
        g_game.renderer->push2D(TextRenderable(font, text,
                    pos + glm::vec2(convertSize(20.f), size.y * 0.5f),
                    glm::vec3(1.f), (isSelected ? 1.f : 0.5f) * w.fadeInAlpha, w.fadeInScale,
                    HorizontalAlign::LEFT, VerticalAlign::CENTER));
        g_game.renderer->push2D(TextRenderable(font, tstr(values[valueIndex]),
                    pos + glm::vec2(size.x * 0.75f, size.y * 0.5f),
                    glm::vec3(1.f), (isSelected ? 1.f : 0.5f) * w.fadeInAlpha, w.fadeInScale,
                    HorizontalAlign::CENTER, VerticalAlign::CENTER));

        if (isSelected)
        {
            Texture* cheveron = g_res.getTexture("cheveron");
            f32 cheveronSize = convertSize(36);
            f32 offset = convertSize(150.f);
            g_game.renderer->push2D(Quad(cheveron,
                    pos + glm::vec2(size.x*0.75f - offset - cheveronSize, size.y*0.5f - cheveronSize*0.5f),
                    cheveronSize, cheveronSize, glm::vec4(1.f), w.fadeInAlpha * w.hover));
            g_game.renderer->push2D(Quad(cheveron,
                    pos + glm::vec2(size.x*0.75f + offset, size.y*0.5f - cheveronSize*0.5f),
                    cheveronSize, cheveronSize, {1,0}, {0,1}, glm::vec4(1.f), w.fadeInAlpha * w.hover));

            i32 dir = didChangeSelectionX();
            if (dir)
            {
                g_audio.playSound(g_res.getSound("click"), SoundType::MENU_SFX);
            }
            if (g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                glm::vec2 mousePos = g_input.getMousePosition();
                f32 x = convertSize(pos.x + size.x * 0.75f);
                dir = mousePos.x - x < 0 ? -1 : 1;
            }
            if (dir)
            {
                valueIndex += dir;
                if (valueIndex < 0)
                {
                    valueIndex = (i32)values.size() - 1;
                }
                if (valueIndex >= (i32)values.size())
                {
                    valueIndex = 0;
                }
                onValueChanged(valueIndex);
            }
        }
    };
    w.fadeInScale = 0.7f;
    w.flags = WidgetFlags::SELECTABLE |
              WidgetFlags::NAVIGATE_VERTICAL;
    widgets.push_back(w);
    return &widgets.back();
}

Widget* Menu::addHelpMessage(glm::vec2 pos)
{
    Widget w;
    w.pos = pos;
    w.onRender = [this](Widget& widget, bool isSelected) {
        if (selectedWidget->helpText)
        {
            Font* font = &g_res.getFont("font", (u32)convertSize(26));
            f32 textWidth = font->stringDimensions(selectedWidget->helpText).x + convertSize(50.f);
            glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
            glm::vec2 size = glm::vec2(textWidth, convertSize(50)) * widget.fadeInScale;
            glm::vec2 pos = convertSize(widget.pos) + center - size * 0.5f;
            f32 alpha = selectedWidget->hover * widget.fadeInAlpha;
            g_game.renderer->push2D(Quad(&g_res.white,
                        pos, size.x, size.y, glm::vec4(0,0,0,0.5f), alpha));
            g_game.renderer->push2D(TextRenderable(font, selectedWidget->helpText,
                        pos + size * 0.5f, glm::vec3(1.f), alpha, widget.fadeInScale,
                        HorizontalAlign::CENTER, VerticalAlign::CENTER));
        }
    };
    w.flags = 0;
    widgets.push_back(w);
    return &widgets.back();
}

Widget* Menu::addLabel(const char* text, glm::vec2 pos, f32 width, Font* font)
{
    Widget w;
    w.pos = pos;
    w.onRender = [text, font, width](Widget& widget, bool isSelected) {
        glm::vec2 textSize = font->stringDimensions(text)
            + glm::vec2(convertSize(60.f), convertSize(40.f));
        if (width > 0.f) { textSize.x = convertSize(width); }
        glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        glm::vec2 size = textSize * widget.fadeInScale;
        glm::vec2 pos = convertSize(widget.pos) + center - size * 0.5f;
#if 0
        g_game.renderer->push2D(Quad(&g_res.white,
                    pos, size.x, size.y, glm::vec4(0,0,0,0.5f), widget.fadeInAlpha));
#endif
        g_game.renderer->push2D(TextRenderable(font, text,
                    pos + size * 0.5f, glm::vec3(1.f), widget.fadeInAlpha, widget.fadeInScale,
                    HorizontalAlign::CENTER, VerticalAlign::CENTER));
    };
    w.flags = 0;
    widgets.push_back(w);
    return &widgets.back();
}

Widget* Menu::addTitle(const char* text, glm::vec2 pos)
{
    Font* font = &g_res.getFont("font_bold", (u32)convertSize(88));
    Widget w;
    w.pos = pos;
    w.onRender = [text, font](Widget& widget, bool isSelected){
        glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        glm::vec2 pos = convertSize(widget.pos) + center;
        glm::vec2 boxSize = convertSize({5000, 90});
        g_game.renderer->push2D(Quad(&g_res.white,
                    pos-boxSize*0.5f, boxSize.x, boxSize.y, glm::vec4(0,0,0,0.5f), widget.fadeInAlpha));
        g_game.renderer->push2D(TextRenderable(font, text,
                    pos, glm::vec3(1.f), widget.fadeInAlpha, widget.fadeInScale,
                    HorizontalAlign::CENTER, VerticalAlign::CENTER));
    };
    w.flags = 0;
    widgets.push_back(w);
    return &widgets.back();
}

void Menu::showMainMenu()
{
    reset();

    addTitle("Main Menu");

    glm::vec2 size(380, 200);
    glm::vec2 smallSize(380, 100);
    selectedWidget = addButton("CHAMPIONSHIP", "Start a new championship with up to 4 players!", { -400, -200 }, size, [this]{
        showNewChampionshipMenu();
    }, WidgetFlags::FADE_OUT);
    addButton("LOAD CHAMPIONSHIP", "Resume a previous championship.", { -0, -200 }, size, [this]{
        reset();
        g_game.loadGame();
        menuMode = CHAMPIONSHIP_MENU;
        g_game.changeScene(championshipTracks[g_game.state.currentRace]);
    }, WidgetFlags::FADE_OUT | WidgetFlags::FADE_TO_BLACK);
    addButton("QUICK RACE", "Jump into a race straightaway!", { 400, -200 }, size, [this]{
        reset();
        startQuickRace();
    }, WidgetFlags::FADE_OUT | WidgetFlags::FADE_TO_BLACK);
    addButton("SETTINGS", "Settings change things.", { -400, -30 }, smallSize, [this]{
        showSettingsMenu();
    }, WidgetFlags::FADE_OUT);
    addButton("EDITOR", "Edit things.", { 0, -30 }, smallSize, [this]{
        g_game.isEditing = true;
        g_game.unloadScene();
        menuMode = HIDDEN;
    }, WidgetFlags::FADE_OUT);
    addButton("CHALLENGES", "Test your abilities in varied and challenging scenarios.", { 400, -30 },
            smallSize, []{}, WidgetFlags::FADE_OUT);

    addButton("EXIT", "Terminate your entertainment experience.", {0, 300}, {200, 50}, []{
        g_game.shouldExit = true;
    }, /*WidgetFlags::FADE_OUT | WidgetFlags::FADE_TO_BLACK*/ 0);

    addHelpMessage({0, 100});
}

void Menu::showNewChampionshipMenu()
{
    g_game.state = {};
    reset();

    addTitle("New Championship");

    addLabel("Press SPACE or controller button to join", { 0, -320 }, 840,
            &g_res.getFont("font", (u32)convertSize(38)));
    glm::vec2 size(250, 75);
    selectedWidget = addButton("BACK", "Return to main menu.", { -280, 320 }, size, [this]{
        showMainMenu();
    }, WidgetFlags::FADE_OUT | WidgetFlags::BACK);
    Widget* beginButton = addButton("BEGIN", "Begin the championship!", { 280, 320 }, size, [this]{
        g_game.isEditing = false;
        g_game.state.currentLeague = 0;
        g_game.state.currentRace = 0;
        g_game.state.gameMode = GameMode::CHAMPIONSHIP;
        g_game.changeScene(championshipTracks[g_game.state.currentRace]);

        // add AI drivers
        for (i32 i=(i32)g_game.state.drivers.size(); i<10; ++i)
        {
            g_game.state.drivers.push_back(Driver(false, false, false, 0, -1, i, i));
        }
        RandomSeries series = randomSeed();
        for (auto& driver : g_game.state.drivers)
        {
            if (!driver.isPlayer)
            {
                driver.aiUpgrades(series);
            }
        }
        reset();
        menuMode = CHAMPIONSHIP_MENU;
    }, WidgetFlags::FADE_OUT | WidgetFlags::FADE_TO_BLACK | WidgetFlags::DISABLED);

    for (u32 i=0; i<4; ++i)
    {
        glm::vec2 size(750, 100);
        Widget w;
        w.pos = { 0, -220 + i * (size.y + 15) };
        w.size = size;
        w.flags = 0;
        w.onSelect = []{};
        w.onRender = [i](Widget& btn, bool isSelected){
            f32 borderSize = convertSize(isSelected ? 5 : 2);
            glm::vec4 borderColor = glm::mix(COLOR_NOT_SELECTED, COLOR_SELECTED, btn.hover);
            glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
            glm::vec2 size = convertSize(btn.size) * btn.fadeInScale;
            glm::vec2 pos = convertSize(btn.pos) + center - size * 0.5f;
            bool enabled = g_game.state.drivers.size() > i;
            if (enabled)
            {
                g_game.renderer->push2D(Quad(&g_res.white,
                            pos - borderSize, size.x + borderSize * 2, size.y + borderSize * 2,
                            borderColor, (0.5f + btn.hover * 0.5f) * btn.fadeInAlpha));
                g_game.renderer->push2D(Quad(&g_res.white, pos, size.x, size.y,
                            glm::vec4(glm::vec3((sinf(btn.hoverTimer * 4.f) + 1.f)*0.5f*btn.hover*0.04f), 0.8f),
                            btn.fadeInAlpha));

                Font* font = &g_res.getFont("font", (u32)convertSize(38));
                const char* text = tstr(g_game.state.drivers[i].playerName);
                f32 textAlpha = isSelected ? 1.f : 0.5f;
                g_game.renderer->push2D(TextRenderable(font, text,
                            pos + glm::vec2(convertSize(20), size.y * 0.5f),
                            glm::vec3(1.f), textAlpha * btn.fadeInAlpha, btn.fadeInScale,
                            HorizontalAlign::LEFT, VerticalAlign::CENTER));

                const char* icon = g_game.state.drivers[i].useKeyboard
                        ? "icon_keyboard" : "icon_controller";
                f32 iconSize = convertSize(75);
                f32 margin = convertSize(20);
                g_game.renderer->push2D(Quad(g_res.getTexture(icon),
                            pos + glm::vec2(size.x - iconSize - margin, size.y * 0.5f - iconSize * 0.5f),
                            iconSize, iconSize, glm::vec4(1.f), btn.fadeInAlpha));

                btn.flags = WidgetFlags::SELECTABLE | WidgetFlags::NAVIGATE_VERTICAL;

            }
            else
            {
                g_game.renderer->push2D(Quad(&g_res.white,
                            pos - borderSize, size.x + borderSize * 2, size.y + borderSize * 2,
                            borderColor, 0.35f * btn.fadeInAlpha));
                g_game.renderer->push2D(Quad(&g_res.white, pos, size.x, size.y,
                            glm::vec4(glm::vec3(0.f), 0.3f), btn.fadeInAlpha));
                Font* font = &g_res.getFont("font", (u32)convertSize(30));
                f32 textAlpha = 0.3f;
                g_game.renderer->push2D(TextRenderable(font, "Empty Player Slot",
                            pos + glm::vec2(convertSize(20), size.y * 0.5f),
                            glm::vec3(1.f), textAlpha * btn.fadeInAlpha, btn.fadeInScale,
                            HorizontalAlign::LEFT, VerticalAlign::CENTER));
                btn.flags = 0;
            }
        };
        widgets.push_back(w);
    }

    addBackgroundBox({0,0}, {880, 780});
    addLogic([this, beginButton]{
        // TODO: ensure all player names are unique before starting championship
        if (!g_game.state.drivers.empty())
        {
            beginButton->flags = beginButton->flags & ~WidgetFlags::DISABLED;
        }

        if (g_game.state.drivers.size() < 4)
        {
            if (g_input.isKeyPressed(KEY_SPACE) && fadeInTimer > 800.f)
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
                    g_game.state.drivers.push_back(Driver(true, true, true, 0));
                    g_game.state.drivers.back().playerName = str("Player ", g_game.state.drivers.size());
                }
            }

            for (auto& controller : g_input.getControllers())
            {
                if (controller.second.isAnyButtonPressed())
                {
                    bool controllerPlayerExists = false;
                    for (auto& driver : g_game.state.drivers)
                    {
                        if (driver.isPlayer && !driver.useKeyboard && driver.controllerID == controller.first)
                        {
                            controllerPlayerExists = true;
                            break;
                        }
                    }

                    if (!controllerPlayerExists)
                    {
                        g_game.state.drivers.push_back(Driver(true, true, false, controller.first));
                        g_game.state.drivers.back().controllerGuid = controller.second.getGuid();
                        g_game.state.drivers.back().playerName = str("Player ", g_game.state.drivers.size());
                    }
                }
            }
        }
    });
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
    Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("world.Quad");
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
                glm::mat4 vehicleTransform = glm::rotate(glm::mat4(1.f), (f32)getTime(), glm::vec3(0, 0, 1));
                driver.getVehicleData()->render(&renderWorlds[playerIndex],
                    glm::translate(glm::mat4(1.f),
                        glm::vec3(0, 0, driver.getTuning().getRestOffset())) *
                    vehicleTransform, nullptr, *driver.getVehicleConfig());
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
    if (g_gui.button("Begin Race", playerIndex == vehicleCount, g_res.getTexture("icon_flag"), false))
    {
        Scene* scene = g_game.changeScene(championshipTracks[g_game.state.currentRace]);
        scene->startRace();
        menuMode = HIDDEN;
        g_gui.clearSelectionStack();
    }
    if (g_gui.button("Championship Standings", true, g_res.getTexture("icon_stats"), false))
    {
        menuMode = CHAMPIONSHIP_STANDINGS;
        g_gui.pushSelection();
    }
    g_gui.button("Player Stats", true, g_res.getTexture("icon_stats2"), false);
    g_gui.gap(10);

    if (g_gui.button("Quit"))
    {
        showMainMenu();
        g_gui.clearSelectionStack();
    }

    g_gui.end();

    Font* bigfont = &g_res.getFont("font", (u32)g_gui.convertSize(64));
    g_game.renderer->push2D(TextRenderable(bigfont, tstr("League ", (char)('A' + g_game.state.currentLeague)),
                menuPos + o, glm::vec3(1.f)));
    Font* mediumFont = &g_res.getFont("font", (u32)g_gui.convertSize(32));
    g_game.renderer->push2D(TextRenderable(mediumFont, tstr("Race ", g_game.state.currentRace + 1),
                menuPos + o + glm::vec2(0, glm::floor(g_gui.convertSize(52))), glm::vec3(1.f)));

    u32 trackPreviewSize = (u32)g_gui.convertSize(380);
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

    Texture* white = &g_res.white;
    u32 vehicleIconWidth = (u32)g_gui.convertSize(290);
    u32 vehicleIconHeight = (u32)g_gui.convertSize(256);
    Font* smallfont = &g_res.getFont("font", (u32)g_gui.convertSize(16));
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
    VehicleConfiguration vehicleConfig2 = vehicleConfig;

    glm::vec2 panelPos = menuPos + glm::vec2(w - o, oy);
    g_gui.beginPanel(tstr("Garage ", mode), panelPos, 1.f, false, true, false);
    if (driver.vehicleIndex == -1)
    {
        g_gui.pushSelection();
    }

    u32 previousMode = mode;
    if (mode == 0)
    {
        currentVehicleIndex = driver.vehicleIndex;

        if (g_gui.button("Choose Vehicle"))
        {
            mode = 1;
            g_gui.pushSelection();
            g_gui.forceSelection(0);
        }
        g_gui.label("Vehicle Upgrades");
        if (g_gui.button("Performance", driver.vehicleIndex != -1, g_res.getTexture("icon_engine"), false))
        {
            mode = 2;
            g_gui.pushSelection();
            g_gui.forceSelection(0);
        }
        if (g_gui.button("Cosmetics", driver.vehicleIndex != -1, g_res.getTexture("icon_spraycan"), false))
        {
            mode = 3;
            g_gui.pushSelection();
            g_gui.forceSelection(0);
        }
        g_gui.label("Equipment");
        for (u32 i=0; i<g_vehicles[currentVehicleIndex]->frontWeaponCount; ++i)
        {
            i32 weaponIndex = vehicleConfig.frontWeaponIndices[i];
            if (g_gui.button(tstr("Front Weapon ", i + 1), driver.vehicleIndex != -1,
                        weaponIndex == -1 ? g_res.getTexture("iconbg") : g_weapons[weaponIndex].info.icon))
            {
                mode = i + 4;
                g_gui.pushSelection();
                g_gui.forceSelection(0);
            }
        }
        for (u32 i=0; i<g_vehicles[currentVehicleIndex]->rearWeaponCount; ++i)
        {
            i32 weaponIndex = vehicleConfig.rearWeaponIndices[i];
            if (g_gui.button(tstr("Rear Weapon ", i + 1), driver.vehicleIndex != -1,
                        weaponIndex == -1 ? g_res.getTexture("iconbg") : g_weapons[weaponIndex].info.icon))
            {
                mode = i + 7;
                g_gui.pushSelection();
                g_gui.forceSelection(0);
            }
        }
        if (g_gui.button("Passive Ability", driver.vehicleIndex != -1,
                    vehicleConfig.specialAbilityIndex == -1 ? g_res.getTexture("iconbg")
                    : g_weapons[vehicleConfig.specialAbilityIndex].info.icon))
        {
            mode = 9;
            g_gui.pushSelection();
            g_gui.forceSelection(0);
        }
    }
    else if (mode == 1)
    {
        Array<std::string> carNames;
        for (auto& v : g_vehicles)
        {
            carNames.push_back(v->name);
        }

        if (currentVehicleIndex == -1)
        {
            currentVehicleIndex = 0;
        }

        if (g_gui.select("Vehicle", carNames.data(), (i32)carNames.size(), currentVehicleIndex))
        {
        }

        auto ownedVehicle = driver.ownedVehicles.find(
                [&](auto& e) { return e.vehicleIndex == currentVehicleIndex; });
        bool isOwned = !!ownedVehicle;
        VehicleData* vehicleData = g_vehicles[currentVehicleIndex].get();

        if (isOwned)
        {
            driver.vehicleIndex = currentVehicleIndex;
        }
        else
        {
            driver.vehicleIndex = -1;
            vehicleConfig = VehicleConfiguration{};
            RandomSeries s{(u32)currentVehicleIndex+1};
            // TODO: set default color in vehicle class
            vehicleConfig.colorIndex = irandom(s, 0, ARRAY_SIZE(g_vehicleColors));
        }

        messageStr = vehicleData->description;

        g_gui.gap(20);
        if (g_gui.button("Purchase", !isOwned && driver.credits >= vehicleData->price))
        {
            driver.ownedVehicles.push_back({
                currentVehicleIndex,
                vehicleConfig
            });
            driver.vehicleIndex = currentVehicleIndex;
            isOwned = true;
            driver.credits -= vehicleData->price;
        }
        if (g_gui.button("Sell", isOwned))
        {
            // TODO: make the vehicle worth more when it has upgrades
            driver.credits += vehicleData->price;
            driver.vehicleIndex = -1;
            driver.ownedVehicles.erase(ownedVehicle);
        }
        g_gui.gap(20);

        if (g_gui.button("Done", isOwned) || g_gui.didGoBack())
        {
            g_gui.popSelection();
            mode = 0;
            // TODO: fix the gui and remove this hack
            g_gui.clearWidgetState("Done");
        }

        g_game.renderer->push2D(QuadRenderable(white,
                    vehiclePreviewPos + glm::vec2(0, vehicleIconHeight - g_gui.convertSize(26)),
                    g_gui.convertSize(120), g_gui.convertSize(26),
                    glm::vec3(0.f), 0.25f, false), 1);
        g_game.renderer->push2D(TextRenderable(smallfont,
                    tstr(isOwned ? "Value: " : "Price: ", vehicleData->price),
                    vehiclePreviewPos + glm::vec2(g_gui.convertSize(8), vehicleIconHeight - g_gui.convertSize(16)),
                    glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::LEFT), 2);
    }
    else if (mode == 2)
    {
        g_gui.label("Performance Upgrades");
        for (i32 i=0; i<(i32)driver.getVehicleData()->availableUpgrades.size(); ++i)
        {
            auto& upgrade = driver.getVehicleData()->availableUpgrades[i];
            auto currentUpgrade = vehicleConfig.performanceUpgrades.find(
                    [&](auto& u) { return u.upgradeIndex == i; });
            bool isEquipped = !!currentUpgrade;
            i32 upgradeLevel = 1;
            const char* extraText = nullptr;
            i32 price = upgrade.price * upgradeLevel;
            if (isEquipped)
            {
                upgradeLevel = currentUpgrade->upgradeLevel;
                extraText =
                    tstr(upgradeLevel, "/", upgrade.maxUpgradeLevel);
                price = upgrade.price * (upgradeLevel + 1);
            }
            bool isSelected = false;
            if (g_gui.itemButton(upgrade.name, tstr("Price: ", price),
                    extraText, driver.credits >= price &&
                        ((upgradeLevel < upgrade.maxUpgradeLevel && isEquipped) || !isEquipped),
                    upgrade.icon, &isSelected))
            {
                g_audio.playSound(g_res.getSound("airdrill"), SoundType::MENU_SFX);
                driver.credits -= price;
                driver.getVehicleConfig()->addUpgrade(i);
            }
            if (isSelected)
            {
                messageStr = upgrade.description;
                if (upgradeLevel < upgrade.maxUpgradeLevel ||
                        (!isEquipped && upgrade.maxUpgradeLevel == 1))
                {
                    vehicleConfig2.addUpgrade(i);
                }
            }
        }
    }
    else if (mode == 3)
    {
        g_gui.label("Cosmetics");
        g_gui.select("Color", g_vehicleColorNames,
                (i32)ARRAY_SIZE(g_vehicleColorNames), driver.getVehicleConfig()->colorIndex);
        g_gui.select("Paint Type", g_paintTypeNames,
                (i32)ARRAY_SIZE(g_paintTypeNames), driver.getVehicleConfig()->paintTypeIndex);
        driver.getVehicleConfig()->dirty = true;
    }
    else if (mode >= 4 && mode <= 6)
    {
        u32 weaponNumber = mode - 4;
        g_gui.label(tstr("Front Weapon ", weaponNumber + 1));
        i32 equippedWeaponIndex = vehicleConfig.frontWeaponIndices[weaponNumber];
        bool hasWeapon = driver.getVehicleConfig()->frontWeaponIndices[weaponNumber] != -1;
        for (i32 i = 0; i<(i32)g_weapons.size(); ++i)
        {
            auto& weapon = g_weapons[i];
            if (weapon.info.weaponType != WeaponInfo::FRONT_WEAPON)
            {
                continue;
            }

            bool isSelected = false;

            const char* extraText = nullptr;
            u32 upgradeLevel = vehicleConfig.frontWeaponUpgradeLevel[weaponNumber];
            bool isEquipped = equippedWeaponIndex != -1 && equippedWeaponIndex == i;
            if (isEquipped)
            {
                extraText = tstr(upgradeLevel, "/", weapon.info.maxUpgradeLevel);
            }
            if (g_gui.itemButton(weapon.info.name, tstr("Price: ", weapon.info.price), extraText,
                        driver.credits >= weapon.info.price && (isEquipped || !hasWeapon) &&
                            ((upgradeLevel < weapon.info.maxUpgradeLevel && isEquipped) || !isEquipped),
                        weapon.info.icon, &isSelected))
            {
                driver.credits -= weapon.info.price;
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
                messageStr = weapon.info.description;
            }
        }

        if (hasWeapon)
        {
            g_gui.gap(20);
            auto& weapon = g_weapons[driver.getVehicleConfig()->frontWeaponIndices[weaponNumber]];
            u32 upgradeLevel = driver.getVehicleConfig()->frontWeaponUpgradeLevel[weaponNumber];
            if (g_gui.itemButton(tstr("Sell ", weapon.info.name),
                        tstr("Value: ", weapon.info.price * upgradeLevel / 2), nullptr, true,
                        g_res.getTexture("icon_sell"), nullptr, false))
            {
                driver.credits += weapon.info.price * upgradeLevel / 2;
                driver.getVehicleConfig()->frontWeaponIndices[weaponNumber] = -1;
                driver.getVehicleConfig()->frontWeaponUpgradeLevel[weaponNumber] = 0;
            }
        }
    }
    else if (mode == 7 || mode == 8)
    {
        u32 weaponNumber = mode - 7;
        g_gui.label(tstr("Rear Weapon ", weaponNumber + 1));
        i32 equippedWeaponIndex = vehicleConfig.rearWeaponIndices[weaponNumber];
        bool hasWeapon = driver.getVehicleConfig()->rearWeaponIndices[weaponNumber] != -1;
        for (i32 i = 0; i<(i32)g_weapons.size(); ++i)
        {
            auto& weapon = g_weapons[i];
            if (weapon.info.weaponType != WeaponInfo::REAR_WEAPON)
            {
                continue;
            }

            bool isSelected = false;

            const char* extraText = nullptr;
            u32 upgradeLevel = vehicleConfig.rearWeaponUpgradeLevel[weaponNumber];
            bool isEquipped = equippedWeaponIndex != -1 && equippedWeaponIndex == i;
            if (isEquipped)
            {
                extraText = tstr(upgradeLevel, "/", weapon.info.maxUpgradeLevel);
            }
            if (g_gui.itemButton(weapon.info.name, tstr("Price: ", weapon.info.price), extraText,
                        driver.credits >= weapon.info.price && (isEquipped || !hasWeapon) &&
                            ((upgradeLevel < weapon.info.maxUpgradeLevel && isEquipped) || !isEquipped),
                        weapon.info.icon, &isSelected))
            {
                driver.credits -= weapon.info.price;
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
                messageStr = weapon.info.description;
            }
        }

        if (hasWeapon)
        {
            g_gui.gap(20);
            auto& weapon = g_weapons[driver.getVehicleConfig()->rearWeaponIndices[weaponNumber]];
            u32 upgradeLevel = driver.getVehicleConfig()->rearWeaponUpgradeLevel[weaponNumber];
            if (g_gui.itemButton(tstr("Sell ", weapon.info.name),
                        tstr("Value: ", weapon.info.price * upgradeLevel / 2), nullptr, true,
                        g_res.getTexture("icon_sell"), nullptr, false))
            {
                driver.credits += weapon.info.price * upgradeLevel / 2;
                driver.getVehicleConfig()->rearWeaponIndices[weaponNumber] = -1;
                driver.getVehicleConfig()->rearWeaponUpgradeLevel[weaponNumber] = 0;
            }
        }
    }
    else if (mode == 9)
    {
        g_gui.label("Passive Ability");
        i32 equippedWeaponIndex = vehicleConfig.specialAbilityIndex;
        bool hasWeapon = vehicleConfig.specialAbilityIndex != -1;
        for (i32 i = 0; i<(i32)g_weapons.size(); ++i)
        {
            auto& weapon = g_weapons[i];
            if (weapon.info.weaponType != WeaponInfo::SPECIAL_ABILITY)
            {
                continue;
            }

            bool isSelected = false;

            const char* extraText = nullptr;
            bool isEquipped = equippedWeaponIndex != -1 && equippedWeaponIndex == i;
            if (isEquipped)
            {
                extraText = "Equipped";
            }
            if (g_gui.itemButton(weapon.info.name, tstr("Price: ", weapon.info.price), extraText,
                        driver.credits >= weapon.info.price && !hasWeapon,
                        weapon.info.icon, &isSelected))
            {
                driver.credits -= weapon.info.price;
                driver.getVehicleConfig()->specialAbilityIndex = i;
                // TODO: Play upgrade sound
            }
            if (isSelected)
            {
                messageStr = weapon.info.description;
            }
        }

        if (hasWeapon)
        {
            g_gui.gap(20);
            auto& weapon = g_weapons[driver.getVehicleConfig()->specialAbilityIndex];
            if (g_gui.itemButton(tstr("Sell ", weapon.info.name),
                        tstr("Value: ", weapon.info.price / 2), nullptr, true,
                        g_res.getTexture("icon_sell"), nullptr, false))
            {
                driver.credits += weapon.info.price / 2;
                driver.getVehicleConfig()->specialAbilityIndex = -1;
            }
        }
    }

    g_gui.gap(20);
    if (previousMode != 1 && mode != 1)
    {
        if (g_gui.button("Done") || g_gui.didGoBack())
        {
            if (mode > 0)
            {
                mode = 0;
            }
            else
            {
                menuMode = MenuMode::CHAMPIONSHIP_MENU;
            }
            g_gui.popSelection();
            // TODO: fix the gui and remove this hack
            g_gui.clearWidgetState("Done");
        }
    }

    g_gui.end();

    static RenderWorld renderWorld;
    Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("world.Quad");
    renderWorld.setName("Garage");
    renderWorld.setSize(vehicleIconWidth, vehicleIconHeight);
    renderWorld.push(LitRenderable(quadMesh,
            glm::scale(glm::mat4(1.f), glm::vec3(20.f)), nullptr, glm::vec3(0.02f)));
#if 0
    VehicleTuning tuningReal;
    g_vehicles[currentVehicleIndex]->initTuning(vehicleConfig, tuningReal);
    VehicleTuning tuningUpgrade = tuningReal;
    g_vehicles[currentVehicleIndex]->initTuning(vehicleConfig2, tuningUpgrade);
    g_vehicles[currentVehicleIndex]->render(&renderWorld,
        glm::translate(glm::mat4(1.f),
            glm::vec3(0, 0, tuningUpgrade.getRestOffset())) *
        glm::rotate(glm::mat4(1.f), (f32)getTime(), glm::vec3(0, 0, 1)),
        nullptr, vehicleConfig);
    renderWorld.setViewportCount(1);
    renderWorld.addDirectionalLight(glm::vec3(-0.5f, 0.2f, -1.f), glm::vec3(1.0));
    renderWorld.setViewportCamera(0, glm::vec3(8.f, -8.f, 10.f),
            glm::vec3(0.f, 0.f, 1.f), 1.f, 50.f, 30.f);
    g_game.renderer->addRenderWorld(&renderWorld);

    g_game.renderer->push2D(QuadRenderable(renderWorld.getTexture(),
                vehiclePreviewPos, (f32)vehicleIconWidth, (f32)vehicleIconHeight,
                glm::vec3(1.f), 1.f, false, true));

    g_game.renderer->push2D(QuadRenderable(white,
                vehiclePreviewPos + glm::vec2(0, vehicleIconHeight),
                (f32)vehicleIconWidth, g_gui.convertSize(26),
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
#endif

#if 0
    struct Stat
    {
        const char* name = nullptr;
        f32 value = 0.f;
    };
    static Stat stats[] = {
        { "Acceleration" },
        { "Top Speed" },
        { "Armor" },
        { "Mass" },
        { "Grip" },
        { "Offroad" },
    };
    static f32 statsUpgrade[ARRAY_SIZE(stats)] = { 0 };
    f32 targetStats[] = {
        tuningReal.specs.acceleration,
        tuningReal.specs.topSpeed,
        tuningReal.specs.armor,
        tuningReal.specs.mass,
        tuningReal.specs.grip,
        tuningReal.specs.offroad,
    };
    f32 targetStatsUpgrade[] = {
        tuningUpgrade.specs.acceleration,
        tuningUpgrade.specs.topSpeed,
        tuningUpgrade.specs.armor,
        tuningUpgrade.specs.mass,
        tuningUpgrade.specs.grip,
        tuningUpgrade.specs.offroad,
    };

    const f32 maxBarWidth = (f32)vehicleIconWidth;
    glm::vec2 statsPos = vehiclePreviewPos
        + glm::vec2(0, vehicleIconHeight + glm::floor(g_gui.convertSize(48)));
    f32 barHeight = glm::floor(g_gui.convertSize(5));
    Font* tinyfont = &g_res.getFont("font", (u32)g_gui.convertSize(13));
    for (u32 i=0; i<ARRAY_SIZE(stats); ++i)
    {
        stats[i].value = smoothMove(stats[i].value, targetStats[i], 8.f, g_game.deltaTime);
        statsUpgrade[i] = smoothMove(statsUpgrade[i], targetStatsUpgrade[i], 8.f, g_game.deltaTime);

        g_game.renderer->push2D(TextRenderable(tinyfont, stats[i].name,
                    statsPos + glm::vec2(0, glm::floor(g_gui.convertSize(i * 27.f))),
                    glm::vec3(1.f)));

        g_game.renderer->push2D(QuadRenderable(white,
                    statsPos + glm::vec2(0, glm::floor(g_gui.convertSize(i * 27.f + 12.f))),
                    maxBarWidth, barHeight, glm::vec3(0.f), 0.9f, true));

        f32 upgradeBarWidth = maxBarWidth * statsUpgrade[i];
        f32 barWidth = maxBarWidth * stats[i].value;

        if (upgradeBarWidth > barWidth)
        {
            g_game.renderer->push2D(QuadRenderable(white,
                        statsPos + glm::vec2(0, glm::floor(g_gui.convertSize(i * 27.f + 12.f))),
                        upgradeBarWidth, barHeight, glm::vec3(0.01f, 0.7f, 0.01f)));
        }

        g_game.renderer->push2D(QuadRenderable(white,
                    statsPos + glm::vec2(0, glm::floor(g_gui.convertSize(i * 27.f + 12.f))),
                    barWidth, barHeight, glm::vec3(0.8f)));

        if (upgradeBarWidth < barWidth)
        {
            g_game.renderer->push2D(QuadRenderable(white,
                        statsPos + glm::vec2((f32)upgradeBarWidth, g_gui.convertSizei(i * 27.f + 12.f)),
                        barWidth-upgradeBarWidth, barHeight, glm::vec3(0.8f, 0.01f, 0.01f)));
        }
    }

    if (messageStr)
    {
        g_game.renderer->push2D(TextRenderable(smallfont, messageStr,
                    statsPos + glm::vec2(0, g_gui.convertSizei(ARRAY_SIZE(stats) * 27.f + 8.f)),
                    glm::vec3(1.f)));
    }
#endif
}

void Menu::championshipStandings()
{
    f32 w = g_gui.convertSizei(550);

    f32 cw = (f32)(g_game.windowWidth/2);
    glm::vec2 menuPos = glm::vec2(cw - w/2, glm::floor(g_game.windowHeight * 0.1f));
    glm::vec2 menuSize = glm::vec2(w, (f32)g_game.windowHeight * 0.8f);
    drawBox(menuPos, menuSize);

    Font* bigfont = &g_res.getFont("font_bold", (u32)g_gui.convertSize(26));
    Font* smallfont = &g_res.getFont("font", (u32)g_gui.convertSize(16));

    g_game.renderer->push2D(TextRenderable(bigfont, "Championship Standings",
                glm::vec2(cw, menuPos.y + g_gui.convertSizei(32)), glm::vec3(1.f),
                1.f, 1.f, HorizontalAlign::CENTER, VerticalAlign::TOP));

    g_game.renderer->push2D(TextRenderable(smallfont, tstr("League ", (char)('A' + g_game.state.currentLeague)),
                glm::vec2(cw, menuPos.y + g_gui.convertSizei(58)), glm::vec3(1.f),
                1.f, 1.f, HorizontalAlign::CENTER, VerticalAlign::TOP));

    static RenderWorld renderWorlds[10];

    f32 columnOffset[] = {
        32, 60, 150, 350
    };

    u32 vehicleIconSize = (u32)g_gui.convertSize(48);
    Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("world.Quad");
    SmallArray<Driver*, 20> sortedDrivers;
    for(auto& driver : g_game.state.drivers)
    {
        sortedDrivers.push_back(&driver);
    }
    std::sort(sortedDrivers.begin(), sortedDrivers.end(), [](Driver* a, Driver* b) {
        return a->leaguePoints > b->leaguePoints;
    });

    for (u32 i=0; i<sortedDrivers.size(); ++i)
    {
        Driver* driver = sortedDrivers[i];

        RenderWorld& rw = renderWorlds[i];
        rw.setName(tstr("Vehicle Icon ", i));
        rw.setSize(vehicleIconSize, vehicleIconSize);
        rw.push(LitRenderable(quadMesh,
                    glm::scale(glm::mat4(1.f), glm::vec3(20.f)),
                    nullptr, glm::vec3(0.02f)));
        if (driver->vehicleIndex != -1)
        {
            g_vehicles[driver->vehicleIndex]->render(&rw,
                glm::translate(glm::mat4(1.f),
                    glm::vec3(0, 0, driver->getTuning().getRestOffset())) *
                glm::rotate(glm::mat4(1.f), (f32)getTime(), glm::vec3(0, 0, 1)),
                nullptr, *driver->getVehicleConfig());
        }
        rw.setViewportCount(1);
        rw.addDirectionalLight(glm::vec3(-0.5f, 0.2f, -1.f), glm::vec3(1.0));
        rw.setViewportCamera(0, glm::vec3(8.f, -8.f, 10.f),
                glm::vec3(0.f, 0.f, 1.f), 1.f, 50.f, 28.f);
        g_game.renderer->addRenderWorld(&rw);

        glm::vec2 pos = menuPos + glm::vec2(0.f,
                        g_gui.convertSizei(100) + i * g_gui.convertSize(48));
        g_game.renderer->push2D(TextRenderable(smallfont, tstr(i + 1),
                    pos + glm::vec2(g_gui.convertSizei(columnOffset[0]), 0),
                    glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::LEFT, VerticalAlign::CENTER));

        g_game.renderer->push2D(QuadRenderable(rw.getTexture(),
                    pos + glm::vec2(g_gui.convertSizei(columnOffset[1]),
                        -glm::floor(vehicleIconSize/2)),
                    (f32)vehicleIconSize, (f32)vehicleIconSize, glm::vec3(1.f), 1.f, false, true));

        g_game.renderer->push2D(TextRenderable(smallfont, driver->playerName.c_str(),
                    pos + glm::vec2(g_gui.convertSizei(columnOffset[2]), 0),
                    glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::LEFT, VerticalAlign::CENTER));
        g_game.renderer->push2D(TextRenderable(smallfont, tstr(driver->leaguePoints),
                    pos + glm::vec2(g_gui.convertSizei(columnOffset[3]), 0),
                    glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::LEFT, VerticalAlign::CENTER));
    }

    if (g_gui.didSelect())
    {
        g_audio.playSound(g_res.getSound("close"), SoundType::MENU_SFX);
        menuMode = CHAMPIONSHIP_MENU;
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
        g_gui.popSelection();
    }
}

void Menu::raceResults()
{
    f32 w = g_gui.convertSizei(650);

    f32 cw = (f32)(g_game.windowWidth/2);
    glm::vec2 menuPos = glm::vec2(cw - w/2, glm::floor(g_game.windowHeight * 0.2f));
    glm::vec2 menuSize = glm::vec2(w, (f32)g_game.windowHeight * 0.6f);
    drawBox(menuPos, menuSize);

    Font* bigfont = &g_res.getFont("font_bold", (u32)g_gui.convertSize(26));
    Font* smallfont = &g_res.getFont("font", (u32)g_gui.convertSize(16));
    Font* tinyfont = &g_res.getFont("font", (u32)g_gui.convertSize(14));

    g_game.renderer->push2D(TextRenderable(bigfont, "Race Results",
                glm::vec2(cw, menuPos.y + g_gui.convertSizei(32)), glm::vec3(1.f),
                1.f, 1.f, HorizontalAlign::CENTER, VerticalAlign::TOP));

    Texture* white = &g_res.white;
    g_game.renderer->push2D(QuadRenderable(white,
                menuPos + glm::vec2(g_gui.convertSize(8), g_gui.convertSize(64)),
                w - g_gui.convertSize(16), g_gui.convertSize(19), glm::vec3(0.f), 0.6f));

    f32 columnOffset[] = {
        32, 90, 225, 315, 400, 460, 520
    };
    const char* columnTitle[] = {
        "NO.",
        "DRIVER",
        "ACCIDENTS",
        "DESTROYED",
        "FRAGS",
        "BONUS",
        "CREDITS EARNED"
    };

    u32 maxColumn = g_game.state.gameMode == GameMode::CHAMPIONSHIP
        ? ARRAY_SIZE(columnTitle) : ARRAY_SIZE(columnTitle) - 1;

    for (u32 i=0; i<maxColumn; ++i)
    {
        g_game.renderer->push2D(TextRenderable(tinyfont, columnTitle[i],
                    menuPos + glm::vec2(g_gui.convertSizei(columnOffset[i]), g_gui.convertSize(70)),
                    glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::LEFT, VerticalAlign::TOP));
    }

    for (auto& row : g_game.currentScene->getRaceResults())
    {
        glm::vec2 pos = menuPos + glm::vec2(0.f,
                        g_gui.convertSizei(100) + row.placement * g_gui.convertSize(24));
        g_game.renderer->push2D(TextRenderable(smallfont, tstr(row.placement + 1),
                    pos + glm::vec2(g_gui.convertSizei(columnOffset[0]), 0),
                    glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::LEFT, VerticalAlign::TOP));
        g_game.renderer->push2D(TextRenderable(smallfont, row.driver->playerName.c_str(),
                    pos + glm::vec2(g_gui.convertSizei(columnOffset[1]), 0),
                    glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::LEFT, VerticalAlign::TOP));
        g_game.renderer->push2D(TextRenderable(smallfont, tstr(row.statistics.accidents),
                    pos + glm::vec2(g_gui.convertSizei(columnOffset[2]), 0),
                    glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::LEFT, VerticalAlign::TOP));
        g_game.renderer->push2D(TextRenderable(smallfont, tstr(row.statistics.destroyed),
                    pos + glm::vec2(g_gui.convertSizei(columnOffset[3]), 0),
                    glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::LEFT, VerticalAlign::TOP));
        g_game.renderer->push2D(TextRenderable(smallfont, tstr(row.statistics.frags),
                    pos + glm::vec2(g_gui.convertSizei(columnOffset[4]), 0),
                    glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::LEFT, VerticalAlign::TOP));
        g_game.renderer->push2D(TextRenderable(smallfont, tstr(row.getBonus()),
                    pos + glm::vec2(g_gui.convertSizei(columnOffset[5]), 0),
                    glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::LEFT, VerticalAlign::TOP));
        if (g_game.state.gameMode == GameMode::CHAMPIONSHIP)
        {
            g_game.renderer->push2D(TextRenderable(smallfont, tstr(row.getCreditsEarned()),
                        pos + glm::vec2(g_gui.convertSizei(columnOffset[6]), 0),
                        glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::LEFT, VerticalAlign::TOP));
        }
    }

    if (g_gui.didSelect())
    {
        for (auto& r : g_game.currentScene->getRaceResults())
        {
            r.driver->lastPlacement = r.placement;
        }

        g_audio.playSound(g_res.getSound("close"), SoundType::MENU_SFX);
        if (g_game.state.gameMode == GameMode::CHAMPIONSHIP)
        {
            menuMode = CHAMPIONSHIP_STANDINGS;
            for (auto& row : g_game.currentScene->getRaceResults())
            {
                row.driver->credits += row.getCreditsEarned();
                row.driver->leaguePoints += row.getLeaguePointsEarned();
            }
            ++g_game.state.currentRace;
        }
        else
        {
            showMainMenu();
            g_game.currentScene->isCameraTourEnabled = true;
        }
    }
}

void Menu::showSettingsMenu()
{
    tmpConfig = g_game.config;
    reset();

    addTitle("Settings");
    glm::vec2 size(350, 150);
    selectedWidget = addButton("GRAPHICS", "", { -185, -180 }, size, [this]{
        showGraphicsSettingsMenu();
    }, WidgetFlags::FADE_OUT);
    addButton("AUDIO", "", { -185, -10 }, size, [this]{
        showAudioSettingsMenu();
    }, WidgetFlags::FADE_OUT);
    addButton("GAMEPLAY", "", { 185, -180 }, size, [this]{
        showGameplaySettingsMenu();
    }, WidgetFlags::FADE_OUT);
    addButton("CONTROLS", "", { 185, -10 }, size, [this]{
        showControlsSettingsMenu();
    }, WidgetFlags::FADE_OUT);
    addButton("BACK", "", { 0, 200 }, {290, 50}, [this]{
        showMainMenu();
    }, WidgetFlags::FADE_OUT);
}

void Menu::showGraphicsSettingsMenu()
{
    reset();
    addTitle("Graphics Settings");
    addBackgroundBox({0,0}, {880, 780});

    // TODO: detect aspect ratio and show resolutions accordingly
    static glm::ivec2 resolutions[] = {
        { 960, 540 },
        { 1024, 576 },
        { 1280, 720 },
        { 1366, 768 },
        { 1600, 900 },
        { 1920, 1080 },
        { 2560, 1440 },
        { 3840, 2160 },
    };
    SmallArray<std::string> resolutionNames;
    i32 valueIndex = 2;
    for (i32 i=0; i<(i32)ARRAY_SIZE(resolutions); ++i)
    {
        if (resolutions[i].x == (i32)tmpConfig.graphics.resolutionX &&
            resolutions[i].y == (i32)tmpConfig.graphics.resolutionY)
        {
            valueIndex = i;
        }
        resolutionNames.push_back(str(resolutions[i].x, " x ", resolutions[i].y));
    }

    glm::vec2 size(750, 60);
    selectedWidget = addSelector("Resolution", "The internal render resolution.", { 0, -300 },
        size, resolutionNames, valueIndex, [this](i32 valueIndex){
            tmpConfig.graphics.resolutionX = resolutions[valueIndex].x;
            tmpConfig.graphics.resolutionY = resolutions[valueIndex].y;
        });
    addSelector("Fullscreen", "Run the game in fullscreen or window.", { 0, -230 }, size, { "OFF", "ON" }, tmpConfig.graphics.fullscreen,
            [this](i32 valueIndex){ tmpConfig.graphics.fullscreen = (bool)valueIndex; });
    addSelector("V-Sync", "Lock the frame rate to the monitor's refresh rate.",
            { 0, -160 }, size, { "OFF", "ON" }, tmpConfig.graphics.vsync,
            [this](i32 valueIndex){ tmpConfig.graphics.vsync = (bool)valueIndex; });

    /*
    f32 maxFPS = tmpConfig.graphics.maxFPS;
    g_gui.slider("Max FPS", 30, 300, maxFPS);
    tmpConfig.graphics.maxFPS = (u32)maxFPS;
    */

    static u32 shadowMapResolutions[] = { 0, 1024, 2048, 4096 };
    SmallArray<std::string> shadowQualityNames = { "Off", "Low", "Medium", "High" };
    i32 shadowQualityIndex = 0;
    for (i32 i=0; i<(i32)ARRAY_SIZE(shadowMapResolutions); ++i)
    {
        if (shadowMapResolutions[i] == tmpConfig.graphics.shadowMapResolution)
        {
            shadowQualityIndex = i;
            break;
        }
    }
    addSelector("Shadows", "The quality of shadows cast by objects in the world.", { 0, -90 },
        size, shadowQualityNames, shadowQualityIndex, [this](i32 valueIndex){
            tmpConfig.graphics.shadowsEnabled = valueIndex > 0;
            tmpConfig.graphics.shadowMapResolution = shadowMapResolutions[valueIndex];
        });

    SmallArray<std::string> ssaoQualityNames = { "Off", "Normal", "High" };
    i32 ssaoQualityIndex = 0;
    if (tmpConfig.graphics.ssaoEnabled)
    {
        ssaoQualityIndex = 1;
        if (tmpConfig.graphics.ssaoHighQuality)
        {
            ssaoQualityIndex = 2;
        }
    }

    addSelector("SSAO", "Darkens occluded areas of the world for improved realism.", { 0, -20 }, size, ssaoQualityNames,
        ssaoQualityIndex, [this](i32 valueIndex){
            tmpConfig.graphics.ssaoEnabled = valueIndex > 0;
            tmpConfig.graphics.ssaoHighQuality = valueIndex == 2;
        });

    addSelector("Bloom", "Adds a glowy light-bleeding effect to bright areas of the world.", { 0, 50 }, size, { "OFF", "ON" }, tmpConfig.graphics.bloomEnabled,
            [this](i32 valueIndex){ tmpConfig.graphics.bloomEnabled = (bool)valueIndex; });

    i32 aaIndex = 0;
    static u32 aaLevels[] = { 0, 2, 4, 8 };
    SmallArray<std::string> aaLevelNames = { "Off", "2x MSAA", "4x MSAA", "8x MSAA" };
    for (i32 i=0; i<(i32)ARRAY_SIZE(aaLevels); ++i)
    {
        if (aaLevels[i] == tmpConfig.graphics.msaaLevel)
        {
            aaIndex = i;
            break;
        }
    }
    addSelector("Anti-Aliasing", "Improves image quality by smoothing jagged edges.", { 0, 120 }, size, aaLevelNames,
        aaIndex, [this](i32 valueIndex){
            tmpConfig.graphics.msaaLevel = aaLevels[valueIndex];
        });

    addHelpMessage({0, 230});

    glm::vec2 buttonSize(280, 60);
    addButton("APPLY", nullptr, { -290, 350 }, buttonSize, [this]{
        g_game.config = tmpConfig;
        g_game.renderer->updateSettingsVersion();
        SDL_SetWindowFullscreen(g_game.window, g_game.config.graphics.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        SDL_GL_SetSwapInterval(g_game.config.graphics.vsync ? 1 : 0);
        g_game.config.save();
        showSettingsMenu();
    }, WidgetFlags::FADE_OUT);

    addButton("DEFAULTS", nullptr, { 0, 350 }, buttonSize, [this]{
        Config defaultConfig;
        tmpConfig.graphics = defaultConfig.graphics;
        Widget* previousSelectedWidget = selectedWidget;
        showGraphicsSettingsMenu();
        selectedWidget = previousSelectedWidget;
        for (auto& w : widgets)
        {
            w.fadeInAlpha = 1.f;
            w.fadeInScale = 1.f;
            fadeInTimer = 1920.f;
        }
    }, 0);

    addButton("CANCEL", nullptr, { 290, 350 }, buttonSize, [this]{
        showSettingsMenu();
    }, WidgetFlags::FADE_OUT | WidgetFlags::BACK);
}

void Menu::showAudioSettingsMenu()
{
    reset();
    addTitle("Audio Settings");
    selectedWidget = addButton("BACK", "", { 0, 200 }, {290, 50}, [this]{
        showSettingsMenu();
    }, WidgetFlags::FADE_OUT | WidgetFlags::BACK);
}

void Menu::showGameplaySettingsMenu()
{
    reset();
    addTitle("Gameplay Settings");
    selectedWidget = addButton("BACK", "", { 0, 200 }, {290, 50}, [this]{
        showSettingsMenu();
    }, WidgetFlags::FADE_OUT | WidgetFlags::BACK);
}

void Menu::showControlsSettingsMenu()
{
    reset();
    addTitle("Control Settings");
    selectedWidget = addButton("BACK", "", { 0, 200 }, {290, 50}, [this]{
        showSettingsMenu();
    }, WidgetFlags::FADE_OUT | WidgetFlags::BACK);
}

#if 0
void Menu::audioOptions()
{
    g_gui.beginPanel("Audio Options", { g_game.windowWidth/2, g_game.windowHeight*0.15f },
            0.5f, false, true);

    g_gui.slider("Master Volume", 0.f, 1.f, tmpConfig.audio.masterVolume);
    g_gui.slider("Vehicle Volume", 0.f, 1.f, tmpConfig.audio.vehicleVolume);
    g_gui.slider("SFX Volume", 0.f, 1.f, tmpConfig.audio.sfxVolume);
    g_gui.slider("Music Volume", 0.f, 1.f, tmpConfig.audio.musicVolume);

    g_gui.gap(20);
    if (g_gui.button("Save"))
    {
        g_game.config.audio = tmpConfig.audio;
        g_game.config.save();
        showMainMenu();
        g_gui.popSelection();
    }

    if (g_gui.button("Reset to Defaults"))
    {
        Config defaultConfig;
        tmpConfig.audio = defaultConfig.audio;
    }

    if (g_gui.button("Cancel") || g_gui.didGoBack())
    {
        showSettingsMenu();
        g_gui.popSelection();
    }

    g_gui.end();
}

void Menu::gameplayOptions()
{
    g_gui.beginPanel("Gameplay Options", { g_game.windowWidth/2, g_game.windowHeight*0.15f },
            0.5f, false, true);

    g_gui.slider("HUD Track Scale", 0.5f, 2.f, tmpConfig.gameplay.hudTrackScale);
    g_gui.slider("HUD Text Scale", 0.5f, 2.f, tmpConfig.gameplay.hudTextScale);

    g_gui.gap(20);
    if (g_gui.button("Save"))
    {
        g_game.config.gameplay = tmpConfig.gameplay;
        g_game.config.save();
        showMainMenu();
        g_gui.popSelection();
    }

    if (g_gui.button("Reset to Defaults"))
    {
        Config defaultConfig;
        tmpConfig.gameplay = defaultConfig.gameplay;
    }

    if (g_gui.button("Cancel") || g_gui.didGoBack())
    {
        showSettingsMenu();
        g_gui.popSelection();
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
    SmallArray<std::string> resolutionNames;
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

    g_gui.gap(20);
    if (g_gui.button("Save"))
    {
        g_game.config = tmpConfig;
        g_game.renderer->updateSettingsVersion();
        SDL_SetWindowFullscreen(g_game.window, g_game.config.graphics.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        SDL_GL_SetSwapInterval(g_game.config.graphics.vsync ? 1 : 0);
        g_game.config.save();
        showMainMenu();
        g_gui.popSelection();
    }

    if (g_gui.button("Reset to Defaults"))
    {
        Config defaultConfig;
        tmpConfig.graphics = defaultConfig.graphics;
    }

    if (g_gui.button("Cancel") || g_gui.didGoBack())
    {
        showSettingsMenu();
        g_gui.popSelection();
    }

    g_gui.end();
}
#endif

void Menu::showPauseMenu()
{
    reset();
    menuMode = MenuMode::PAUSE_MENU;
    fadeInTimer = 400.f;

    addBackgroundBox({0,0}, {400, 400}, 0.8f);
    addLogic([this]{
        bool unpause = g_input.isKeyPressed(KEY_ESCAPE);
        for (auto& c : g_input.getControllers())
        {
            unpause |= c.second.isButtonPressed(BUTTON_START);
        }
        if (fadeInTimer > 800.f && unpause)
        {
            menuMode = MenuMode::HIDDEN;
            g_game.currentScene->setPaused(false);
        }
    });

    Font* font = &g_res.getFont("font_bold", (u32)convertSize(55));
    addLabel("PAUSED", {0, -160}, 0, font);
    glm::vec2 size(350, 80);
    selectedWidget = addButton("Resume", "", {0, -70}, size, [this]{
        menuMode = MenuMode::HIDDEN;
        g_game.currentScene->setPaused(false);
    });
    addButton("Forfeit Race", "", {0, 30}, size, []{
        g_game.currentScene->setPaused(false);
        g_game.currentScene->forfeitRace();
    });
    addButton("Quit to Desktop", "", {0, 130}, size, []{
        g_game.shouldExit = true;
    });
}

void Menu::drawBox(glm::vec2 pos, glm::vec2 size)
{
    f32 border = glm::floor(g_gui.convertSize(3));
    g_game.renderer->push2D(QuadRenderable(&g_res.white,
                pos - border, size.x + border * 2, size.y + border * 2,
                glm::vec3(1.f), 0.4f, false));
    g_game.renderer->push2D(QuadRenderable(&g_res.white,
                pos, size.x, size.y, glm::vec3(0.f), 0.8f, true));
}

void Menu::onUpdate(Renderer* renderer, f32 deltaTime)
{
    if (menuMode == MenuMode::VISIBLE)
    {
        Texture* tex = g_res.getTexture("checkers_fade");
        f32 w = (f32)g_game.windowWidth;
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
        case MenuMode::VISIBLE:
        case MenuMode::PAUSE_MENU:
            break;
        case MenuMode::CHAMPIONSHIP_MENU:
            championshipMenu();
            break;
        case MenuMode::CHAMPIONSHIP_GARAGE:
            championshipGarage();
            break;
        case MenuMode::CHAMPIONSHIP_STANDINGS:
            championshipStandings();
            break;
        case MenuMode::RACE_RESULTS:
            raceResults();
            break;
    }

    if (menuMode == MenuMode::HIDDEN || widgets.empty())
    {
        return;
    }

    repeatTimer = glm::max(repeatTimer - g_game.deltaTime, 0.f);

    glm::vec2 mousePos = g_input.getMousePosition();
    glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
    const char* helpText = "";
    Widget* previousSelectedWidget = selectedWidget;

    Widget* activatedWidget = nullptr;

    for (auto& widget : widgets)
    {
        if (widget.flags & WidgetFlags::DISABLED)
        {
            continue;
        }

        bool activated = false;
        if (widget.flags & WidgetFlags::BACK && didGoBack())
        {
            activated = true;
        }

        glm::vec2 size = convertSize(widget.size);
        glm::vec2 pos = convertSize(widget.pos) + center - size * 0.5f;
        bool isHovered = pointInRectangle(mousePos, pos, pos + size);
        if (fadeIn && (widget.flags & WidgetFlags::SELECTABLE))
        {
            if (isHovered)
            {
                if (g_input.didMouseMove())
                {
                    selectedWidget = &widget;
                }
                if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                {
                    activated = true;
                }
            }
        }
        bool isSelected = selectedWidget == &widget;
        widget.hover = smoothMove(widget.hover,
                isSelected ? 1.f : 0.f, 8.f, deltaTime);
        if (isSelected)
        {
            widget.hoverTimer += deltaTime;
            helpText = widget.helpText;
            if (didSelect() && fadeIn)
            {
                activated = true;
            }
        }

        if (activated)
        {
            g_audio.playSound(g_res.getSound("click"), SoundType::MENU_SFX);
            if (widget.flags & WidgetFlags::FADE_OUT)
            {
                fadeIn = false;
                fadeInTimer = REFERENCE_HEIGHT;
            }
            else
            {
                activatedWidget = &widget;
            }
        }

        widget.onRender(widget, isSelected);

        if (fadeIn)
        {
            if (fadeInTimer > widget.pos.y + REFERENCE_HEIGHT * 0.5f)
            {
                widget.fadeInAlpha = smoothMoveSnap(widget.fadeInAlpha, 1.f, 10.f, deltaTime, 0.002f);
                widget.fadeInScale = smoothMoveSnap(widget.fadeInScale, 1.f, 10.f, deltaTime, 0.002f);
            }
        }
        else
        {
            if (fadeInTimer < widget.pos.y + REFERENCE_HEIGHT * 0.5f)
            {
                widget.fadeInAlpha = smoothMove(widget.fadeInAlpha, 0.f, 7.f, deltaTime);
                widget.fadeInAlpha = clamp(widget.fadeInAlpha - deltaTime * 3.f, 0.f, 1.f);
                widget.fadeInScale = smoothMove(widget.fadeInScale, 0.f, 5.f, deltaTime);
            }
        }
    }

    if (blackFadeAlpha > 0.f)
    {
        g_game.renderer->push2D(Quad(&g_res.white, {0,0}, g_game.windowWidth, g_game.windowHeight,
                    glm::vec4(0,0,0,1), blackFadeAlpha), 100);
    }

    if (fadeIn)
    {
        fadeInTimer += deltaTime * 2100.f;
        blackFadeAlpha = clamp(blackFadeAlpha-deltaTime*5.f, 0.f, 1.f);
    }
    else
    {
        fadeInTimer -= deltaTime * 2100.f;
        if ((selectedWidget->flags & WidgetFlags::FADE_TO_BLACK) && fadeInTimer < REFERENCE_HEIGHT * 0.35f)
        {
            blackFadeAlpha = clamp(blackFadeAlpha+deltaTime*5.f, 0.f, 1.f);
        }
        if (fadeInTimer < -100.f)
        {
            activatedWidget = selectedWidget;
        }
    }

    if (activatedWidget)
    {
        activatedWidget->onSelect();
    }
    else if (fadeIn)
    {
        i32 selectDirX = (selectedWidget->flags & WidgetFlags::NAVIGATE_HORIZONTAL) ?
            didChangeSelectionX() : 0;
        i32 selectDirY = (selectedWidget->flags & WidgetFlags::NAVIGATE_VERTICAL) ?
            didChangeSelectionY() : 0;
        f32 xRef = selectedWidget->pos.x + (selectedWidget->size.x * selectDirX) * 0.5f;
        f32 yRef = selectedWidget->pos.y + (selectedWidget->size.y * selectDirY) * 0.5f;
        Widget* minSelectTargetX = nullptr;
        Widget* minSelectTargetY = nullptr;
        Widget* maxSelectTargetX = nullptr;
        Widget* maxSelectTargetY = nullptr;
        f32 minDistX = FLT_MAX;
        f32 minDistY = FLT_MAX;
        f32 maxDistX = FLT_MIN;
        f32 maxDistY = FLT_MIN;
        if (selectDirX || selectDirY)
        {
            for (auto& widget : widgets)
            {
                if (&widget == selectedWidget
                        || !(widget.flags & WidgetFlags::SELECTABLE)
                        || (widget.flags & WidgetFlags::DISABLED))
                {
                    continue;
                }

                if (selectDirX)
                {
                    f32 xDist = selectDirX * ((widget.pos.x - (widget.size.x * selectDirX) * 0.5f) - xRef);
                    f32 yDist = glm::abs((widget.pos.y + widget.size.y * 0.5f)
                                - (selectedWidget->pos.y + selectedWidget->size.y * 0.5f)) * 0.5f;
                    if (xDist > 0.f && xDist + yDist < minDistX)
                    {
                        minDistX = xDist + yDist;
                        minSelectTargetX = &widget;
                    }
                    if (xDist + yDist < maxDistX)
                    {
                        maxDistX = xDist + yDist;
                        maxSelectTargetX = &widget;
                    }
                }
                if (selectDirY)
                {
                    f32 yDist = selectDirY * ((widget.pos.y - (widget.size.y * selectDirY) * 0.5f) - yRef);
                    f32 xDist = glm::abs((widget.pos.x + widget.size.x * 0.5f)
                                - (selectedWidget->pos.x + selectedWidget->size.x * 0.5f)) * 0.5f;
                    if (yDist > 0.f && yDist + xDist < minDistY)
                    {
                        minDistY = yDist + xDist;
                        minSelectTargetY = &widget;
                    }
                    if (yDist + xDist < maxDistY)
                    {
                        maxDistY = yDist + xDist;
                        maxSelectTargetY = &widget;
                    }
                }
            }
        }
        if (minSelectTargetX)
        {
            selectedWidget = minSelectTargetX;
        }
        else if (maxSelectTargetX)
        {
            selectedWidget = maxSelectTargetX;
        }
        if (minSelectTargetY)
        {
            selectedWidget = minSelectTargetY;
        }
        else if (maxSelectTargetY)
        {
            selectedWidget = maxSelectTargetY;
        }

        if (selectedWidget != previousSelectedWidget)
        {
            selectedWidget->hoverTimer = 0.f;
            g_audio.playSound(g_res.getSound("select"), SoundType::MENU_SFX);
        }
    }
}
