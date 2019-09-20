#pragma once

#include "math.h"
#include <map>
#include <vector>

enum struct WidgetType
{
    NONE,
    PANEL,
    BUTTON,
    SELECT,
};

struct WidgetState
{
    WidgetType widgetType;
    u64 frameCount = 0;
    f32 hoverIntensity = 0.f;
    i32 selectIndex = 0;
    std::map<std::string, std::unique_ptr<WidgetState>> childState;
};

struct WidgetStackItem
{
    WidgetType widgetType;
    i32 selectableChildCount = 0;
    glm::vec2 position = { 0, 0 };
    glm::vec2 size = { 0, 0 };
    glm::vec2 nextWidgetPosition = { 0, 0 };
    bool hasKeyboardSelect = false;
    WidgetState* widgetState = nullptr;
    i32* outputValueI32 = nullptr;
    f32 itemHeight = 0.f;
    f32 itemSpacing = 0.f;
};

class Gui
{
    std::map<std::string, std::unique_ptr<WidgetState>> childState;
    std::vector<WidgetStackItem> widgetStack;
    class Font* fontSmall = nullptr;
    class Font* fontBig = nullptr;
    class Texture* white = nullptr;
    class Renderer* renderer = nullptr;

    WidgetState* getWidgetState(WidgetState* parent, std::string const& identifier,
            WidgetType widgetType);

public:
    bool isMouseOverUI = false;
    bool isMouseClickHandled = false;
    bool isMouseCaptured = false;
    bool isKeyboardInputCaptured = false;
    bool isKeyboardInputHandled = true;

    f32 convertSize(f32 size);

    void beginFrame();
    void endFrame();
    void end();
    void beginPanel(std::string const& text, glm::vec2 position, f32 height,
            f32 halign, bool solidBackground=false, i32 buttonCount=-1,
            bool showTitle=true, f32 itemHeight=36, f32 itemSpacing=6, f32 panelWidth = 220);
    bool button(std::string const& text, bool active=true);
    bool toggle(std::string const& text, bool& enabled);
    bool slider(std::string const& text, f32 minValue, f32 maxValue, f32& value);
    bool textEdit(std::string const& text, std::string& value);
    void beginSelect(std::string const& text, i32* selectedIndex, bool showTitle=true);
    bool option(std::string const& text, i32 value, const char* icon=nullptr);
    void label(std::string const& text);
} g_gui;

