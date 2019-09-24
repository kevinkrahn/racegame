#include "gui.h"
#include "renderer.h"
#include "game.h"
#include "input.h"
#include "2d.h"

void Gui::beginFrame()
{
    fontSmall = &g_resources.getFont("font", convertSize(18));
    fontBig = &g_resources.getFont("font", convertSize(28));
    white = g_resources.getTexture("white");
    renderer = g_game.renderer.get();
    isMouseOverUI = false;
    isMouseClickHandled = false;
    isKeyboardInputHandled = false;
    if (g_input.isMouseButtonReleased(MOUSE_LEFT))
    {
        isMouseCaptured = false;
    }
}

void Gui::endFrame()
{
    assert(widgetStack.empty());
}

f32 Gui::convertSize(f32 size)
{
    f32 baseHeight = 720.f;
    return size * (g_game.windowHeight / baseHeight);
}

WidgetState* Gui::getWidgetState(WidgetState* parent, std::string const& identifier, WidgetType widgetType)
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

i32 Gui::didChangeSelection()
{
    i32 result = (i32)(g_input.isKeyPressed(KEY_RIGHT, true) || didSelect())
                - (i32)g_input.isKeyPressed(KEY_LEFT, true);

    for (auto& pair : g_input.getControllers())
    {
        i32 tmpResult = pair.second.isButtonPressed(BUTTON_DPAD_RIGHT) -
                        pair.second.isButtonPressed(BUTTON_DPAD_LEFT);
        if (tmpResult)
        {
            result = tmpResult;
            break;
        }
    }

    return result;
}

