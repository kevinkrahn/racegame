#include "menu.h"
#include "resources.h"
#include "renderer.h"
#include "game.h"
#include "2d.h"
#include "input.h"
#include "scene.h"
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
                    (driverIndexOffset + i) % g_ais.size()));
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
    reset();
}

constexpr f32 REFERENCE_HEIGHT = 1080.f;
f32 convertSize(f32 size)
{
    return glm::floor(size * (g_game.windowHeight / REFERENCE_HEIGHT));
}

// TODO: remove
f32 convertSize720(f32 size)
{
    return size * (g_game.windowHeight / 720.f);
}

// TODO: remove
f32 convertSize720i(f32 size)
{
    return glm::floor(size * (g_game.windowHeight / 720.f));
}

glm::vec2 convertSize(glm::vec2 size)
{
    return glm::vec2(convertSize(size.x), convertSize(size.y));
}

i32 Menu::didChangeSelectionX()
{
    i32 result = (i32)(g_input.isKeyPressed(KEY_RIGHT, true) || didSelect())
                - (i32)g_input.isKeyPressed(KEY_LEFT, true);

    for (auto& pair : g_input.getControllers())
    {
        i32 tmpResult = pair.value.isButtonPressed(BUTTON_DPAD_RIGHT) -
                        pair.value.isButtonPressed(BUTTON_DPAD_LEFT);
        if (!tmpResult)
        {
            if (repeatTimer == 0.f)
            {
                f32 xaxis = pair.value.getAxis(AXIS_LEFT_X);
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
        i32 tmpResult = pair.value.isButtonPressed(BUTTON_DPAD_DOWN) -
                        pair.value.isButtonPressed(BUTTON_DPAD_UP);
        if (!tmpResult)
        {
            if (repeatTimer == 0.f)
            {
                f32 yaxis = pair.value.getAxis(AXIS_LEFT_Y);
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

f32 didMoveX()
{
    f32 result = (f32)g_input.isKeyDown(KEY_RIGHT) - (f32)g_input.isKeyDown(KEY_LEFT);
    for (auto& pair : g_input.getControllers())
    {
        result += (f32)pair.value.isButtonDown(BUTTON_DPAD_RIGHT) -
                  (f32)pair.value.isButtonDown(BUTTON_DPAD_LEFT);
        result += pair.value.getAxis(AXIS_LEFT_X);
    }
    return result;
}

Widget* Menu::addBackgroundBox(glm::vec2 pos, glm::vec2 size, f32 alpha, bool scaleOut)
{
    Widget w;
    w.pos = pos;
    w.size = size;
    w.fadeInScale = 1.f;
    w.onRender = [alpha, scaleOut](Widget& w, bool){
        glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        f32 scale = scaleOut ? w.fadeInScale : 1.f;
        glm::vec2 size = convertSize(w.size) * scale;
        glm::vec2 pos = convertSize(w.pos) + center - size * 0.5f;
        ui::rectBlur(-1000, nullptr, pos, size, glm::vec4(glm::vec3(0.f), alpha), w.fadeInAlpha);
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

Widget* Menu::addLogic(std::function<void(Widget&)> onUpdate)
{
    Widget w;
    w.flags = 0;
    w.onRender = [onUpdate](Widget& w, bool isSelected){ onUpdate(w); };
    widgets.push_back(w);
    return &widgets.back();
}

void drawSelectableBox(Widget& w, bool isSelected, bool isEnabled,
        glm::vec4 color=glm::vec4(0,0,0,0.8f), glm::vec4 unselectedBorderColor=COLOR_NOT_SELECTED)
{
    f32 borderSize = convertSize(isSelected ? 5 : 2);
    glm::vec4 borderColor = glm::mix(unselectedBorderColor, COLOR_SELECTED, w.hover);
    glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
    glm::vec2 size = convertSize(w.size) * w.fadeInScale;
    glm::vec2 pos = convertSize(w.pos) + center - size * 0.5f;
    ui::rectBlur(ui::BORDER, &g_res.white,
                pos - borderSize, size + borderSize * 2,
                borderColor, (0.5f + w.hover * 0.5f) * w.fadeInAlpha);
    if (!isEnabled)
    {
        color.a = 0.4f;
    }
    glm::vec3 col = glm::mix(glm::vec3(color), glm::vec3(1.f),
            (sinf(w.hoverTimer * 4.f) + 1.f)*0.5f*w.hover*0.04f);
    ui::rectBlur(ui::BG, &g_res.white, pos, size,
            glm::vec4(col, color.a), w.fadeInAlpha);
}

Widget* Menu::addButton(const char* text, const char* helpText, glm::vec2 pos, glm::vec2 size,
        std::function<void()> onSelect, u32 flags, Texture* image, std::function<bool()> isEnabled)
{
    Font* font = &g_res.getFont("font", (u32)convertSize(38));

    Widget button;
    button.helpText = helpText;
    button.pos = pos;
    button.size = size;
    button.onSelect = std::move(onSelect);
    button.onRender = [image, font, text, isEnabled](Widget& btn, bool isSelected){
        bool enabled = isEnabled();
        drawSelectableBox(btn, isSelected, enabled);
        glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        glm::vec2 size = convertSize(btn.size) * btn.fadeInScale;
        glm::vec2 pos = convertSize(btn.pos) + center - size * 0.5f;
        if (enabled)
        {
            btn.flags = btn.flags & ~WidgetFlags::CANNOT_ACTIVATE;
        }
        else
        {
            btn.flags |= WidgetFlags::CANNOT_ACTIVATE;
        }
        if (image)
        {
            f32 imageSize = convertSize(64) * btn.fadeInScale;
            f32 textOffsetX = imageSize + convertSize(40);
#if 1
            ui::text(font, text, pos + glm::vec2(textOffsetX, size.y * 0.5f),
                        glm::vec3(1.f), (isSelected ? 1.f : 0.5f) * btn.fadeInAlpha, btn.fadeInScale,
                        HAlign::LEFT, VAlign::CENTER);
#else
            ui::text(font, text, pos + size * 0.5f,
                        glm::vec3(1.f), (isSelected ? 1.f : 0.5f) * btn.fadeInAlpha, btn.fadeInScale,
                        HAlign::CENTER, VAlign::CENTER));
#endif
            ui::rectBlur(ui::IMAGE, image, pos + glm::vec2(convertSize(20), size.y*0.5f-imageSize*0.5f),
                    glm::vec2(imageSize), glm::vec4(1.f), btn.fadeInAlpha);
        }
        else
        {
            f32 alpha = (enabled ? (isSelected ? 1.f : 0.5f) : 0.25f) * btn.fadeInAlpha;
            ui::text(font, text, pos + size * 0.5f,
                    glm::vec3(1.f), alpha, btn.fadeInScale, HAlign::CENTER, VAlign::CENTER);
        }
    };
    button.fadeInScale = 0.7f;
    button.flags = WidgetFlags::SELECTABLE |
                   WidgetFlags::NAVIGATE_VERTICAL |
                   WidgetFlags::NAVIGATE_HORIZONTAL | flags;
    widgets.push_back(button);
    return &widgets.back();
}

Widget* Menu::addImageButton(const char* text, const char* helpText, glm::vec2 pos, glm::vec2 size,
        std::function<void()> onSelect, u32 flags, Texture* image, f32 imageMargin,
        std::function<ImageButtonInfo(bool isSelected)> getInfo)
{
    Font* font = &g_res.getFont("font", (u32)convertSize(22));

    Widget button;
    button.helpText = helpText;
    button.pos = pos;
    button.size = size;
    button.onSelect = std::move(onSelect);
    button.onRender = [=](Widget& btn, bool isSelected){
        auto info = getInfo(isSelected);
#if 0
        drawSelectableBox(btn, isSelected, info.isEnabled, {0,0,0,0.8f},
                info.isHighlighted ? glm::vec4(0.05f,0.05f,1.f,1.f) : COLOR_NOT_SELECTED);
#else
        drawSelectableBox(btn, isSelected, info.isEnabled, {0,0,0,0.8f},
                info.isHighlighted ? COLOR_SELECTED : COLOR_NOT_SELECTED);
#endif
        glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        glm::vec2 imageSize = convertSize(btn.size - imageMargin) * btn.fadeInScale;
        glm::vec2 pos = convertSize(btn.pos) + center - imageSize * 0.5f;
        f32 textOffsetY1 = convertSize(btn.pos.y) + center.y - (convertSize(btn.size.y) * 0.5f
            - convertSize(16)) * btn.fadeInScale;
        f32 textOffsetY2 = convertSize(btn.pos.y) + center.y + (convertSize(btn.size.y) * 0.5f
            - convertSize(16)) * btn.fadeInScale;

        f32 alpha = (info.isEnabled ? (isSelected ? 1.f : 0.8f) : 0.25f) * btn.fadeInAlpha;
        if (info.flipImage)
        {
            ui::rectUVBlur(ui::IMAGE, image, pos, imageSize, {1,1}, {0,0}, glm::vec4(1.f), alpha);
        }
        else
        {
            ui::rectBlur(ui::IMAGE, image, pos, imageSize, glm::vec4(1.f), alpha);
        }

        ui::text(font, text, {pos.x + imageSize.x*0.5f, textOffsetY1},
                    glm::vec3(1.f), alpha, btn.fadeInScale, HAlign::CENTER, VAlign::CENTER);

        if (info.maxUpgradeLevel == 0 || info.upgradeLevel < info.maxUpgradeLevel)
        {
            ui::text(font, info.bottomText, {pos.x + imageSize.x*0.5f, textOffsetY2},
                    glm::vec3(1.f), alpha, btn.fadeInScale, HAlign::CENTER, VAlign::CENTER);
        }

        if (info.maxUpgradeLevel > 0)
        {
            Texture* tex1 = g_res.getTexture("button");
            Texture* tex2 = g_res.getTexture("button_glow");
            f32 size = convertSize(16);
            f32 sep = convertSize(20);
            for (i32 i=0; i<info.maxUpgradeLevel; ++i)
            {
                glm::vec2 pos = center + convertSize(btn.pos) - (convertSize({btn.size.x, 0}) * 0.5f
                    - glm::vec2(0, sep * info.maxUpgradeLevel * 0.5f) + glm::vec2(0, sep * i)
                    - convertSize({ 12, -10 })) * btn.fadeInScale;
                ui::rectBlur(ui::IMAGE, tex1, pos - size*0.5f * btn.fadeInScale,
                            glm::vec2(size * btn.fadeInScale), glm::vec4(1.f), btn.fadeInAlpha);
            }
            for (i32 i=0; i<info.upgradeLevel; ++i)
            {
                glm::vec2 pos = center + convertSize(btn.pos) - (convertSize({btn.size.x, 0}) * 0.5f
                    - glm::vec2(0, sep * info.maxUpgradeLevel * 0.5f) + glm::vec2(0, sep * i)
                    - convertSize({ 12, -10 })) * btn.fadeInScale;
                ui::rectBlur(ui::IMAGE, tex2, pos - size*0.5f * btn.fadeInScale,
                            glm::vec2(size * btn.fadeInScale), glm::vec4(1.f), btn.fadeInAlpha);
            }
        }
    };
    button.fadeInScale = 0.7f;
    button.flags = WidgetFlags::SELECTABLE |
                   WidgetFlags::NAVIGATE_VERTICAL |
                   WidgetFlags::NAVIGATE_HORIZONTAL | flags;
    widgets.push_back(button);
    return &widgets.back();
}

Widget* Menu::addSlider(const char* text, glm::vec2 pos, glm::vec2 size, u32 flags,
            std::function<void(f32)> onValueChanged, std::function<SliderInfo()> getInfo)
{
    Font* font = &g_res.getFont("font", (u32)convertSize(28));

    Widget w;
    w.pos = pos;
    w.size = size;
    w.flags = WidgetFlags::SELECTABLE | WidgetFlags::NAVIGATE_VERTICAL | flags;
    w.onSelect = []{};
    w.onRender = [=](Widget& w, bool isSelected) {
        drawSelectableBox(w, isSelected, true, {0,0,0,0.5f});

        glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        glm::vec2 size = convertSize({w.size.x, w.size.y * 0.5f}) * w.fadeInScale;
        glm::vec2 pos = convertSize(w.pos) + center - glm::vec2(size.x * 0.5f, 0.f);

        auto info = getInfo();

        f32 alpha = (isSelected ? 1.f : 0.6f) * w.fadeInAlpha;
        ui::text(font, text, pos + glm::vec2(size.x * 0.5f, -convertSize(8)),
                    glm::vec3(1.f), alpha, w.fadeInScale, HAlign::CENTER, VAlign::BOTTOM);
        ui::rectBlur(ui::IMAGE, info.tex, pos, size,
                glm::vec4(info.color1, 1.f), glm::vec4(info.color2, 1.f), w.fadeInAlpha);

        if (isSelected)
        {
            f32 dir = didMoveX();
            if (dir != 0.f)
            {
                f32 t = (info.val - info.min) / (info.max - info.min);
                t = clamp(t + dir * g_game.realDeltaTime * 0.5f, 0.f, 1.f);
                onValueChanged(glm::lerp(info.min, info.max, t));
            }
            if (g_input.isMouseButtonDown(MOUSE_LEFT))
            {
                glm::vec2 mousePos = g_input.getMousePosition();
                f32 localX = mousePos.x - pos.x;
                f32 t = clamp(localX / size.x, 0.f, 1.f);
                onValueChanged(glm::lerp(info.min, info.max, t));
            }
        }

        f32 t = (info.val - info.min) / (info.max - info.min);
        ui::rectBlur(ui::IMAGE, &g_res.white, pos + glm::vec2(t * size.x, 0.f),
                {convertSize(4), size.y}, glm::vec4(0.1f, 0.1f, 0.1f, 1.f), w.fadeInAlpha);
    };
    w.fadeInScale = 0.7f;
    widgets.push_back(w);
    return &widgets.back();
}

Widget* Menu::addSelector(const char* text, const char* helpText, glm::vec2 pos, glm::vec2 size,
        SmallArray<std::string> values, i32 valueIndex,
        std::function<void(i32 valueIndex)> onValueChanged)
{
    Font* font = &g_res.getFont("font", (u32)convertSize(34));

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
            ui::rectBlur(ui::BORDER, &g_res.white, pos - borderSize, size + borderSize * 2,
                        borderColor, (0.5f + w.hover * 0.5f) * w.fadeInAlpha);
        }
        ui::rectBlur(ui::BG, &g_res.white, pos, size,
                glm::vec4(glm::vec3((sinf(w.hoverTimer * 4.f) + 1.f)*0.5f*w.hover*0.04f), 0.8f),
                w.fadeInAlpha);
        ui::text(font, text,
                    pos + glm::vec2(convertSize(20.f), size.y * 0.5f),
                    glm::vec3(1.f), (isSelected ? 1.f : 0.5f) * w.fadeInAlpha, w.fadeInScale,
                    HAlign::LEFT, VAlign::CENTER);
        ui::text(font, tstr(values[valueIndex]),
                    pos + glm::vec2(size.x * 0.75f, size.y * 0.5f),
                    glm::vec3(1.f), (isSelected ? 1.f : 0.5f) * w.fadeInAlpha, w.fadeInScale,
                    HAlign::CENTER, VAlign::CENTER);

        if (isSelected)
        {
            Texture* cheveron = g_res.getTexture("cheveron");
            f32 cheveronSize = convertSize(36);
            f32 offset = convertSize(150.f);
            ui::rectBlur(ui::ICON, cheveron,
                    pos + glm::vec2(size.x*0.75f - offset - cheveronSize, size.y*0.5f - cheveronSize*0.5f),
                    glm::vec2(cheveronSize), glm::vec4(1.f), w.fadeInAlpha * w.hover);
            ui::rectUVBlur(ui::ICON, cheveron,
                    pos + glm::vec2(size.x*0.75f + offset, size.y*0.5f - cheveronSize*0.5f),
                    glm::vec2(cheveronSize), {1,0}, {0,1}, glm::vec4(1.f), w.fadeInAlpha * w.hover);

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
            glm::vec2 textSize = font->stringDimensions(selectedWidget->helpText) + convertSize({50,30});
            glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
            glm::vec2 size = textSize * widget.fadeInScale;
            glm::vec2 pos = convertSize(widget.pos) + center - size * 0.5f;
            f32 alpha = selectedWidget->hover * widget.fadeInAlpha;
            ui::rectBlur(ui::BG, &g_res.white, pos, size, glm::vec4(0,0,0,0.5f), alpha);
            ui::text(font, selectedWidget->helpText,
                        pos + size * 0.5f, glm::vec3(1.f), alpha, widget.fadeInScale,
                        HAlign::CENTER, VAlign::CENTER);
        }
    };
    w.flags = 0;
    widgets.push_back(w);
    return &widgets.back();
}

Widget* Menu::addLabel(std::function<const char*()> getText, glm::vec2 pos, Font* font,
        HAlign halign, VAlign valign, glm::vec3 const& color, u32 flags)
{
    Widget w;
    w.pos = pos;
    w.onRender = [=](Widget& widget, bool isSelected) {
        glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        ui::text(font, getText(), center + convertSize(w.pos), color,
                    widget.fadeInAlpha, widget.fadeInScale, halign, valign);
    };
    w.flags = flags;
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
        ui::rectBlur(-500, &g_res.white,
                pos-boxSize*0.5f, boxSize, glm::vec4(0,0,0,0.5f), widget.fadeInAlpha);
        ui::text(font, text, pos, glm::vec3(1.f),
                widget.fadeInAlpha, widget.fadeInScale, HAlign::CENTER, VAlign::CENTER);
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
        showChampionshipMenu();
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
        reset();
        g_game.loadEditor();
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

    addLabel([]{ return "Press SPACE or controller button to join"; }, { 0, -320 },
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
            g_game.state.drivers.push_back(Driver(false, false, false, -1, i, i));
        }
        RandomSeries series = randomSeed();
        for (auto& driver : g_game.state.drivers)
        {
            if (!driver.isPlayer)
            {
                driver.aiUpgrades(series);
            }
        }
        garage.playerIndex = 0;
        showInitialCarLotMenu(garage.playerIndex);
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
                ui::rectBlur(ui::BORDER, &g_res.white,
                            pos - borderSize, size + borderSize * 2,
                            borderColor, (0.5f + btn.hover * 0.5f) * btn.fadeInAlpha);
                ui::rectBlur(ui::BORDER, &g_res.white, pos, size,
                            glm::vec4(glm::vec3((sinf(btn.hoverTimer * 4.f) + 1.f)*0.5f*btn.hover*0.04f), 0.8f),
                            btn.fadeInAlpha);

                Font* font = &g_res.getFont("font", (u32)convertSize(38));
                const char* text = tstr(g_game.state.drivers[i].playerName);
                f32 textAlpha = isSelected ? 1.f : 0.5f;
                ui::text(font, text,
                            pos + glm::vec2(convertSize(20), size.y * 0.5f),
                            glm::vec3(1.f), textAlpha * btn.fadeInAlpha, btn.fadeInScale,
                            HAlign::LEFT, VAlign::CENTER);

                const char* icon = g_game.state.drivers[i].useKeyboard
                        ? "icon_keyboard" : "icon_controller";
                f32 iconSize = convertSize(75);
                f32 margin = convertSize(20);
                ui::rectBlur(ui::IMAGE, g_res.getTexture(icon),
                            pos + glm::vec2(size.x - iconSize - margin, size.y * 0.5f - iconSize * 0.5f),
                            glm::vec2(iconSize), glm::vec4(1.f), btn.fadeInAlpha);

                btn.flags = WidgetFlags::SELECTABLE | WidgetFlags::NAVIGATE_VERTICAL;

            }
            else
            {
                ui::rectBlur(ui::BORDER, &g_res.white, pos - borderSize,
                        size + borderSize * 2, borderColor, 0.35f * btn.fadeInAlpha);
                ui::rectBlur(ui::BG, &g_res.white, pos, size,
                            glm::vec4(glm::vec3(0.f), 0.3f), btn.fadeInAlpha);
                Font* font = &g_res.getFont("font", (u32)convertSize(30));
                f32 textAlpha = 0.3f;
                ui::text(font, "Empty Player Slot", pos + glm::vec2(convertSize(20), size.y * 0.5f),
                            glm::vec3(1.f), textAlpha * btn.fadeInAlpha, btn.fadeInScale,
                            HAlign::LEFT, VAlign::CENTER);
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
                        g_game.state.drivers.push_back(Driver(true, true, false, controller.key));
                        g_game.state.drivers.back().controllerGuid = controller.value.getGuid();
                        g_game.state.drivers.back().playerName = str("Player ", g_game.state.drivers.size());
                    }
                }
            }
        }
    });
}

