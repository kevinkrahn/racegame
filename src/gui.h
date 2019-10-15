#pragma once

#include "math.h"
#include <map>
#include <vector>
#include <functional>

enum struct WidgetType
{
    NONE,
    PANEL,
    BUTTON,
    SELECT,
};

struct CompareKey
{
    bool operator()(const char* a, const char* b) const
    {
        return strcmp(a, b) < 0;
    }
};

struct WidgetState
{
    WidgetType widgetType;
    u64 frameCount = 0;
    f32 hoverIntensity = 0.f;
    i32 selectIndex = 0;
    std::map<const char*, std::unique_ptr<WidgetState>, CompareKey> childState;
    f32 repeatTimer = 0.f;
    i32 selectableChildCount = 0;
    bool mouseCaptured = false;
    bool isMouseOver = false;
};

struct WidgetStackItem
{
    WidgetType widgetType;
    glm::vec2 position = { 0, 0 };
    glm::vec2 size = { 0, 0 };
    glm::vec2 nextWidgetPosition = { 0, 0 };
    bool useKeyboardControl = false;
    WidgetState* widgetState = nullptr;
    i32* outputValueI32 = nullptr;
    f32 itemHeight = 0.f;
    f32 itemSpacing = 0.f;
    bool solidBackground = false;
};

class Gui
{
    std::map<const char*, std::unique_ptr<WidgetState>, CompareKey> childState;
    std::vector<WidgetStackItem> widgetStack;
    class Font* fontTiny = nullptr;
    class Font* fontSmall = nullptr;
    class Font* fontBig = nullptr;
    struct Texture* white = nullptr;
    class Renderer* renderer = nullptr;

    WidgetState* getWidgetState(WidgetState* parent, const char* identifier,
            WidgetType widgetType);

    bool buttonBase(WidgetStackItem& parent, WidgetState* widgetState, glm::vec2 pos,
            f32 bw, f32 bh, std::function<bool()> onHover, std::function<bool()> onSelected,
            bool active=true);

public:
    bool isMouseOverUI = false;
    bool isMouseClickHandled = false;
    bool isMouseCaptured = false;
    bool isKeyboardInputCaptured = false;
    bool isKeyboardInputHandled = true;

    bool didSelect();
    i32 didChangeSelection();

    f32 convertSize(f32 size);

    void beginFrame();
    void endFrame();
    void end();
    void beginPanel(const char* text, glm::vec2 position, f32 halign,
            bool solidBackground=false, bool useKeyboardControl=false,
            bool showTitle=true, f32 itemHeight=36, f32 itemSpacing=6, f32 panelWidth = 220);
    bool button(const char* text, bool active=true);
    bool vehicleButton(const char* text, Texture* icon, struct Driver* driver);
    bool toggle(const char* text, bool& enabled);
    bool slider(const char* text, f32 minValue, f32 maxValue, f32& value);
    bool textEdit(const char* text, std::string& value);
    i32 select(const char* text, std::string* firstValue,
            i32 count, i32& currentIndex);
    void beginSelect(const char* text, i32* selectedIndex, bool showTitle=true);
    bool option(const char* text, i32 value, const char* icon=nullptr);
    void label(const char* text, bool showBackground=false);
} g_gui;