void Gui::beginPanel(std::string const& text, glm::vec2 position, f32 height,
            f32 halign, bool solidBackground, i32 buttonCount, bool showTitle,
            f32 itemHeight, f32 itemSpacing, f32 panelWidth)
{
    f32 width = convertSize(panelWidth);
    height = convertSize(height);
    position.x -= width * halign;
    if (solidBackground)
    {
        renderer->push(QuadRenderable(white,
                    position, width, height,
                    glm::vec3(0.02f), 0.8f, true));
    }
    if (showTitle)
    {
        renderer->push(TextRenderable(fontBig, text,
                    position + glm::vec2(width/2, convertSize(10.f)),
                    glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::CENTER, VerticalAlign::TOP));
    }
    i32 selectIndex = 0;
    WidgetState* panelState = getWidgetState(nullptr, text, WidgetType::PANEL);
    if (buttonCount >= 0)
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

        panelState->selectIndex += move;
        if (panelState->selectIndex < 0)
        {
            panelState->selectIndex = buttonCount - 1;
        }
        if (panelState->selectIndex >= buttonCount)
        {
            panelState->selectIndex = 0;
        }

        selectIndex = panelState->selectIndex;
    }
    widgetStack.push_back({
        WidgetType::PANEL,
        0,
        position,
        { width, height },
        showTitle ? position + glm::vec2(0, convertSize(40.f)) : position,
        buttonCount > 0,
        panelState,
        nullptr,
        glm::floor(convertSize(itemHeight)),
        glm::floor(convertSize(itemSpacing)),
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

bool Gui::button(std::string const& text, bool active)
{
    assert(widgetStack.size() > 0);
    assert(widgetStack.back().widgetType == WidgetType::PANEL);

    WidgetStackItem& parent = widgetStack.back();
    WidgetState* widgetState = getWidgetState(parent.widgetState, text, WidgetType::BUTTON);

    glm::vec2& pos = parent.nextWidgetPosition;
    f32 bh = parent.itemHeight;
    f32 bw = parent.size.x;

    renderer->push(QuadRenderable(white,
                pos, bw, bh, glm::vec3(0.f), active ? 0.85f : 0.65f, true));

    bool selected = false;
    bool clicked = false;
    glm::vec2 mousePos = g_input.getMousePosition();
    bool canHover = g_input.didMouseMove() || !widgetStack.back().hasKeyboardSelect;
    if ((g_input.isMouseButtonPressed(MOUSE_LEFT) || canHover)
            && pointInRectangle(mousePos, pos, pos + glm::vec2(bw, bh)))
    {
        isMouseOverUI = true;
        if (active)
        {
            parent.widgetState->selectIndex = parent.selectableChildCount;
            selected = true;
            if (!isMouseClickHandled && g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                isMouseClickHandled = true;
                clicked = true;
            }
        }
    }
    if (active && widgetStack.back().hasKeyboardSelect)
    {
        selected = (parent.selectableChildCount == parent.widgetState->selectIndex);
        if (!isKeyboardInputHandled && selected && didSelect())
        {
            clicked = true;
            isKeyboardInputHandled = true;
        }
    }

    ++parent.selectableChildCount;

    if (selected && active)
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
        f32 selectLineHeight = glm::floor(bh * 0.05f);
        renderer->push(QuadRenderable(white, pos, bw*widgetState->hoverIntensity,
                    selectLineHeight, glm::vec3(1.f), widgetState->hoverIntensity * 0.8f));
        renderer->push(QuadRenderable(white,
                    pos + glm::vec2(bw * (1.f - widgetState->hoverIntensity), bh - selectLineHeight),
                    bw*widgetState->hoverIntensity, selectLineHeight, glm::vec3(1.f),
                    widgetState->hoverIntensity * 0.8f));
    }

    renderer->push(TextRenderable(fontSmall, text, pos + glm::vec2(bw/2, bh/2),
                glm::vec3(1.f), active ? 1.f : 0.75f, 1.f,
                HorizontalAlign::CENTER, VerticalAlign::CENTER));

    pos.y += bh + parent.itemSpacing;

    return clicked;
}

bool Gui::toggle(std::string const& text, bool& enabled)
{
    assert(widgetStack.size() > 0);
    assert(widgetStack.back().widgetType == WidgetType::PANEL);

    WidgetStackItem& parent = widgetStack.back();
    glm::vec2& pos = parent.nextWidgetPosition;
    WidgetState* widgetState = getWidgetState(parent.widgetState, text, WidgetType::BUTTON);

    f32 bh = parent.itemHeight;
    f32 bw = parent.size.x;

    renderer->push(QuadRenderable(white,
                pos, bw, bh, glm::vec3(0.f), 0.85f, true));

    bool selected = false;
    bool clicked = false;
    glm::vec2 mousePos = g_input.getMousePosition();
    bool canHover = g_input.didMouseMove() || !widgetStack.back().hasKeyboardSelect;
    if ((g_input.isMouseButtonPressed(MOUSE_LEFT) || canHover)
            && pointInRectangle(mousePos, pos, pos + glm::vec2(bw, bh)))
    {
        isMouseOverUI = true;
        parent.widgetState->selectIndex = parent.selectableChildCount;
        selected = true;
        if (!isMouseClickHandled && g_input.isMouseButtonPressed(MOUSE_LEFT))
        {
            enabled = !enabled;
            isMouseClickHandled = true;
            clicked = true;
        }
    }
    if (widgetStack.back().hasKeyboardSelect)
    {
        selected = (parent.selectableChildCount == parent.widgetState->selectIndex);
        if (!isKeyboardInputHandled && selected && (didSelect() || didChangeSelection()))
        {
            enabled = !enabled;
            clicked = true;
            isKeyboardInputHandled = true;
        }
    }

    ++parent.selectableChildCount;

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
        f32 selectLineHeight = glm::floor(bh * 0.05f);
        renderer->push(QuadRenderable(white, pos, bw*widgetState->hoverIntensity,
                    selectLineHeight, glm::vec3(1.f), widgetState->hoverIntensity * 0.8f));
        renderer->push(QuadRenderable(white,
                    pos + glm::vec2(bw * (1.f - widgetState->hoverIntensity), bh - selectLineHeight),
                    bw*widgetState->hoverIntensity, selectLineHeight, glm::vec3(1.f),
                    widgetState->hoverIntensity * 0.8f));
    }

    renderer->push(TextRenderable(fontSmall, str(text, ": ", enabled ? "ON" : "OFF"),
                pos + glm::vec2(bw/2, bh/2), glm::vec3(1.f), 1.f, 1.f,
                HorizontalAlign::CENTER, VerticalAlign::CENTER));

    pos.y += bh + parent.itemSpacing;

    return clicked;
}