void Menu::showInitialCarLotMenu(u32 playerIndex)
{
    garage.initialCarSelect = true;
    garage.driver = &g_game.state.drivers[playerIndex];
    garage.previewVehicleIndex = 0;
    garage.previewVehicleConfig = VehicleConfiguration{};
    garage.previewVehicleConfig.reloadMaterials();
    g_vehicles[garage.previewVehicleIndex]->initTuning(garage.previewVehicleConfig, garage.previewTuning);
    garage.currentStats = garage.previewTuning.computeVehicleStats();
    garage.upgradeStats = garage.currentStats;

    reset();
    createVehiclePreview();
    createCarLotMenu();
}

void Menu::showChampionshipMenu()
{
    reset();

    static RenderWorld renderWorlds[MAX_VIEWPORTS];

    i32 playerIndex = 0;
    for (auto& driver : g_game.state.drivers)
    {
        if (driver.isPlayer)
        {
            Widget w;
            w.helpText = "Buy, sell, or upgrade your vehicle";
            w.pos = { 55, -135 + playerIndex * 170 };
            w.size = { 450, 150 };
            w.fadeInScale = 0.7f;
            w.onRender = [&driver, playerIndex](Widget& w, bool isSelected){
                Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("world.Quad");
                u32 vehicleIconSize = (u32)convertSize(w.size.y);
                RenderWorld& rw = renderWorlds[playerIndex];
                rw.setName(tstr("Vehicle Icon ", playerIndex));
                rw.setSize(vehicleIconSize*2, vehicleIconSize*2);
                drawSimple(&rw, quadMesh, &g_res.white, glm::scale(glm::mat4(1.f), glm::vec3(20.f)), glm::vec3(0.02f));
                if (driver.vehicleIndex != -1)
                {
                    //glm::mat4 vehicleTransform = glm::rotate(glm::mat4(1.f), (f32)getTime(), glm::vec3(0, 0, 1));
                    glm::mat4 vehicleTransform = glm::mat4(1.f);
                    driver.getVehicleData()->render(&rw, glm::translate(glm::mat4(1.f),
                            glm::vec3(0, 0, driver.getTuning().getRestOffset())) *
                        vehicleTransform, nullptr, *driver.getVehicleConfig());
                }
                rw.setViewportCount(1);
                rw.addDirectionalLight(glm::vec3(-0.5f, 0.2f, -1.f), glm::vec3(1.0));
                rw.setViewportCamera(0, glm::vec3(8.f, -8.f, 10.f), glm::vec3(0.f, 0.f, 1.f), 1.f, 50.f, 30.f);
                g_game.renderer->addRenderWorld(&rw);

                glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
                glm::vec2 size = convertSize(w.size) * w.fadeInScale;
                glm::vec2 pos = convertSize(w.pos) + center - size * 0.5f;
                f32 borderSize = convertSize(isSelected ? 5 : 2);
                glm::vec4 borderColor = glm::mix(COLOR_NOT_SELECTED, COLOR_SELECTED, w.hover);
                ui::rectBlur(ui::BORDER, &g_res.white, pos-borderSize, size+borderSize*2,
                            borderColor, (0.5f + w.hover * 0.5f) * w.fadeInAlpha);
                ui::rectBlur(ui::BG, &g_res.white, pos, size,
                        glm::vec4(glm::vec3((sinf(w.hoverTimer * 4.f) + 1.f)*0.5f*w.hover*0.04f), 0.8f),
                        w.fadeInAlpha);

                f32 iconSize = vehicleIconSize * w.fadeInScale;
                ui::rectUVBlur(ui::IMAGE, rw.getTexture(), pos, glm::vec2(iconSize), {1,1}, {0,0},
                        glm::vec4(1.f), w.fadeInAlpha);

                f32 textAlpha = (isSelected ? 1.f : 0.5f) * w.fadeInAlpha;
                f32 margin = convertSize(15.f);
                Font* font = &g_res.getFont("font", (u32)convertSize(34));
                ui::text(font, tstr(driver.playerName, "'s Garage"),
                            pos + glm::vec2(iconSize + margin, margin),
                            glm::vec3(1.f), textAlpha, w.fadeInScale);

                Font* fontSmall = &g_res.getFont("font", (u32)convertSize(28));
                ui::text(fontSmall, tstr("Credits: ", driver.credits),
                            pos + glm::vec2(iconSize + margin, margin + convertSize(35.f)),
                            glm::vec3(1.f), textAlpha, w.fadeInScale);

            };
            w.onSelect = [playerIndex, this]{
                garage.driver = &g_game.state.drivers[playerIndex];
                garage.previewVehicleIndex = garage.driver->vehicleIndex;
                garage.previewVehicleConfig = *garage.driver->getVehicleConfig();
                garage.previewTuning = garage.driver->getTuning();
                garage.currentStats = garage.previewTuning.computeVehicleStats();
                garage.upgradeStats = garage.currentStats;
                showGarageMenu();
            };
            w.fadeInScale = 0.7f;
            w.flags = WidgetFlags::SELECTABLE |
                        WidgetFlags::NAVIGATE_VERTICAL |
                        WidgetFlags::NAVIGATE_HORIZONTAL |
                        WidgetFlags::FADE_OUT;
            widgets.push_back(w);

            ++playerIndex;
        }
    }

    f32 x = 500.f;
    f32 y = -170.f;
    glm::vec2 size(400, 80);
    selectedWidget = addButton("BEGIN RACE", "Start the next race.", {x,y}, size, [this]{
        reset();
        Scene* scene = g_game.changeScene(championshipTracks[g_game.state.currentRace]);
        scene->startRace();
    }, WidgetFlags::FADE_OUT | WidgetFlags::FADE_TO_BLACK, g_res.getTexture("icon_flag"));
    y += 100;

    /*
    addButton("STANDINGS", "View the current championship standings.", {x,y}, size, [this]{
        menuMode = CHAMPIONSHIP_STANDINGS;
    }, 0, g_res.getTexture("icon_stats"));
    y += 100;
    */

    addButton("QUIT", "Return to main menu.", {x,y}, size, [this]{
        showMainMenu();
    }, WidgetFlags::FADE_OUT);
    y += 100;

    addBackgroundBox({0,-425+90}, {1920, 180}, 0.5f, false);
    Font* bigFont = &g_res.getFont("font_bold", (u32)convertSize(110));
    Widget* label = addLabel([]{ return tstr("League ", (char)('A' + g_game.state.currentLeague)); },
            {0, -370}, bigFont, HAlign::CENTER, VAlign::CENTER);
    Font* mediumFont = &g_res.getFont("font", (u32)convertSize(60));
    addLabel([]{ return tstr("Race ", g_game.state.currentRace + 1, "/10"); }, {0, -290}, mediumFont,
            HAlign::CENTER, VAlign::CENTER);

    addLogic([label]{
        glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        u32 trackPreviewSize = (u32)convertSize(380);
        g_game.currentScene->updateTrackPreview(g_game.renderer.get(), trackPreviewSize);
        f32 size = trackPreviewSize * label->fadeInScale;
        glm::vec2 trackPreviewPos = center + convertSize({ -450, -350 }) - size * 0.5f;
        Texture* trackTex = g_game.currentScene->getTrackPreview2D().getTexture();
        ui::rectUV(ui::IMAGE, trackTex, trackPreviewPos,
                glm::vec2(size), {0,1}, {1,0}, glm::vec3(1.f), label->fadeInAlpha);
    });
}

