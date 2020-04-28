#include "gui.h"
#include "renderer.h"
#include "game.h"
#include "input.h"
#include "2d.h"
#include "audio.h"
#include "driver.h"

void Gui::beginFrame()
{
    fontTiny = &g_res.getFont("font", (u32)convertSize(13));
    fontSmall = &g_res.getFont("font", (u32)convertSize(16));
    fontBig = &g_res.getFont("font_bold", (u32)convertSize(26));
    white = &g_res.white;
    renderer = g_game.renderer.get();
    isMouseOverUI = false;
    isMouseClickHandled = false;
    isKeyboardInputHandled = false;
    isKeyboardInputCaptured = false;
    previousTextInputCapture = textInputCapture;
    textInputCapture = nullptr;
}

void Gui::pushSelection()
{
    assert(widgetStack.size() > 0);
    assert(widgetStack.back().widgetType == WidgetType::PANEL);
    selectStack.push_back(widgetStack.back().widgetState->selectIndex);
    //print("Pushed ", selectStack.back(), '\n');
}

void Gui::popSelection()
{
    if (!selectStack.empty())
    {
        poppedSelectIndex = selectStack.back();
        selectStack.pop_back();
        //print("Popped ", poppedSelectIndex, '\n');
    }
}

void Gui::clearSelectionStack()
{
    selectStack.clear();
    //print("Cleared stack\n");
}
void Gui::forceSelection(i32 selection)
{
    poppedSelectIndex = selection;
    //print("Forced selection: ", selection, "\n");
}

void Gui::endFrame()
{
    assert(widgetStack.empty());
    if (g_input.isMouseButtonReleased(MOUSE_LEFT))
    {
        isMouseCaptured = false;
    }
    if (textInputCapture != previousTextInputCapture)
    {
        if (textInputCapture)
        {
            g_input.beginTextInput();
        }
        else
        {
            g_input.stopTextInput();
        }
    }
}

f32 Gui::convertSize(f32 size)
{
    f32 baseHeight = 720.f;
    return size * (g_game.windowHeight / baseHeight);
}

f32 Gui::convertSizei(f32 size)
{
    return glm::floor(convertSize(size));
}

WidgetState* Gui::getWidgetState(WidgetState* parent, const char* identifier,
        WidgetType widgetType)
{
    decltype(childState)& childStateMap = parent ? parent->childState : childState;
    auto it = childStateMap.find(identifier);
    if (it == childStateMap.end())
    {
        std::unique_ptr<WidgetState> widgetState = std::make_unique<WidgetState>();
        widgetState->widgetType = widgetType;
        widgetState->frameCount = g_game.frameCount;
        childStateMap[identifier] = std::move(widgetState);
        return childStateMap[identifier].get();
    }
    if (it->second->frameCount + 1 != g_game.frameCount)
    {
        childStateMap.erase(identifier);
        std::unique_ptr<WidgetState> widgetState = std::make_unique<WidgetState>();
        widgetState->widgetType = widgetType;
        widgetState->frameCount = g_game.frameCount;
        childStateMap[identifier] = std::move(widgetState);
        return childStateMap[identifier].get();
    }
    assert(it->second->widgetType == widgetType);
    it->second->frameCount = g_game.frameCount;
    return it->second.get();
}

void Gui::clearWidgetState(const char* text)
{
    assert(widgetStack.size() > 0);
    assert(widgetStack.back().widgetType == WidgetType::PANEL);
    WidgetStackItem& parent = widgetStack.back();
    parent.widgetState->childState.erase(text);
}

