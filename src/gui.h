#pragma once

#include "math.h"
#include <map>
#include <vector>

enum struct WidgetType
{
    NONE,
    PANEL,
    BUTTON,
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
    WidgetState* widgetState;
};

class Gui
{
    std::map<std::string, std::unique_ptr<WidgetState>> childState;
    std::vector<WidgetStackItem> widgetStack;
    bool isMouseOverUI = false;
    bool isMouseClickHandled = false;
    bool isInputCaptured = false;
    bool isKeyboardInputHandled = true;
    class Font* fontSmall = nullptr;
    class Font* fontBig = nullptr;
    class Texture* white = nullptr;
    class Renderer* renderer = nullptr;

    f32 convertSize(f32 size);
    WidgetState* getWidgetState(WidgetState* parent, std::string const& identifier, WidgetType widgetType);

    f32 panelWidth = 220;
    f32 buttonHeight = 36;
    f32 buttonSpacing = 6;

public:
    void beginFrame();
    void endFrame();
    void end();
    void beginPanel(std::string const& text, glm::vec2 position, f32 height,
            f32 halign, bool solidBackground=false, i32 buttonCount=-1);
    bool button(std::string const& text, bool active=true);
    bool toggle(std::string const& text, bool& enabled);
    bool slider(std::string const& text, f32& value);
    bool textEdit(std::string const& text, std::string& value);
    void beginSelector(i32& selectedIndex, bool big=false);
    bool selector(std::string const& text, const char* icon=nullptr);
} g_gui;