static const glm::vec2 vehiclePreviewSize = {600,400};
void Menu::createVehiclePreview()
{
    addBackgroundBox({0,0}, {1920,1080}, 0.3f, false);

    Widget* box = addBackgroundBox({-260,200}, {vehiclePreviewSize.x, 300}, 0.8f);
    addHelpMessage({0, 400});

    Font* font = &g_res.getFont("font", (u32)convertSize(30));
    std::string garageName = str(garage.driver->playerName, "'s Garage");

    addBackgroundBox({-260, -375}, {vehiclePreviewSize.x, 50}, 0.8f);
    addLabel([=]{ return tstr(garageName); }, {-260 - vehiclePreviewSize.x * 0.5f
            + font->stringDimensions(garageName.c_str()).x * 0.5f + 20, -375}, font);

    Font* font2 = &g_res.getFont("font_bold", (u32)convertSize(30));
    std::string creditsMaxSize = "CREDITS: 000000";
    addBackgroundBox({-260 + vehiclePreviewSize.x * 0.5f
            - font2->stringDimensions(creditsMaxSize.c_str()).x * 0.5f - 20, -375},
            { font2->stringDimensions(creditsMaxSize.c_str()).x + 40, 50.f }, 0.92f);
    addLabel([this]{ return tstr("CREDITS: ", garage.driver->credits); },
            {-260 + vehiclePreviewSize.x * 0.5f
            - font2->stringDimensions(creditsMaxSize.c_str()).x * 0.5f - 20, -375}, font2,
            HAlign::CENTER, VAlign::CENTER, COLOR_SELECTED);

    static RenderWorld rw;

    struct Stat
    {
        const char* name = nullptr;
        f32 value = 0.5f;
    };
    static Stat stats[] = {
        { "ACCELERATION" },
        { "TOP SPEED" },
        { "ARMOR" },
        { "MASS" },
        { "GRIP" },
        { "OFFROAD" },
    };
    static f32 statsUpgrade[ARRAY_SIZE(stats)] = { 0 };

    addLogic([box,this]{
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

        Font* tinyfont = &g_res.getFont("font", (u32)convertSize(24));
        f32 barMargin = convertSize(30 * box->fadeInScale);
        f32 maxBarWidth = convertSize(vehiclePreviewSize.x * box->fadeInScale) - barMargin*2;
        f32 barHeight = convertSize(8 * box->fadeInScale);
        f32 barSep = convertSize(40 * box->fadeInScale);
        f32 barOffset = convertSize(20 * box->fadeInScale);
        glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        glm::vec2 statsPos = center +
            convertSize(box->pos - box->size * 0.5f * box->fadeInScale) + barMargin;
        for (u32 i=0; i<ARRAY_SIZE(stats); ++i)
        {
            stats[i].value = smoothMove(stats[i].value, targetStats[i], 8.f, g_game.deltaTime);
            statsUpgrade[i] = smoothMove(statsUpgrade[i], targetStatsUpgrade[i], 8.f, g_game.deltaTime);

            ui::text(tinyfont, stats[i].name, statsPos + glm::vec2(0, i * barSep),
                        glm::vec3(1.f), box->fadeInAlpha);

            ui::rectBlur(ui::ICON-1, &g_res.white, statsPos + glm::vec2(0, i * barSep + barOffset),
                        glm::vec2(maxBarWidth, barHeight), glm::vec4(0,0,0,0.9f), box->fadeInAlpha);

            f32 barWidth = maxBarWidth * stats[i].value;
            f32 upgradeBarWidth = maxBarWidth * statsUpgrade[i];

            if (upgradeBarWidth - barWidth > 0.01f)
            {
                ui::rectBlur(ui::ICON+1, &g_res.white, statsPos + glm::vec2(0, i * barSep + barOffset),
                        glm::vec2(upgradeBarWidth, barHeight),
                        glm::vec4(0.01f,0.7f,0.01f,0.9f), box->fadeInAlpha);
            }

            if (upgradeBarWidth - barWidth < -0.01f)
            {
                ui::rectBlur(ui::ICON+2, &g_res.white, statsPos + glm::vec2(0, i * barSep + barOffset),
                            glm::vec2(upgradeBarWidth, barHeight), glm::vec4(0.8f, 0.01f, 0.01f, 0.9f),
                            box->fadeInAlpha);
            }

            ui::rectBlur(ui::ICON+3, &g_res.white, statsPos + glm::vec2(0, i * barSep + barOffset),
                        glm::vec2(glm::min(upgradeBarWidth, barWidth), barHeight),
                        glm::vec4(glm::vec3(0.7f), 1.f), box->fadeInAlpha);
        }
    });

    addLogic([this](Widget& w){
        glm::vec2 vehicleIconSize = convertSize(vehiclePreviewSize);

        Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("world.Quad");
        rw.setName("Garage");
        rw.setSize((u32)vehicleIconSize.x, (u32)vehicleIconSize.y);
        drawSimple(&rw, quadMesh, &g_res.white,
                glm::scale(glm::mat4(1.f), glm::vec3(20.f)), glm::vec3(0.02f));

        glm::mat4 vehicleTransform = glm::rotate(glm::mat4(1.f), (f32)getTime(), glm::vec3(0, 0, 1));
        g_vehicles[garage.previewVehicleIndex]->render(&rw, glm::translate(glm::mat4(1.f),
                glm::vec3(0, 0, garage.previewTuning.getRestOffset())) *
            vehicleTransform, nullptr, garage.previewVehicleConfig);
        rw.setViewportCount(1);
        rw.addDirectionalLight(glm::vec3(-0.5f, 0.2f, -1.f), glm::vec3(1.0));
        rw.setViewportCamera(0, glm::vec3(8.f, -8.f, 10.f), glm::vec3(0.f, 0.f, 1.f), 1.f, 50.f, 30.f);
        g_game.renderer->addRenderWorld(&rw);

        glm::vec2 size = vehicleIconSize * w.fadeInScale;
        glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        glm::vec2 pos = center + convertSize({-260, -150}) - size * 0.5f;
        ui::rectUVBlur(ui::IMAGE, rw.getTexture(), pos, size, {1,1}, {0,0},
                glm::vec4(1.f), w.fadeInAlpha);
    });
}