bool Gui::slider(std::string const& text, f32 minValue, f32 maxValue, f32& value)
{
    assert(widgetStack.size() > 0);
    assert(widgetStack.back().widgetType == WidgetType::PANEL);

    WidgetStackItem& parent = widgetStack.back();
    glm::vec2& pos = parent.nextWidgetPosition;
    WidgetState* widgetState = getWidgetState(parent.widgetState, text, WidgetType::BUTTON);

    f32 bh = parent.itemHeight;
    f32 bw = parent.size.x;

    renderer->push(QuadRenderable(white,
                pos, bw, bh, glm::vec3(0.f),  0.85f, true));

    bool selected = false;
    bool clicked = false;
    glm::vec2 mousePos = g_input.getMousePosition();
    bool canHover = g_input.didMouseMove() || !widgetStack.back().hasKeyboardSelect;
    if ((g_input.isMouseButtonPressed(MOUSE_LEFT) || canHover)
            && pointInRectangle(mousePos, pos, pos + glm::vec2(bw, bh)))
    {
        isMouseOverUI = true;
        parent.widgetState->selectIndex = parent.selectableChildCount;
        selected = true;
        // TODO: should the mouse scroll wheel change the value?
        if (!isMouseClickHandled && g_input.isMouseButtonDown(MOUSE_LEFT))
        {
            isMouseCaptured = true;
            isMouseClickHandled = true;
            f32 t = (mousePos.x - pos.x) / bw;
            value = minValue + (maxValue - minValue) * t;
            clicked = true;
        }
    }
    if (widgetStack.back().hasKeyboardSelect)
    {
        selected = (parent.selectableChildCount == parent.widgetState->selectIndex);
        f32 valChange = didChangeSelection() * ((maxValue - minValue) / 20.f);
        if (!isKeyboardInputHandled && selected && valChange != 0.f)
        {
            clicked = true;
            isKeyboardInputHandled = true;
            value = clamp(value + valChange, minValue, maxValue);
        }
    }

    ++parent.selectableChildCount;

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
        f32 selectLineHeight = glm::floor(bh * 0.05f);
        renderer->push(QuadRenderable(white, pos, bw*widgetState->hoverIntensity,
                    selectLineHeight, glm::vec3(1.f), widgetState->hoverIntensity * 0.8f));
        renderer->push(QuadRenderable(white,
                    pos + glm::vec2(bw * (1.f - widgetState->hoverIntensity), bh - selectLineHeight),
                    bw*widgetState->hoverIntensity, selectLineHeight, glm::vec3(1.f),
                    widgetState->hoverIntensity * 0.8f));
    }
    f32 valueHeight = glm::floor(bh * 0.1f);
    renderer->push(QuadRenderable(white, pos + glm::vec2(0, valueHeight),
                bw * ((value - minValue) / (maxValue - minValue)), bh - valueHeight * 2.f,
            value >= 0.f ? glm::vec3(0.0f, 0.f, 0.9f) : glm::vec3(0.9f, 0.f, 0.f), 0.4f));

    renderer->push(TextRenderable(fontSmall, str(text, ": ", std::fixed, std::setprecision(2), value),
                pos + glm::vec2(bw/2, bh/2),
                glm::vec3(1.f), 1.f, 1.f,
                HorizontalAlign::CENTER, VerticalAlign::CENTER));

    pos.y += bh + parent.itemSpacing;

    return clicked;
}