bool Gui::didSelect()
{
    if (isKeyboardInputHandled)
    {
        return false;
    }
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

bool Gui::didGoBack()
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

i32 Gui::didChangeSelection(WidgetState* panelState)
{
    i32 result = (i32)(g_input.isKeyPressed(KEY_RIGHT, true) || didSelect())
                - (i32)g_input.isKeyPressed(KEY_LEFT, true);

    for (auto& pair : g_input.getControllers())
    {
        i32 tmpResult = pair.second.isButtonPressed(BUTTON_DPAD_RIGHT) -
                        pair.second.isButtonPressed(BUTTON_DPAD_LEFT);
        if (!tmpResult)
        {
            if (panelState->repeatTimer == 0.f)
            {
                f32 xaxis = pair.second.getAxis(AXIS_LEFT_X);
                if (xaxis < -0.2f)
                {
                    tmpResult = -1;
                    panelState->repeatTimer = 0.2f;
                }
                else if (xaxis > 0.2f)
                {
                    tmpResult = 1;
                    panelState->repeatTimer = 0.2f;
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

void Gui::beginPanel(const char* text, glm::vec2 position, f32 halign,
        bool solidBackground, bool useKeyboardControl, bool showTitle,
        f32 itemHeight, f32 itemSpacing, f32 panelWidth)
{
    f32 width = convertSize(panelWidth);
    position.x -= width * halign;
    if (showTitle)
    {
        renderer->push2D(TextRenderable(fontBig, text,
                    position + glm::vec2(width/2, convertSize(10.f)),
                    glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::CENTER, VerticalAlign::TOP));
    }
    WidgetState* panelState = getWidgetState(nullptr, text, WidgetType::PANEL);
    if (useKeyboardControl)
    {
        panelState->repeatTimer = glm::max(panelState->repeatTimer - g_game.deltaTime, 0.f);
        i32 move = (i32)g_input.isKeyPressed(KEY_DOWN, true)
                 - (i32)g_input.isKeyPressed(KEY_UP, true);
        for (auto& pair : g_input.getControllers())
        {
            if (panelState->repeatTimer == 0.f)
            {
                f32 yaxis = pair.second.getAxis(AXIS_LEFT_Y);
                if (pair.second.isButtonDown(BUTTON_DPAD_UP) || yaxis < -0.2f)
                {
                    move = -1;
                    panelState->repeatTimer = 0.2f;
                }
                else if (pair.second.isButtonDown(BUTTON_DPAD_DOWN) || yaxis > 0.2f)
                {
                    move = 1;
                    panelState->repeatTimer = 0.2f;
                }
            }
        }

        if (panelState->selectableChildCount > 0)
        {
            if (move != 0)
            {
                g_audio.playSound(g_res.getSound("select"), SoundType::MENU_SFX);
            }
            panelState->selectIndex += move;
            if (panelState->selectIndex < 0)
            {
                panelState->selectIndex = panelState->selectableChildCount - 1;
            }
            if (panelState->selectIndex >= panelState->selectableChildCount)
            {
                panelState->selectIndex = 0;
            }
        }
        panelState->selectableChildCount = 0;
    }
    if (poppedSelectIndex != -1)
    {
        panelState->selectIndex = poppedSelectIndex;
        poppedSelectIndex = -1;
    }
    widgetStack.push_back({
        WidgetType::PANEL,
        position,
        { width, 0.f },
        showTitle ? position + glm::vec2(0, convertSize(42.f)) : position,
        useKeyboardControl,
        panelState,
        nullptr,
        glm::floor(convertSize(itemHeight)),
        glm::floor(convertSize(itemSpacing)),
        solidBackground
    });
}

void Gui::end()
{
    assert(widgetStack.size() > 0);

    WidgetStackItem w = std::move(widgetStack.back());
    widgetStack.pop_back();
    switch(w.widgetType)
    {
        case WidgetType::PANEL:
            if (w.solidBackground)
            {
                renderer->push2D(QuadRenderable(white, w.position, w.size.x,
                            w.nextWidgetPosition.y - w.position.y,
                            glm::vec3(0.f), 0.8f, true), -1);
            }
            if (pointInRectangle(g_input.getMousePosition(), w.position,
                        w.position + glm::vec2(w.size.x, w.nextWidgetPosition.y - w.position.y)))
            {
                isMouseOverUI = true;
            }
            break;
        case WidgetType::SELECT:
            assert(widgetStack.back().widgetType == WidgetType::PANEL);
            widgetStack.back().nextWidgetPosition =
                w.nextWidgetPosition + glm::vec2(0, w.itemSpacing * 2.f);
            break;
        default:
            break;
    }
}

bool Gui::buttonBase(WidgetStackItem& parent, WidgetState* widgetState, glm::vec2 pos,
        f32 bw, f32 bh, std::function<bool()> onHover, std::function<bool()> onSelected,
        bool active)
{
    renderer->push2D(QuadRenderable(white,
                pos, bw, bh, glm::vec3(0.f), active ? 0.9f : 0.7f, true));

    bool selected = false;
    bool clicked = false;
    glm::vec2 mousePos = g_input.getMousePosition();
    bool canHover = (g_input.didMouseMove() || !widgetStack.back().useKeyboardControl);
        //&& !isMouseCaptured;
    if ((g_input.isMouseButtonPressed(MOUSE_LEFT) || canHover)
            && pointInRectangle(mousePos, pos, pos + glm::vec2(bw, bh)))
    {
        isMouseOverUI = true;
        if (!widgetState->isMouseOver)
        {
            g_audio.playSound(g_res.getSound("select"), SoundType::MENU_SFX);
            widgetState->isMouseOver = true;
        }
        parent.widgetState->selectIndex = parent.widgetState->selectableChildCount;
        selected = true;
        if (!isMouseClickHandled && active)
        {
            if (onHover())
            {
                isMouseClickHandled = true;
                isMouseCaptured = true;
                clicked = true;
                g_audio.playSound(g_res.getSound("click"), SoundType::MENU_SFX);
            }
        }
    }
    if (parent.widgetState->selectIndex != parent.widgetState->selectableChildCount)
    {
        widgetState->isMouseOver = false;
    }
    if (widgetStack.back().useKeyboardControl)
    {
        selected =
            (parent.widgetState->selectableChildCount == parent.widgetState->selectIndex);
        if (!isKeyboardInputHandled && selected && active)
        {
            if (onSelected())
            {
                clicked = true;
                isKeyboardInputHandled = true;
                g_audio.playSound(g_res.getSound("click"), SoundType::MENU_SFX);
            }
        }
    }

    ++parent.widgetState->selectableChildCount;

    if (selected)
    {
        widgetState->hoverIntensity =
            glm::min(widgetState->hoverIntensity + g_game.deltaTime * 4.f, 1.f);
    }
    else
    {
        widgetState->hoverIntensity =
            glm::max(widgetState->hoverIntensity - g_game.deltaTime * 4.f, 0.f);
    }

    if (widgetState->hoverIntensity > 0.f)
    {
        f32 lineAlpha = 0.8f;
        if (!active)
        {
            lineAlpha = 0.08f;
        }
        lineAlpha *= widgetState->hoverIntensity;
        f32 selectLineHeight = glm::floor(convertSize(2));
        renderer->push2D(QuadRenderable(white, pos, bw*widgetState->hoverIntensity,
                    selectLineHeight, glm::vec3(1.f), lineAlpha));
        renderer->push2D(QuadRenderable(white,
                    pos + glm::vec2(bw * (1.f - widgetState->hoverIntensity), bh - selectLineHeight),
                    bw*widgetState->hoverIntensity, selectLineHeight, glm::vec3(1.f), lineAlpha));
    }

    return clicked;
}

bool Gui::button(const char* text, bool active, Texture* icon, bool iconbg)
{
    assert(widgetStack.size() > 0);
    assert(widgetStack.back().widgetType == WidgetType::PANEL);

    WidgetStackItem& parent = widgetStack.back();
    WidgetState* widgetState = getWidgetState(parent.widgetState, text, WidgetType::BUTTON);

    glm::vec2& pos = parent.nextWidgetPosition;
    f32 bh = parent.itemHeight;
    f32 bw = parent.size.x;
    bool clicked = buttonBase(parent, widgetState, pos, bw, bh, [] {
        return g_input.isMouseButtonPressed(MOUSE_LEFT);
    }, [this] {
        return didSelect();
    }, active);

    if (icon)
    {
        renderer->push2D(TextRenderable(fontSmall, text,
                    pos + glm::vec2(icon ? bh : convertSize(4.f), bh/2),
                    glm::vec3(1.f), active ? 1.f : 0.5f, 1.f,
                    HorizontalAlign::LEFT, VerticalAlign::CENTER));

        f32 iconSize = glm::floor(bh * 0.8f);
        if (iconbg)
        {
            renderer->push2D(QuadRenderable(g_res.getTexture("iconbg"),
                        pos + glm::vec2(bh * 0.1f), iconSize, iconSize));
        }
        renderer->push2D(QuadRenderable(icon,
                    pos + glm::vec2(bh * 0.1f), iconSize, iconSize));
    }
    else
    {
        renderer->push2D(TextRenderable(fontSmall, text, pos + glm::vec2(bw/2, bh/2),
                    glm::vec3(1.f), active ? 1.f : 0.5f, 1.f,
                    HorizontalAlign::CENTER, VerticalAlign::CENTER));
    }

    pos.y += bh + parent.itemSpacing;

    return clicked;
}

bool Gui::itemButton(const char* text, const char* smallText, const char* extraText,
        bool active, Texture* icon, bool* isSelected, bool showIconBackground)
{
    assert(widgetStack.size() > 0);
    assert(widgetStack.back().widgetType == WidgetType::PANEL);

    if (isSelected) *isSelected = false;

    WidgetStackItem& parent = widgetStack.back();
    WidgetState* widgetState = getWidgetState(parent.widgetState, text, WidgetType::BUTTON);

    glm::vec2& pos = parent.nextWidgetPosition;
    f32 bh = parent.itemHeight;
    f32 bw = parent.size.x;
    bool clicked = buttonBase(parent, widgetState, pos, bw, bh, [] {
        return g_input.isMouseButtonPressed(MOUSE_LEFT);
    }, [&] {
        return didSelect();
    }, active);

    if (isSelected)
    {
        *isSelected =
            (parent.widgetState->selectableChildCount - 1 == parent.widgetState->selectIndex);
    }

    renderer->push2D(TextRenderable(fontSmall, text,
                pos + glm::vec2(bh, glm::floor(convertSize(5.f))),
                glm::vec3(1.f), active ? 1.f : 0.5f, 1.f,
                HorizontalAlign::LEFT, VerticalAlign::TOP));

    if (extraText)
    {
        renderer->push2D(TextRenderable(fontSmall, extraText,
                    pos + glm::vec2(bw - glm::floor(convertSize(4.f)), glm::floor(bh/2)),
                    glm::vec3(1.f), active ? 1.f : 0.5f, 1.f,
                    HorizontalAlign::RIGHT, VerticalAlign::CENTER));
    }

    renderer->push2D(TextRenderable(fontTiny, smallText,
                pos + glm::vec2(bh, glm::floor(convertSize(21.f))),
                glm::vec3(1.f), active ? 1.f : 0.5f, 1.f,
                HorizontalAlign::LEFT, VerticalAlign::TOP));

    f32 iconSize = glm::floor(bh * 0.8f);
    if (showIconBackground)
    {
        renderer->push2D(QuadRenderable(g_res.getTexture("iconbg"),
                    pos + glm::vec2(bh * 0.1f), iconSize, iconSize));
    }
    if (icon)
    {
        renderer->push2D(QuadRenderable(icon,
                    pos + glm::vec2(bh * 0.1f), iconSize, iconSize));
    }

    pos.y += bh + parent.itemSpacing;

    return clicked;
}

bool Gui::vehicleButton(const char* text, Texture* icon, Driver* driver)
{
    assert(widgetStack.size() > 0);
    assert(widgetStack.back().widgetType == WidgetType::PANEL);

    WidgetStackItem& parent = widgetStack.back();
    WidgetState* widgetState = getWidgetState(parent.widgetState, text, WidgetType::BUTTON);

    glm::vec2& pos = parent.nextWidgetPosition;
    f32 bh = glm::floor(convertSize(68));
    f32 bw = parent.size.x;
    bool clicked = buttonBase(parent, widgetState, pos, bw, bh, [] {
        return g_input.isMouseButtonPressed(MOUSE_LEFT);
    }, [this] {
        return didSelect();
    });

    f32 iconSize = glm::floor(convertSize(64));
    renderer->push2D(TextRenderable(fontSmall, text, pos +
                glm::vec2(glm::floor(iconSize + convertSize(8.f)), glm::floor(convertSize(6.f))),
                glm::vec3(1.f), 1.f, 1.f,
                HorizontalAlign::LEFT, VerticalAlign::TOP));
    renderer->push2D(TextRenderable(fontTiny, tstr("Credits: ", driver->credits), pos +
                glm::vec2(glm::floor(iconSize + convertSize(8.f)), glm::floor(convertSize(26.f))),
                glm::vec3(1.f), 1.f, 1.f,
                HorizontalAlign::LEFT, VerticalAlign::TOP));
    if (driver->vehicleIndex == -1)
    {
        renderer->push2D(TextRenderable(fontTiny, "You need to buy a vehicle!", pos +
                    glm::vec2(glm::floor(iconSize + convertSize(8.f)),
                        glm::floor(convertSize(42.f))),
                    glm::vec3(1.f, 0.f, 0.f), 1.f, 1.f,
                    HorizontalAlign::LEFT, VerticalAlign::TOP));
    }

    renderer->push2D(QuadRenderable(icon,
                pos + glm::vec2(convertSize(2)), iconSize, iconSize, glm::vec3(1.f), 1.f, false, true));

    pos.y += bh + parent.itemSpacing;

    return clicked;
}

bool Gui::toggle(const char* text, bool& enabled)
{
    assert(widgetStack.size() > 0);
    assert(widgetStack.back().widgetType == WidgetType::PANEL);

    WidgetStackItem& parent = widgetStack.back();
    WidgetState* widgetState = getWidgetState(parent.widgetState, text, WidgetType::BUTTON);

    glm::vec2& pos = parent.nextWidgetPosition;
    f32 bh = parent.itemHeight;
    f32 bw = parent.size.x;
    bool clicked = buttonBase(parent, widgetState, pos, bw, bh, [] {
        return g_input.isMouseButtonPressed(MOUSE_LEFT);
    }, [this] {
        return didSelect();
    });
    if (clicked)
    {
        enabled = !enabled;
    }

    renderer->push2D(TextRenderable(fontSmall, tstr(text, ": ", enabled ? "ON" : "OFF"),
                pos + glm::vec2(bw/2, bh/2), glm::vec3(1.f), 1.f, 1.f,
                HorizontalAlign::CENTER, VerticalAlign::CENTER));

    pos.y += bh + parent.itemSpacing;

    return clicked;
}

bool Gui::slider(const char* text, f32 minValue, f32 maxValue, f32& value)
{
    assert(widgetStack.size() > 0);
    assert(widgetStack.back().widgetType == WidgetType::PANEL);

    WidgetStackItem& parent = widgetStack.back();
    glm::vec2& pos = parent.nextWidgetPosition;
    WidgetState* widgetState = getWidgetState(parent.widgetState, text, WidgetType::BUTTON);

    f32 bh = parent.itemHeight;
    f32 bw = parent.size.x;

    bool clicked = buttonBase(parent, widgetState, pos, bw, bh, [&] {
        if (g_input.isMouseButtonPressed(MOUSE_LEFT))
        {
            widgetState->mouseCaptured = true;
            return true;
        }
        return false;
    }, [&] {
        f32 valChange = didChangeSelection(parent.widgetState) * ((maxValue - minValue) / 20.f);
        if (valChange != 0.f)
        {
            value = clamp(value + valChange, minValue, maxValue);
            return true;
        }
        return false;
    });

    if (widgetState->mouseCaptured)
    {
        glm::vec2 mousePos = g_input.getMousePosition();
        f32 t = clamp((mousePos.x - pos.x) / bw, 0.f, 1.f);
        value = minValue + (maxValue - minValue) * t;

        if (g_input.isMouseButtonReleased(MOUSE_LEFT))
        {
            widgetState->mouseCaptured = false;
        }
    }

    f32 valueHeight = glm::floor(bh * 0.1f);
    renderer->push2D(QuadRenderable(white, pos + glm::vec2(0, valueHeight),
                bw * ((value - minValue) / (maxValue - minValue)), bh - valueHeight * 2.f,
            value >= 0.f ? glm::vec3(0.0f, 0.f, 0.9f) : glm::vec3(0.9f, 0.f, 0.f), 0.4f));

    renderer->push2D(TextRenderable(fontSmall, tstr(text, ": ", std::fixed, std::setprecision(2), value),
                pos + glm::vec2(bw/2, bh/2),
                glm::vec3(1.f), 1.f, 1.f,
                HorizontalAlign::CENTER, VerticalAlign::CENTER));

    pos.y += bh + parent.itemSpacing;

    return clicked;
}

void Gui::beginSelect(const char* text, i32* selectedIndex, bool showTitle)
{
    assert(widgetStack.size() > 0);
    assert(widgetStack.back().widgetType == WidgetType::PANEL);

    WidgetStackItem& parent = widgetStack.back();

    if (showTitle)
    {
        renderer->push2D(TextRenderable(fontTiny, text,
                    parent.nextWidgetPosition + glm::vec2(parent.size.x / 2, convertSize(5.f)),
                    glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::CENTER, VerticalAlign::TOP));
    }

    WidgetState* selectState = getWidgetState(parent.widgetState, text, WidgetType::SELECT);
    widgetStack.push_back({
        WidgetType::SELECT,
        parent.nextWidgetPosition,
        parent.size,
        showTitle ? parent.nextWidgetPosition
            + glm::vec2(0, convertSize(20.f)) : parent.nextWidgetPosition,
        false,
        selectState,
        selectedIndex,
        glm::floor(parent.itemHeight * 0.75f),
        parent.itemSpacing,
        parent.solidBackground
    });
}

bool Gui::option(const char* text, i32 value, Texture* icon)
{
    assert(widgetStack.size() > 0);
    assert(widgetStack.back().widgetType == WidgetType::SELECT);

    WidgetStackItem& parent = widgetStack.back();
    glm::vec2& pos = parent.nextWidgetPosition;
    WidgetState* widgetState = getWidgetState(parent.widgetState, text, WidgetType::BUTTON);

    f32 bh = parent.itemHeight;
    f32 bw = parent.size.x;

    bool clicked = buttonBase(parent, widgetState, pos, bw, bh, [] {
        return g_input.isMouseButtonPressed(MOUSE_LEFT);
    }, [] {
        return false;
    });

    if (clicked)
    {
        *parent.outputValueI32 = value;
    }

    if (*parent.outputValueI32 == value)
    {
        renderer->push2D(QuadRenderable(white, pos, bw, bh, glm::vec3(0.2f), 0.3f, true));
    }

    if (icon)
    {
        f32 iconSize = glm::floor(bh * 0.5f);
        renderer->push2D(QuadRenderable(icon,
                    pos + glm::vec2(bh * 0.25f), iconSize, iconSize));
    }

    renderer->push2D(TextRenderable(fontSmall, text,
                pos + glm::vec2(icon ? bh : convertSize(4.f), bh/2),
                glm::vec3(1.f), 1.f, 1.f,
                HorizontalAlign::LEFT, VerticalAlign::CENTER));

    pos.y += bh;

    return clicked;
}

void Gui::label(const char* text, bool showBackground)
{
    WidgetStackItem& parent = widgetStack.back();
    glm::vec2& pos = parent.nextWidgetPosition;
    f32 bh = parent.itemHeight;
    f32 bw = parent.size.x;
    if (showBackground)
    {
        renderer->push2D(QuadRenderable(white, pos, bw, bh, glm::vec3(0.f), 0.85f, true));
    }
    renderer->push2D(TextRenderable(fontTiny, text, pos + glm::vec2(bw/2, bh/2),
                glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::CENTER, VerticalAlign::CENTER));
    //pos.y += bh + parent.itemSpacing;
    pos.y += bh;
}

void Gui::gap(f32 size)
{
    WidgetStackItem& parent = widgetStack.back();
    glm::vec2& pos = parent.nextWidgetPosition;
    pos.y += convertSize(size);
}

i32 Gui::select(const char* text, std::string* firstValue,
        i32 count, i32& currentIndex)
{
    assert(widgetStack.size() > 0);
    assert(widgetStack.back().widgetType == WidgetType::PANEL);

    WidgetStackItem& parent = widgetStack.back();
    glm::vec2& pos = parent.nextWidgetPosition;
    WidgetState* widgetState = getWidgetState(parent.widgetState, text, WidgetType::BUTTON);

    f32 bh = parent.itemHeight;
    f32 bw = parent.size.x;

    bool clicked = buttonBase(parent, widgetState, pos, bw, bh, [&] {
        if (g_input.isMouseButtonPressed(MOUSE_LEFT))
        {
            glm::vec2 mousePos = g_input.getMousePosition();
            if (mousePos.x > pos.x + bw / 2)
            {
                ++currentIndex;
            }
            else
            {
                --currentIndex;
            }
            return true;
        }
        return false;
    }, [&] {
        i32 valChange = didChangeSelection(parent.widgetState);
        if (valChange != 0)
        {
            currentIndex += valChange;
            return true;
        }
        return false;
    });
    if (currentIndex < 0)
    {
        currentIndex = count - 1;
    }
    else if (currentIndex >= count)
    {
        currentIndex = 0;
    }

    if (widgetState->hoverIntensity > 0.f)
    {
        Texture* cheveron = g_res.getTexture("cheveron");
        if (currentIndex > 0)
        {
            renderer->push2D(QuadRenderable(cheveron, pos + glm::vec2(bh*0.25f), bh*0.5f, bh*0.5f,
                        glm::vec3(1.f), widgetState->hoverIntensity, false, false));
        }
        if (currentIndex < count - 1)
        {
            renderer->push2D(QuadRenderable(cheveron, pos + glm::vec2(bw-bh*0.75f , bh*0.25f),
                        bh*0.5f, bh*0.5f, glm::vec3(1.f), widgetState->hoverIntensity,
                        false, true));
        }
    }

    renderer->push2D(TextRenderable(fontSmall, tstr(text, ": ", *(firstValue + currentIndex)),
                pos + glm::vec2(bw/2, bh/2),
                glm::vec3(1.f), 1.f, 1.f,
                HorizontalAlign::CENTER, VerticalAlign::CENTER));

    pos.y += bh + parent.itemSpacing;

    return clicked ? currentIndex : -1;
}

bool Gui::textEdit(const char* text, std::string& value)
{
    assert(widgetStack.size() > 0);
    assert(widgetStack.back().widgetType == WidgetType::PANEL);

    WidgetStackItem& parent = widgetStack.back();
    WidgetState* widgetState = getWidgetState(parent.widgetState, text, WidgetType::BUTTON);

    glm::vec2& pos = parent.nextWidgetPosition;
    f32 bh = parent.itemHeight;
    f32 bw = parent.size.x;
    if (buttonBase(parent, widgetState, pos, bw, bh, [] {
        return g_input.isMouseButtonPressed(MOUSE_LEFT);
    }, [this] {
        return didSelect();
    }))
    {
        widgetState->captured = !widgetState->captured;
    }

    f32 textBgStart = glm::floor(convertSize(6));
    f32 valueHeight = glm::floor(bh * 0.2f);
    renderer->push2D(QuadRenderable(white, pos + glm::vec2(textBgStart, valueHeight),
            bw - (textBgStart * 2), bh - valueHeight * 2.f, glm::vec3(0.1f), 0.1f));

    f32 textStart = glm::floor(convertSize(8));
    //f32 maxWidth = bw - textStart * 2;
    bool selected =
        (parent.widgetState->selectableChildCount - 1 == parent.widgetState->selectIndex);

    bool wasChanged = false;
    if (selected)
    {
        if (widgetState->captured)
        {
            if (!g_input.getInputText().empty())
            {
                value += g_input.getInputText();
                wasChanged = true;
            }
            if (g_input.isKeyPressed(KEY_BACKSPACE, true) && !value.empty())
            {
                value.pop_back();
                wasChanged = true;
            }

            f32 blinkHeight = fontSmall->getHeight() * 1.25f;
            renderer->push2D(QuadRenderable(white, pos + glm::vec2(textStart +
                            fontSmall->stringDimensions(value.c_str()).x + convertSizei(2),
                            bh/2 - blinkHeight / 2),
                    convertSize(1), blinkHeight, glm::vec3(1.f), 0.5f));

            isKeyboardInputCaptured = true;
            isKeyboardInputHandled = true;
            textInputCapture = widgetState;
        }
    }
    else
    {
        widgetState->captured = false;
    }

    renderer->push2D(TextRenderable(fontSmall, tstr(value), pos
                + glm::vec2(textStart, glm::floor(bh/2)), glm::vec3(1.f), 1.f, 1.f,
                HorizontalAlign::LEFT, VerticalAlign::CENTER));

    pos.y += bh + parent.itemSpacing;

    return wasChanged;
}