void Menu::createMainGarageMenu()
{
    Texture* iconbg = g_res.getTexture("iconbg");
    glm::vec2 size(450, 75);
    f32 x = 280;
    f32 y = -400 + size.y * 0.5f;
    f32 gap = 12;
    selectedWidget = addButton("PERFORMANCE", "Upgrades to enhance your vehicle's performance.", {x,y}, size, [this]{
        resetTransient();
        createPerformanceMenu();
    }, WidgetFlags::TRANSIENT | WidgetFlags::FADE_OUT_TRANSIENT, g_res.getTexture("icon_engine"));
    y += size.y + gap;

    addButton("COSMETICS", "Change your vehicle's appearance with decals or paint.", {x,y}, size, [this]{
        resetTransient();
        createCosmeticsMenu();
    }, WidgetFlags::TRANSIENT | WidgetFlags::FADE_OUT_TRANSIENT, g_res.getTexture("icon_spraycan"));
    y += size.y + gap;

    i32 frontWeapon = garage.previewVehicleConfig.frontWeaponIndices[0];
    addButton("FRONT WEAPON", "Install a weapon on the front weapon slot.", {x,y}, size, [this]{
        resetTransient();
        createWeaponsMenu(WeaponType::FRONT_WEAPON,
                garage.previewVehicleConfig.frontWeaponIndices[0],
                garage.previewVehicleConfig.frontWeaponUpgradeLevel[0]);
    }, WidgetFlags::TRANSIENT | WidgetFlags::FADE_OUT_TRANSIENT,
        frontWeapon == -1 ? iconbg : g_weapons[frontWeapon].info.icon);
    y += size.y + gap;

    if (g_vehicles[garage.previewVehicleIndex]->frontWeaponCount > 1)
    {
        i32 roofWeapon = garage.previewVehicleConfig.frontWeaponIndices[1];
        addButton("ROOF WEAPON", "Install a weapon on the roof weapon slot.", {x,y}, size, [this]{
            resetTransient();
            createWeaponsMenu(WeaponType::FRONT_WEAPON,
                garage.previewVehicleConfig.frontWeaponIndices[1],
                garage.previewVehicleConfig.frontWeaponUpgradeLevel[1]);
        }, WidgetFlags::TRANSIENT | WidgetFlags::FADE_OUT_TRANSIENT,
            roofWeapon == -1 ? iconbg : g_weapons[roofWeapon].info.icon);
        y += size.y + gap;
    }

    i32 rearWeapon = garage.previewVehicleConfig.rearWeaponIndices[0];
    addButton("REAR WEAPON", "Install a weapon in the rear weapon slot.", {x,y}, size, [this]{
        resetTransient();
        createWeaponsMenu(WeaponType::REAR_WEAPON,
                garage.previewVehicleConfig.rearWeaponIndices[0],
                garage.previewVehicleConfig.rearWeaponUpgradeLevel[0]);
    }, WidgetFlags::TRANSIENT | WidgetFlags::FADE_OUT_TRANSIENT,
        rearWeapon == -1 ? iconbg : g_weapons[rearWeapon].info.icon);
    y += size.y + gap;

    addButton("PASSIVE ABILITY", "Install a passive ability.", {x,y}, size, [this]{
        resetTransient();
        static u32 upgradeLevel;
        upgradeLevel = garage.previewVehicleConfig.specialAbilityIndex == -1 ? 0 : 1;
        createWeaponsMenu(WeaponType::SPECIAL_ABILITY,
                garage.previewVehicleConfig.specialAbilityIndex, upgradeLevel);
    }, WidgetFlags::TRANSIENT | WidgetFlags::FADE_OUT_TRANSIENT,
        garage.previewVehicleConfig.specialAbilityIndex == -1 ? iconbg
            : g_weapons[garage.previewVehicleConfig.specialAbilityIndex].info.icon);
    y += size.y + gap;

    addButton("CAR LOT", "Buy a new vehicle!", {x,y}, size, [this]{
        resetTransient();
        createCarLotMenu();
    }, WidgetFlags::TRANSIENT | WidgetFlags::FADE_OUT_TRANSIENT);
    y += size.y + gap;

    addButton("DONE", nullptr, {x, 350-size.y*0.5f}, size, [this]{
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
                showChampionshipMenu();
            }
            else
            {
                showInitialCarLotMenu(garage.playerIndex);
            }
        }
        else
        {
            showChampionshipMenu();
        }
    }, WidgetFlags::FADE_OUT | WidgetFlags::BACK | WidgetFlags::TRANSIENT);
}