void Gui::beginSelect(std::string const& text, i32* selectedIndex, bool showTitle)
{
    assert(widgetStack.size() > 0);
    assert(widgetStack.back().widgetType == WidgetType::PANEL);

    WidgetStackItem& parent = widgetStack.back();

    if (showTitle)
    {
        renderer->push(TextRenderable(fontSmall, text,
                    parent.nextWidgetPosition + glm::vec2(parent.size.x / 2, convertSize(5.f)),
                    glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::CENTER, VerticalAlign::TOP));
    }

    WidgetState* selectState = getWidgetState(parent.widgetState, text, WidgetType::SELECT);
    widgetStack.push_back({
        WidgetType::SELECT,
        0,
        parent.nextWidgetPosition,
        parent.size,
        showTitle ? parent.nextWidgetPosition
            + glm::vec2(0, convertSize(20.f)) : parent.nextWidgetPosition,
        false,
        selectState,
        selectedIndex,
        glm::floor(parent.itemHeight * 0.75f),
        parent.itemSpacing,
    });
}

bool Gui::option(std::string const& text, i32 value, const char* icon)
{
    assert(widgetStack.size() > 0);
    assert(widgetStack.back().widgetType == WidgetType::SELECT);

    WidgetStackItem& parent = widgetStack.back();
    glm::vec2& pos = parent.nextWidgetPosition;
    WidgetState* widgetState = getWidgetState(parent.widgetState, text, WidgetType::BUTTON);

    f32 bh = parent.itemHeight;
    f32 bw = parent.size.x;

    bool selected = false;
    bool clicked = false;
    glm::vec2 mousePos = g_input.getMousePosition();
    if (pointInRectangle(mousePos, pos, pos + glm::vec2(bw, bh)))
    {
        isMouseOverUI = true;
        selected = true;
        if (!isMouseClickHandled && g_input.isMouseButtonPressed(MOUSE_LEFT))
        {
            *parent.outputValueI32 = value;
            isMouseClickHandled = true;
            clicked = true;
        }
    }

    glm::vec3 col = glm::vec3(*parent.outputValueI32 == value ? 0.06f : 0.f);
    renderer->push(QuadRenderable(white, pos, bw, bh, col, 0.85f, true));

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
        f32 selectLineHeight = glm::floor(bh * 0.05f);
        renderer->push(QuadRenderable(white, pos, bw*widgetState->hoverIntensity,
                    selectLineHeight, glm::vec3(1.f), widgetState->hoverIntensity * 0.8f));
        renderer->push(QuadRenderable(white,
                    pos + glm::vec2(bw * (1.f - widgetState->hoverIntensity), bh - selectLineHeight),
                    bw*widgetState->hoverIntensity, selectLineHeight, glm::vec3(1.f),
                    widgetState->hoverIntensity * 0.8f));
    }

    if (icon)
    {
        Texture* iconTexture = g_resources.getTexture(icon);
        u32 iconSize = bh * 0.5f;
        renderer->push(QuadRenderable(iconTexture,
                    pos + glm::vec2(bh * 0.25), iconSize, iconSize));
    }

    renderer->push(TextRenderable(fontSmall, text,
                pos + glm::vec2(icon ? bh : convertSize(4.f), bh/2),
                glm::vec3(1.f), 1.f, 1.f,
                HorizontalAlign::LEFT, VerticalAlign::CENTER));

    pos.y += bh;

    return clicked;
}

void Gui::label(std::string const& text)
{
    WidgetStackItem& parent = widgetStack.back();
    glm::vec2& pos = parent.nextWidgetPosition;
    f32 bh = parent.itemHeight;
    f32 bw = parent.size.x;
    renderer->push(QuadRenderable(white, pos, bw, bh, glm::vec3(0.f), 0.85f, true));
    renderer->push(TextRenderable(fontSmall, text, pos + glm::vec2(bw/2, bh/2),
                glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::CENTER, VerticalAlign::CENTER));
    pos.y += bh + parent.itemSpacing;
}

