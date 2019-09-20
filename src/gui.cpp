#include "gui.h"
#include "renderer.h"
#include "game.h"
#include "input.h"
#include "2d.h"

void Gui::beginFrame()
{
    fontSmall = &g_resources.getFont("font", convertSize(20));
    fontBig = &g_resources.getFont("font", convertSize(30));
    white = g_resources.getTexture("white");
    renderer = g_game.renderer.get();
    isMouseOverUI = false;
    isMouseClickHandled = false;
    isKeyboardInputHandled = false;
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

void Gui::beginPanel(std::string const& text, glm::vec2 position, f32 height,
            f32 halign, bool solidBackground, i32 buttonCount)
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
    if (text.size() > 0)
    {
        renderer->push(TextRenderable(fontBig, text,
                    position + glm::vec2(width/2, convertSize(10.f)),
                    glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::CENTER, VerticalAlign::TOP));
    }
    i32 selectIndex = 0;
    WidgetState* panelState = getWidgetState(nullptr, text, WidgetType::PANEL);
    if (buttonCount >= 0)
    {
        if (g_input.isKeyPressed(KEY_UP, true))
        {
            --panelState->selectIndex;
            if (panelState->selectIndex < 0)
            {
                panelState->selectIndex = buttonCount - 1;
            }
        }

        if (g_input.isKeyPressed(KEY_DOWN, true))
        {
            ++panelState->selectIndex;
            if (panelState->selectIndex >= buttonCount)
            {
                panelState->selectIndex = 0;
            }
        }
        selectIndex = panelState->selectIndex;
    }
    widgetStack.push_back({
        WidgetType::PANEL,
        0,
        position,
        { width, height },
        position + glm::vec2(0, convertSize(40.f)),
        buttonCount > 0,
        panelState
    });
}

void Gui::end()
{
    assert(widgetStack.size() > 0);

    switch(widgetStack.back().widgetType)
    {
        case WidgetType::PANEL:
            break;
        default:
            break;
    }

    widgetStack.pop_back();
}

bool Gui::button(std::string const& text, bool active)
{
    assert(widgetStack.size() > 0);
    assert(widgetStack.back().widgetType == WidgetType::PANEL);

    WidgetStackItem& parent = widgetStack.back();
    glm::vec2& pos = parent.nextWidgetPosition;
    WidgetState* widgetState = getWidgetState(parent.widgetState, text, WidgetType::BUTTON);

    f32 bh = convertSize(buttonHeight);
    f32 bw = parent.size.x;

    renderer->push(QuadRenderable(white,
                pos, bw, bh, glm::vec3(0.f), 0.8f, true));

    bool selected = false;
    bool clicked = false;
    glm::vec2 mousePos = g_input.getMousePosition();
    if ((g_input.isMouseButtonPressed(MOUSE_LEFT) || g_input.didMouseMove())
            && pointInRectangle(mousePos, pos, pos + glm::vec2(bw, bh)))
    {
        parent.widgetState->selectIndex = parent.selectableChildCount;
        selected = true;
        isMouseOverUI = true;
        if (!isMouseClickHandled && g_input.isMouseButtonPressed(MOUSE_LEFT))
        {
            isMouseClickHandled = true;
            clicked = true;
        }
    }
    if (widgetStack.back().hasKeyboardSelect)
    {
        selected = (parent.selectableChildCount == parent.widgetState->selectIndex);
        if (!isKeyboardInputHandled && selected && g_input.isKeyPressed(KEY_RETURN))
        {
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
        renderer->push(QuadRenderable(white, pos, bw*widgetState->hoverIntensity,
                    bh * 0.05f, glm::vec3(1.f), widgetState->hoverIntensity * 0.8f));
        renderer->push(QuadRenderable(white,
                    pos + glm::vec2(bw * (1.f - widgetState->hoverIntensity), bh - bh * 0.05f),
                    bw*widgetState->hoverIntensity, bh * 0.05f, glm::vec3(1.f),
                    widgetState->hoverIntensity * 0.8f));
    }

    renderer->push(TextRenderable(fontSmall, text, pos + glm::vec2(bw/2, bh/2),
                glm::vec3(1.f), 1.f, 1.f, HorizontalAlign::CENTER, VerticalAlign::CENTER));

    pos.y += bh + convertSize(buttonSpacing);

    return clicked;
}