void Menu::createPerformanceMenu()
{
    glm::vec2 buttonSize(450, 75);
    addButton("BACK", nullptr, {280, 350-buttonSize.y*0.5f}, buttonSize, [this]{
        garage.previewVehicleConfig = *garage.driver->getVehicleConfig();
        garage.upgradeStats = garage.currentStats;
        resetTransient();
        createMainGarageMenu();
    }, WidgetFlags::FADE_OUT_TRANSIENT | WidgetFlags::BACK | WidgetFlags::TRANSIENT);

    glm::vec2 size(150, 150);
    f32 x = 280-buttonSize.x*0.5f + size.x*0.5f;
    f32 y = -400 + size.y * 0.5f;
    f32 gap = 6;
    u32 buttonsPerRow = 3;

    static i32 previewUpgradeIndex;
    previewUpgradeIndex = -1;
    for (i32 i=0; i<(i32)garage.driver->getVehicleData()->availableUpgrades.size(); ++i)
    {
        auto& upgrade = garage.driver->getVehicleData()->availableUpgrades[i];
        glm::vec2 pos = { x + (i % buttonsPerRow) * (size.x + gap),
                          y + (i / buttonsPerRow) * (size.y + gap) };
        Widget* w = addImageButton(upgrade.name, upgrade.description, pos, size, [i, this]{
            auto& upgrade = garage.driver->getVehicleData()->availableUpgrades[i];
            auto currentUpgrade = garage.driver->getVehicleConfig()->performanceUpgrades.find(
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
            if (isPurchasable)
            {
                g_audio.playSound(g_res.getSound("airdrill"), SoundType::MENU_SFX);
                garage.driver->credits -= price;
                garage.driver->getVehicleConfig()->addUpgrade(i);
            }
            else
            {
                g_audio.playSound(g_res.getSound("nono"), SoundType::MENU_SFX);
            }
        }, WidgetFlags::TRANSIENT, upgrade.icon, 48, [i, this](bool isSelected){
            auto& upgrade = garage.driver->getVehicleData()->availableUpgrades[i];
            auto currentUpgrade = garage.driver->getVehicleConfig()->performanceUpgrades.find(
                    [i](auto& u) { return u.upgradeIndex == i; });
            bool isEquipped = !!currentUpgrade;
            i32 upgradeLevel = 0;
            i32 price = upgrade.price;
            if (isEquipped)
            {
                upgradeLevel = currentUpgrade->upgradeLevel;
                price = upgrade.price * (upgradeLevel + 1);
            }

            if (isSelected && upgradeLevel < upgrade.maxUpgradeLevel && previewUpgradeIndex != i)
            {
                previewUpgradeIndex = i;
                garage.previewVehicleConfig = *garage.driver->getVehicleConfig();
                garage.previewVehicleConfig.addUpgrade(i);
                g_vehicles[garage.previewVehicleIndex]->initTuning(garage.previewVehicleConfig, garage.previewTuning);
                garage.upgradeStats = garage.previewTuning.computeVehicleStats();
            }

            bool isPurchasable = garage.driver->credits >= price && upgradeLevel < upgrade.maxUpgradeLevel;
            return ImageButtonInfo{
                isPurchasable, false, upgrade.maxUpgradeLevel, upgradeLevel, tstr(price) };
        });

        if (i == 0)
        {
            selectedWidget = w;
        }
    }
}

void Menu::createCosmeticsMenu()
{
    glm::vec2 size(450, 70);
    f32 x = 280;
    f32 y = -400 + size.y * 0.5f;
    f32 gap = 12;

    static f32 hue;
    static f32 saturation;
    static f32 value;
    hue = garage.previewVehicleConfig.hsv.x;
    saturation = garage.previewVehicleConfig.hsv.y;
    value = garage.previewVehicleConfig.hsv.z;

    selectedWidget = addSlider("PAINT HUE", {x,y}, size, WidgetFlags::TRANSIENT, [this](f32 val){
        hue = val;
        garage.previewVehicleConfig.hsv.x = hue;
        garage.previewVehicleConfig.color = srgb(hsvToRgb(hue, saturation, value));
        garage.previewVehicleConfig.reloadMaterials();
        garage.driver->getVehicleConfig()->hsv = garage.previewVehicleConfig.hsv;
        garage.driver->getVehicleConfig()->color = garage.previewVehicleConfig.color;
        garage.driver->getVehicleConfig()->reloadMaterials();
    }, []{
        return SliderInfo{
            glm::vec3(1.f),
            glm::vec3(1.f),
            hue,
            g_res.getTexture("hues"),
            0.01f,
            0.99f,
        };
    });
    y += size.y + gap;

    addSlider("PAINT SATURATION", {x,y}, size, WidgetFlags::TRANSIENT, [this](f32 val){
        saturation = val;
        garage.previewVehicleConfig.hsv.y = saturation;
        garage.previewVehicleConfig.color = srgb(hsvToRgb(hue, saturation, value));
        garage.previewVehicleConfig.reloadMaterials();
        garage.driver->getVehicleConfig()->hsv = garage.previewVehicleConfig.hsv;
        garage.driver->getVehicleConfig()->color = garage.previewVehicleConfig.color;
        garage.driver->getVehicleConfig()->reloadMaterials();
    }, []{
        return SliderInfo{
            hsvToRgb(hue, 0.f, value),
            hsvToRgb(hue, 1.f, value),
            saturation,
            &g_res.white,
            0.f,
            0.99f,
        };
    });
    y += size.y + gap;

    addSlider("PAINT BRIGHTNESS", {x,y}, size, WidgetFlags::TRANSIENT, [this](f32 val){
        value = val;
        garage.previewVehicleConfig.hsv.z = value;
        garage.previewVehicleConfig.color = srgb(hsvToRgb(hue, saturation, value));
        garage.previewVehicleConfig.reloadMaterials();
        garage.driver->getVehicleConfig()->hsv = garage.previewVehicleConfig.hsv;
        garage.driver->getVehicleConfig()->color = garage.previewVehicleConfig.color;
        garage.driver->getVehicleConfig()->reloadMaterials();
    }, []{
        return SliderInfo{
            hsvToRgb(hue, saturation, 0.f),
            hsvToRgb(hue, saturation, 1.f),
            value,
            &g_res.white,
            0.02f,
            0.98f,
        };
    });
    y += size.y + gap;

    addSlider("PAINT SHININESS", {x,y}, size, WidgetFlags::TRANSIENT, [this](f32 val){
        garage.previewVehicleConfig.paintShininess = val;
        garage.previewVehicleConfig.reloadMaterials();
        garage.driver->getVehicleConfig()->paintShininess = garage.previewVehicleConfig.paintShininess;
        garage.driver->getVehicleConfig()->reloadMaterials();
    }, [&]{
        return SliderInfo{ glm::vec3(0.f), glm::vec3(1.f),
            garage.previewVehicleConfig.paintShininess, &g_res.white, 0.f, 1.f };
    });
    y += size.y + gap;

    for (u32 i=0; i<3; ++i)
    {
        const char* text[3] = { "LAYER 1", "LAYER 2", "LAYER 3" };
        addButton(text[i], "Add graphics to your vehicle.", {x,y}, size, [&]{
        }, WidgetFlags::TRANSIENT);
        y+= size.y + gap;
    }

    glm::vec2 buttonSize(450, 75);
    addButton("BACK", nullptr, {280, 350-buttonSize.y*0.5f}, buttonSize, [this]{
        resetTransient();
        createMainGarageMenu();
    }, WidgetFlags::FADE_OUT_TRANSIENT | WidgetFlags::BACK | WidgetFlags::TRANSIENT);

}

void Menu::createCarLotMenu()
{
    static VehicleConfiguration vehicleConfig;
    static Array<RenderWorld> renderWorlds(g_vehicles.size());

    if (garage.previewVehicleIndex == -1)
    {
        garage.previewVehicleIndex = 0;
    }

    glm::vec2 buttonSize(450, 75);
    addButton("DONE", nullptr, {280, 350-buttonSize.y*0.5f}, buttonSize, [this]{
        garage.previewVehicleIndex = garage.driver->vehicleIndex;
        garage.previewVehicleConfig = *garage.driver->getVehicleConfig();
        garage.previewTuning = garage.driver->getTuning();
        garage.currentStats = garage.previewTuning.computeVehicleStats();
        garage.upgradeStats = garage.currentStats;
        resetTransient();
        createMainGarageMenu();
    }, WidgetFlags::FADE_OUT_TRANSIENT | WidgetFlags::BACK | WidgetFlags::TRANSIENT, nullptr, [this]{
        return garage.driver->vehicleIndex != -1;
    });

    addButton("BUY CAR", nullptr, {280, 350-buttonSize.y*0.5f - buttonSize.y - 12}, buttonSize, [this]{
        garage.driver->vehicleIndex = garage.previewVehicleIndex;
        garage.driver->vehicleConfig = garage.previewVehicleConfig;
        i32 totalCost = g_vehicles[garage.previewVehicleIndex]->price - garage.driver->getVehicleValue();
        garage.driver->credits -= totalCost;
    }, WidgetFlags::TRANSIENT, nullptr, [this]{
        i32 totalCost = g_vehicles[garage.previewVehicleIndex]->price - garage.driver->getVehicleValue();
        return garage.previewVehicleIndex != garage.driver->vehicleIndex &&
            garage.driver->credits >= totalCost;
    });

    {
        Widget w;
        w.flags = WidgetFlags::TRANSIENT;
        w.onRender = [this](Widget& w, bool) {
            Font* fontBold = &g_res.getFont("font_bold", (u32)convertSize(24));
            Font* font = &g_res.getFont("font", (u32)convertSize(24));

            glm::vec2 pos = glm::vec2(-260, -150) - vehiclePreviewSize * 0.5f + glm::vec2(20);
            glm::vec2 center = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;

            if (garage.driver->vehicleIndex == -1)
            {
                ui::text(fontBold,
                    tstr("PRICE: ", g_vehicles[garage.previewVehicleIndex]->price),
                    center + convertSize(pos), glm::vec3(1.f),
                    w.fadeInAlpha, 1.f, HAlign::LEFT, VAlign::TOP);
            }
            else
            {
                if (garage.driver->vehicleIndex != garage.previewVehicleIndex)
                {
                    ui::text(font,
                        tstr("PRICE: ", g_vehicles[garage.previewVehicleIndex]->price),
                        center + convertSize(pos), glm::vec3(0.6f),
                        w.fadeInAlpha, 1.f, HAlign::LEFT, VAlign::TOP);

                    ui::text(font,
                        tstr("TRADE: ", garage.driver->getVehicleValue()),
                        center + convertSize(pos + glm::vec2(0, 24)), glm::vec3(0.6f),
                        w.fadeInAlpha, 1.f, HAlign::LEFT, VAlign::TOP);

                    ui::text(fontBold,
                        tstr("TOTAL: ", g_vehicles[garage.previewVehicleIndex]->price - garage.driver->getVehicleValue()),
                        center + convertSize(pos + glm::vec2(0, 48)), glm::vec3(1.f),
                        w.fadeInAlpha, 1.f, HAlign::LEFT, VAlign::TOP);
                }
            }
        };
        widgets.push_back(w);
    }

    glm::vec2 size(150, 150);
    f32 x = 280-buttonSize.x*0.5f + size.x*0.5f;
    f32 y = -400 + size.y * 0.5f;
    f32 gap = 6;
    u32 buttonsPerRow = 3;

    for (i32 i=0; i<(i32)g_vehicles.size(); ++i)
    {
        RenderWorld& rw = renderWorlds[i];

        Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("world.Quad");
        rw.setName("Garage");
        rw.setSize((u32)convertSize(size.x)*2, (u32)convertSize(size.y)*2);
        drawSimple(&rw, quadMesh, &g_res.white, glm::scale(glm::mat4(1.f), glm::vec3(20.f)), glm::vec3(0.02f));

        VehicleTuning tuning;
        g_vehicles[i]->initTuning(vehicleConfig, tuning);
        f32 vehicleAngle = 0.f;
        glm::mat4 vehicleTransform = glm::rotate(glm::mat4(1.f), vehicleAngle, glm::vec3(0, 0, 1));
        g_vehicles[i]->render(&rw, glm::translate(glm::mat4(1.f), glm::vec3(0, 0, tuning.getRestOffset())) *
            vehicleTransform, nullptr, vehicleConfig);
        rw.setViewportCount(1);
        rw.addDirectionalLight(glm::vec3(-0.5f, 0.2f, -1.f), glm::vec3(1.0));
        rw.setViewportCamera(0, glm::vec3(8.f, -8.f, 10.f), glm::vec3(0.f, 0.f, 1.f), 1.f, 50.f, 30.f);
        g_game.renderer->addRenderWorld(&rw);

        glm::vec2 pos = { x + (i % buttonsPerRow) * (size.x + gap),
                          y + (i / buttonsPerRow) * (size.y + gap) };
        Widget* w = addImageButton(g_vehicles[i]->name, g_vehicles[i]->description, pos, size, [i,this]{
            garage.previewVehicleIndex = i;
            if (i == garage.driver->vehicleIndex)
            {
                garage.previewVehicleConfig = *garage.driver->getVehicleConfig();
                garage.previewTuning = garage.driver->getTuning();
                garage.currentStats = garage.previewTuning.computeVehicleStats();
                garage.upgradeStats = garage.currentStats;
            }
            else
            {
                garage.previewVehicleConfig = VehicleConfiguration{};
                garage.previewVehicleConfig.reloadMaterials();
                g_vehicles[garage.previewVehicleIndex]->initTuning(garage.previewVehicleConfig, garage.previewTuning);
                garage.currentStats = garage.previewTuning.computeVehicleStats();
                garage.upgradeStats = garage.currentStats;
            }
        }, WidgetFlags::TRANSIENT, rw.getTexture(), 0.f, [i,this](bool isSelected){
            //bool buttonEnabled = garage.driver->credits >= g_vehicles[i]->price || i == garage.driver->vehicleIndex;
            bool buttonEnabled = true;
            return ImageButtonInfo{
                buttonEnabled,
                i == garage.previewVehicleIndex, 0, 0,
                i == garage.driver->vehicleIndex ? "OWNED" : tstr(g_vehicles[i]->price), true };
        });

        if (i == 0)
        {
            selectedWidget = w;
        }
    }
}

void Menu::createWeaponsMenu(WeaponType weaponType, i32& weaponSlot, u32& upgradeLevel)
{
    glm::vec2 buttonSize(450, 75);
    selectedWidget = addButton("BACK", nullptr, {280, 350-buttonSize.y*0.5f}, buttonSize, [this]{
        garage.driver->vehicleConfig = garage.previewVehicleConfig;
        resetTransient();
        createMainGarageMenu();
    }, WidgetFlags::FADE_OUT_TRANSIENT | WidgetFlags::BACK | WidgetFlags::TRANSIENT);

    selectedWidget = addButton("SELL", "Sell the currently equipped item for half the original value.",
        {280, 350-buttonSize.y*0.5f-buttonSize.y-12}, buttonSize, [this, &weaponSlot, &upgradeLevel]{
        garage.driver->credits += g_weapons[weaponSlot].info.price / 2;
        upgradeLevel -= 1;
        // TODO: use different sound for selling
        g_audio.playSound(g_res.getSound("airdrill"), SoundType::MENU_SFX);
        if (upgradeLevel == 0)
        {
            weaponSlot = -1;
        }
    }, WidgetFlags::TRANSIENT, nullptr, [&upgradeLevel]{
        return upgradeLevel > 0;
    });

    glm::vec2 size(150, 150);
    f32 x = 280-buttonSize.x*0.5f + size.x*0.5f;
    f32 y = -400 + size.y * 0.5f;
    f32 gap = 6;
    u32 buttonsPerRow = 3;

    i32 buttonCount = 0;
    for (i32 i=0; i<(i32)g_weapons.size(); ++i)
    {
        auto& weapon = g_weapons[i];
        if (weapon.info.weaponType != weaponType)
        {
            continue;
        }
        glm::vec2 pos = { x + (buttonCount % buttonsPerRow) * (size.x + gap),
                          y + (buttonCount / buttonsPerRow) * (size.y + gap) };
        ++buttonCount;
        Widget* w = addImageButton(weapon.info.name, weapon.info.description, pos, size,
            [i, this, &weaponSlot, &upgradeLevel]{
            u32 weaponUpgradeLevel = (weaponSlot == i) ? upgradeLevel: 0;
            bool isPurchasable = garage.driver->credits >= g_weapons[i].info.price
                && weaponUpgradeLevel < g_weapons[i].info.maxUpgradeLevel
                && (weaponSlot == -1 || weaponSlot == i);
            if (isPurchasable)
            {
                // TODO: Play sound that is more appropriate for a weapon
                g_audio.playSound(g_res.getSound("airdrill"), SoundType::MENU_SFX);
                garage.driver->credits -= g_weapons[i].info.price;
                weaponSlot = i;
                upgradeLevel += 1;
            }
            else
            {
                g_audio.playSound(g_res.getSound("nono"), SoundType::MENU_SFX);
            }
        }, WidgetFlags::TRANSIENT, weapon.info.icon, 48,
            [i, this, &weaponSlot, &upgradeLevel](bool isSelected){
            u32 weaponUpgradeLevel = (weaponSlot == i) ? upgradeLevel: 0;
            bool isPurchasable = garage.driver->credits >= g_weapons[i].info.price
                && weaponUpgradeLevel < g_weapons[i].info.maxUpgradeLevel
                && (weaponSlot == -1 || weaponSlot == i);
            return ImageButtonInfo{
                isPurchasable, weaponSlot == i, (i32)g_weapons[i].info.maxUpgradeLevel,
                (i32)weaponUpgradeLevel, tstr(g_weapons[i].info.price) };
        });

        if (i == 0)
        {
            selectedWidget = w;
        }
    }
}

void Menu::showGarageMenu()
{
    reset();
    createVehiclePreview();
    createMainGarageMenu();
}

void Menu::championshipStandings()
{
    f32 w = convertSize720i(550);

    f32 cw = (f32)(g_game.windowWidth/2);
    glm::vec2 menuPos = glm::vec2(cw - w/2, glm::floor(g_game.windowHeight * 0.1f));
    glm::vec2 menuSize = glm::vec2(w, (f32)g_game.windowHeight * 0.8f);
    drawBox(menuPos, menuSize);

    Font* bigfont = &g_res.getFont("font_bold", (u32)convertSize720(26));
    Font* smallfont = &g_res.getFont("font", (u32)convertSize720(16));

    ui::text(bigfont, "Championship Standings",
                glm::vec2(cw, menuPos.y + convertSize720i(32)), glm::vec3(1.f),
                1.f, 1.f, HAlign::CENTER, VAlign::TOP);

    ui::text(smallfont, tstr("League ", (char)('A' + g_game.state.currentLeague)),
                glm::vec2(cw, menuPos.y + convertSize720i(58)), glm::vec3(1.f),
                1.f, 1.f, HAlign::CENTER, VAlign::TOP);

    static RenderWorld renderWorlds[10];

    f32 columnOffset[] = {
        32, 60, 150, 350
    };

    u32 vehicleIconSize = (u32)convertSize720(48);
    Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("world.Quad");
    SmallArray<Driver*, 20> sortedDrivers;
    for(auto& driver : g_game.state.drivers)
    {
        sortedDrivers.push_back(&driver);
    }
    sortedDrivers.sort([](Driver* a, Driver* b) { return a->leaguePoints > b->leaguePoints; });

    for (u32 i=0; i<sortedDrivers.size(); ++i)
    {
        Driver* driver = sortedDrivers[i];

        RenderWorld& rw = renderWorlds[i];
        rw.setName(tstr("Vehicle Icon ", i));
        rw.setSize(vehicleIconSize*2, vehicleIconSize*2);
        drawSimple(&rw, quadMesh, &g_res.white, glm::scale(glm::mat4(1.f), glm::vec3(20.f)), glm::vec3(0.02f));
        if (driver->vehicleIndex != -1)
        {
            g_vehicles[driver->vehicleIndex]->render(&rw,
                glm::translate(glm::mat4(1.f),
                    glm::vec3(0, 0, driver->getTuning().getRestOffset())) *
                //glm::rotate(glm::mat4(1.f), (f32)getTime(), glm::vec3(0, 0, 1)),
                glm::mat4(1.f),
                nullptr, *driver->getVehicleConfig());
        }
        rw.setViewportCount(1);
        rw.addDirectionalLight(glm::vec3(-0.5f, 0.2f, -1.f), glm::vec3(1.0));
        rw.setViewportCamera(0, glm::vec3(8.f, -8.f, 10.f),
                glm::vec3(0.f, 0.f, 1.f), 1.f, 50.f, 28.f);
        g_game.renderer->addRenderWorld(&rw);

        glm::vec2 pos = menuPos + glm::vec2(0.f,
                        convertSize720i(100) + i * convertSize720(48));
        ui::text(smallfont, tstr(i + 1),
                    pos + glm::vec2(convertSize720i(columnOffset[0]), 0),
                    glm::vec3(1.f), 1.f, 1.f, HAlign::LEFT, VAlign::CENTER);

        ui::rectUV(ui::IMAGE, rw.getTexture(),
                    pos + glm::vec2(convertSize720i(columnOffset[1]), -glm::floor(vehicleIconSize/2)),
                    {1,1}, {0,0}, glm::vec2(vehicleIconSize), glm::vec3(1.f), 1.f);

        ui::text(smallfont, driver->playerName.c_str(),
                    pos + glm::vec2(convertSize720i(columnOffset[2]), 0),
                    glm::vec3(1.f), 1.f, 1.f, HAlign::LEFT, VAlign::CENTER);
        ui::text(smallfont, tstr(driver->leaguePoints),
                    pos + glm::vec2(convertSize720i(columnOffset[3]), 0),
                    glm::vec3(1.f), 1.f, 1.f, HAlign::LEFT, VAlign::CENTER);
    }

    if (didSelect())
    {
        g_audio.playSound(g_res.getSound("close"), SoundType::MENU_SFX);
        showChampionshipMenu();
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
    }
}

void Menu::raceResults()
{
    f32 w = convertSize720i(650);

    f32 cw = (f32)(g_game.windowWidth/2);
    glm::vec2 menuPos = glm::vec2(cw - w/2, glm::floor(g_game.windowHeight * 0.2f));
    glm::vec2 menuSize = glm::vec2(w, (f32)g_game.windowHeight * 0.6f);
    drawBox(menuPos, menuSize);

    Font* bigfont = &g_res.getFont("font_bold", (u32)convertSize720(26));
    Font* smallfont = &g_res.getFont("font", (u32)convertSize720(16));
    Font* tinyfont = &g_res.getFont("font", (u32)convertSize720(14));

    ui::text(bigfont, "Race Results",
                glm::vec2(cw, menuPos.y + convertSize720i(32)), glm::vec3(1.f),
                1.f, 1.f, HAlign::CENTER, VAlign::TOP);

    ui::rect(ui::BG, nullptr,
                menuPos + glm::vec2(convertSize720(8), convertSize720(64)),
                glm::vec2(w - convertSize720(16), convertSize720(19)), glm::vec3(0.f), 0.6f);

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
        ui::text(tinyfont, columnTitle[i],
                    menuPos + glm::vec2(convertSize720i(columnOffset[i]), convertSize720(70)),
                    glm::vec3(1.f), 1.f, 1.f, HAlign::LEFT, VAlign::TOP);
    }

    for (auto& row : g_game.currentScene->getRaceResults())
    {
        glm::vec2 pos = menuPos + glm::vec2(0.f,
                        convertSize720i(100) + row.placement * convertSize720(24));
        ui::text(smallfont, tstr(row.placement + 1),
                    pos + glm::vec2(convertSize720i(columnOffset[0]), 0),
                    glm::vec3(1.f), 1.f, 1.f, HAlign::LEFT, VAlign::TOP);
        ui::text(smallfont, row.driver->playerName.c_str(),
                    pos + glm::vec2(convertSize720i(columnOffset[1]), 0),
                    glm::vec3(1.f), 1.f, 1.f, HAlign::LEFT, VAlign::TOP);
        ui::text(smallfont, tstr(row.statistics.accidents),
                    pos + glm::vec2(convertSize720i(columnOffset[2]), 0),
                    glm::vec3(1.f), 1.f, 1.f, HAlign::LEFT, VAlign::TOP);
        ui::text(smallfont, tstr(row.statistics.destroyed),
                    pos + glm::vec2(convertSize720i(columnOffset[3]), 0),
                    glm::vec3(1.f), 1.f, 1.f, HAlign::LEFT, VAlign::TOP);
        ui::text(smallfont, tstr(row.statistics.frags),
                    pos + glm::vec2(convertSize720i(columnOffset[4]), 0),
                    glm::vec3(1.f), 1.f, 1.f, HAlign::LEFT, VAlign::TOP);
        ui::text(smallfont, tstr(row.getBonus()),
                    pos + glm::vec2(convertSize720i(columnOffset[5]), 0),
                    glm::vec3(1.f), 1.f, 1.f, HAlign::LEFT, VAlign::TOP);
        if (g_game.state.gameMode == GameMode::CHAMPIONSHIP)
        {
            ui::text(smallfont, tstr(row.getCreditsEarned()),
                        pos + glm::vec2(convertSize720i(columnOffset[6]), 0),
                        glm::vec3(1.f), 1.f, 1.f, HAlign::LEFT, VAlign::TOP);
        }
    }

    if (didSelect())
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
    }, WidgetFlags::FADE_OUT | WidgetFlags::BACK);
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

    glm::vec2 size(750, 50);
    f32 y = -300;
    selectedWidget = addSelector("Resolution", "The internal render resolution.", { 0, y },
        size, resolutionNames, valueIndex, [this](i32 valueIndex){
            tmpConfig.graphics.resolutionX = resolutions[valueIndex].x;
            tmpConfig.graphics.resolutionY = resolutions[valueIndex].y;
        });
    y += size.y + 10;

    addSelector("Fullscreen", "Run the game in fullscreen or window.", { 0, y }, size, { "OFF", "ON" }, tmpConfig.graphics.fullscreen,
            [this](i32 valueIndex){ tmpConfig.graphics.fullscreen = (bool)valueIndex; });
    y += size.y + 10;

    addSelector("V-Sync", "Lock the frame rate to the monitor's refresh rate.",
            { 0, y }, size, { "OFF", "ON" }, tmpConfig.graphics.vsync,
            [this](i32 valueIndex){ tmpConfig.graphics.vsync = (bool)valueIndex; });
    y += size.y + 10;

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
    addSelector("Shadows", "The quality of shadows cast by objects in the world.", { 0, y },
        size, shadowQualityNames, shadowQualityIndex, [this](i32 valueIndex){
            tmpConfig.graphics.shadowsEnabled = valueIndex > 0;
            tmpConfig.graphics.shadowMapResolution = shadowMapResolutions[valueIndex];
        });
    y += size.y + 10;

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

    addSelector("SSAO", "Darkens occluded areas of the world for improved realism.", { 0, y }, size, ssaoQualityNames,
        ssaoQualityIndex, [this](i32 valueIndex){
            tmpConfig.graphics.ssaoEnabled = valueIndex > 0;
            tmpConfig.graphics.ssaoHighQuality = valueIndex == 2;
        });
    y += size.y + 10;

    addSelector("Bloom", "Adds a glowy light-bleeding effect to bright areas of the world.", { 0, y }, size, { "OFF", "ON" }, tmpConfig.graphics.bloomEnabled,
            [this](i32 valueIndex){ tmpConfig.graphics.bloomEnabled = (bool)valueIndex; });
    y += size.y + 10;

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
    addSelector("Anti-Aliasing", "Improves image quality by smoothing jagged edges.", { 0, y }, size, aaLevelNames,
        aaIndex, [this](i32 valueIndex){
            tmpConfig.graphics.msaaLevel = aaLevels[valueIndex];
        });
    y += size.y + 10;

    addSelector("Sharpen", "Sharpen the final image in a post-processing pass.",
            { 0, y }, size, { "OFF", "ON" }, tmpConfig.graphics.sharpenEnabled,
            [this](i32 valueIndex){ tmpConfig.graphics.sharpenEnabled = (bool)valueIndex; });
    y += size.y + 10;

    addSelector("Potato Mode", "Disables basic graphics features to reduce system requirements.",
            { 0, y }, size, { "OFF", "ON" }, !tmpConfig.graphics.fogEnabled, [this](i32 valueIndex){
        tmpConfig.graphics.pointLightsEnabled = !(bool)valueIndex;
        tmpConfig.graphics.motionBlurEnabled = !(bool)valueIndex;
        tmpConfig.graphics.fogEnabled = !(bool)valueIndex;
    });
    y += size.y + 10;

    y += 20;
    addHelpMessage({0, y});

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
    fadeInTimer = 400.f;

    addBackgroundBox({0,0}, {400, 400}, 0.8f);

    Font* font = &g_res.getFont("font_bold", (u32)convertSize(55));
    addLabel([]{ return "PAUSED"; }, {0, -160}, font);
    glm::vec2 size(350, 80);
    selectedWidget = addButton("RESUME", "", {0, -70}, size, [this]{
        reset();
        g_game.currentScene->setPaused(false);
    }, WidgetFlags::BACK);
    addButton("FORFEIT RACE", "", {0, 30}, size, [this]{
        reset();
        g_game.currentScene->setPaused(false);
        g_game.currentScene->forfeitRace();
    });
    addButton("QUIT TO DESKTOP", "", {0, 130}, size, []{
        g_game.shouldExit = true;
    });
}