i32 Gui::select(std::string const& text, std::string* firstValue,
        i32 count, i32& currentIndex)
{
    assert(widgetStack.size() > 0);
    assert(widgetStack.back().widgetType == WidgetType::PANEL);

    WidgetStackItem& parent = widgetStack.back();
    glm::vec2& pos = parent.nextWidgetPosition;
    WidgetState* widgetState = getWidgetState(parent.widgetState, text, WidgetType::BUTTON);

    f32 bh = parent.itemHeight;
    f32 bw = parent.size.x;

    bool selected = false;
    bool clicked = false;
    glm::vec2 mousePos = g_input.getMousePosition();
    bool canHover = g_input.didMouseMove() || !widgetStack.back().hasKeyboardSelect;
    if ((g_input.isMouseButtonPressed(MOUSE_LEFT) || canHover)
            && pointInRectangle(mousePos, pos, pos + glm::vec2(bw, bh)))
    {
        isMouseOverUI = true;
        parent.widgetState->selectIndex = parent.selectableChildCount;
        selected = true;
        if (!isMouseClickHandled && g_input.isMouseButtonDown(MOUSE_LEFT))
        {
            isMouseCaptured = true;
            isMouseClickHandled = true;
            clicked = true;
            if (mousePos.x > pos.x + bw / 2)
            {
                ++currentIndex;
            }
            else
            {
                --currentIndex;
            }
        }
    }
    if (widgetStack.back().hasKeyboardSelect)
    {
        selected = (parent.selectableChildCount == parent.widgetState->selectIndex);
        i32 valChange = didChangeSelection();
        if (!isKeyboardInputHandled && selected && valChange != 0.f)
        {
            clicked = true;
            isKeyboardInputHandled = true;
            currentIndex += valChange;
        }
    }
    if (currentIndex < 0)
    {
        currentIndex = count - 1;
    }
    else if (currentIndex >= count)
    {
        currentIndex = 0;
    }

    ++parent.selectableChildCount;

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

    renderer->push(QuadRenderable(white,
                pos, bw, bh, glm::vec3(0.f),  0.85f, true));

    if (widgetState->hoverIntensity > 0.f)
    {
        f32 selectLineHeight = glm::floor(bh * 0.05f);
        renderer->push(QuadRenderable(white, pos, bw*widgetState->hoverIntensity,
                    selectLineHeight, glm::vec3(1.f), widgetState->hoverIntensity * 0.8f));
        renderer->push(QuadRenderable(white,
                    pos + glm::vec2(bw * (1.f - widgetState->hoverIntensity), bh - selectLineHeight),
                    bw*widgetState->hoverIntensity, selectLineHeight, glm::vec3(1.f),
                    widgetState->hoverIntensity * 0.8f));

        Texture* cheveron = g_resources.getTexture("cheveron");
        if (currentIndex > 0)
        {
            renderer->push(QuadRenderable(cheveron, pos + glm::vec2(bh*0.25f), bh*0.5f, bh*0.5f,
                        glm::vec3(1.f), widgetState->hoverIntensity, false, false));
        }
        if (currentIndex < count - 1)
        {
            renderer->push(QuadRenderable(cheveron, pos + glm::vec2(bw-bh*0.75f , bh*0.25f),
                        bh*0.5f, bh*0.5f, glm::vec3(1.f), widgetState->hoverIntensity,
                        false, true));
        }
    }

    renderer->push(TextRenderable(fontSmall, str(text, ": ", *(firstValue + currentIndex)),
                pos + glm::vec2(bw/2, bh/2),
                glm::vec3(1.f), 1.f, 1.f,
                HorizontalAlign::CENTER, VerticalAlign::CENTER));

    pos.y += bh + parent.itemSpacing;

    return clicked ? currentIndex : -1;
}