void Menu::drawBox(glm::vec2 pos, glm::vec2 size)
{
    f32 border = glm::floor(convertSize720(3));
    ui::rectBlur(ui::BORDER, nullptr, pos-border, size+border*2, glm::vec4(glm::vec3(1.f), 0.4f), 1.f);
    ui::rectBlur(ui::BG, nullptr, pos, size, glm::vec4(glm::vec3(0.f), 0.8f), 1.f);
}

void Menu::onUpdate(Renderer* renderer, f32 deltaTime)
{
    if (g_game.currentScene && !g_game.currentScene->isRaceInProgress && !g_game.isEditing)
    {
        Texture* tex = g_res.getTexture("checkers_fade");
        f32 w = (f32)g_game.windowWidth;
        f32 h = convertSize720(tex->height * 0.5f);
        ui::rectUVBlur(-200, tex, glm::vec2(0), {w,h}, glm::vec2(0.f, 0.999f),
                    glm::vec2(g_game.windowWidth/convertSize720(tex->width * 0.5f), 0.001f));
        ui::rectUVBlur(-200, tex, glm::vec2(0, g_game.windowHeight-h), {w,h},
                    glm::vec2(0.f, 0.001f),
                    glm::vec2(g_game.windowWidth/convertSize720(tex->width * 0.5f), 0.999f));
    }

    switch (menuMode)
    {
        case MenuMode::VISIBLE:
            break;
        case MenuMode::CHAMPIONSHIP_STANDINGS:
            championshipStandings();
            break;
        case MenuMode::RACE_RESULTS:
            raceResults();
            break;
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
        if (fadeIn && fadeInTimer > 500.f && (widget.flags & WidgetFlags::BACK) && didGoBack())
        {
            activated = true;
            selectedWidget = &widget;
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
                if (g_input.isMouseButtonPressed(MOUSE_LEFT)
                        && selectedWidget == &widget && fadeInTimer > 500.f)
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
            if (didSelect() && fadeIn && fadeInTimer > 500.f)
            {
                activated = true;
            }
        }

        if (activated)
        {
            if (widget.flags & WidgetFlags::CANNOT_ACTIVATE)
            {
                g_audio.playSound(g_res.getSound("nono"), SoundType::MENU_SFX);
            }
            else
            {
#if 0
                if (widget.flags & WidgetFlags::BACK)
                {
                    g_audio.playSound(g_res.getSound("close"), SoundType::MENU_SFX);
                }
                else
                {
                    g_audio.playSound(g_res.getSound("click"), SoundType::MENU_SFX);
                }
#else
                g_audio.playSound(g_res.getSound("click"), SoundType::MENU_SFX);
#endif
                if (widget.flags & (WidgetFlags::FADE_OUT | WidgetFlags::FADE_OUT_TRANSIENT))
                {
                    fadeIn = false;
                    fadeInTimer = REFERENCE_HEIGHT;
                }
                else
                {
                    activatedWidget = &widget;
                }
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
            if (!(selectedWidget->flags & WidgetFlags::FADE_OUT_TRANSIENT) ||
                ((selectedWidget->flags & WidgetFlags::FADE_OUT_TRANSIENT) && (widget.flags & WidgetFlags::TRANSIENT)))
            {
                if (fadeInTimer < widget.pos.y + REFERENCE_HEIGHT * 0.5f)
                {
                    widget.fadeInAlpha = smoothMove(widget.fadeInAlpha, 0.f, 7.f, deltaTime);
                    widget.fadeInAlpha = clamp(widget.fadeInAlpha - deltaTime * 3.f, 0.f, 1.f);
                    widget.fadeInScale = smoothMove(widget.fadeInScale, 0.f, 5.f, deltaTime);
                }
            }
        }
    }

    if (blackFadeAlpha > 0.f)
    {
        ui::rectBlur(10000, &g_res.white, {0,0}, {g_game.windowWidth, g_game.windowHeight},
                    glm::vec4(0,0,0,1), blackFadeAlpha);
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
    else if (fadeIn && selectedWidget)
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
