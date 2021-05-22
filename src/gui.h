#pragma once

#include "math.h"
#include "misc.h"
#include "config.h"
#include "font.h"
#include "input.h"

//#define WIDGET(name) class #name : public gui::_Widget<#name>
#define WIDGET(name) class #name : public gui::Widget

namespace gui
{
    namespace WidgetFlags
    {
        enum
        {
            BLOCK_INPUT = 1 << 0,
            CLIP_OVERFLOW = 1 << 1,
        };
    };

    class Widget
    {
    public:
        Widget* root = nullptr;
        Widget* childFirst = nullptr;
        Widget* childLast = nullptr;
        Widget* neighbor = nullptr;
        Vec2 position = { 0, 0 };
        Vec2 size = { 0, 0 };
        u32 flags = 0;

        Widget* build() { return this; }
        virtual void onUpdate() {}
        virtual void onRender() {}
        virtual void onClick() {}
        virtual void onGainedFocus() {}
        virtual void onLostFocus() {}
    };
    static_assert(sizeof(Widget) == 64);

    Buffer widgetBuffer;
    Widget root;
    // TODO: what about modal dialogs?

    void init()
    {
        widgetBuffer.resize(megabytes(4), 16);
    }

    void updateWidget(Widget* w, f32 deltaTime)
    {
        for (Widget* child = w->childFirst; w; w = w->neighbor) {
            updateWidget(child, deltaTime);
        }
    }

    void onUpdate(f32 deltaTime)
    {
        updateWidget(&root, deltaTime);
        root = Widget{};
        root.root = &root;
        widgetBuffer.clear();
    }

    template <typename T>
    T* add(Widget* parent, T const& widget, Vec2 position={0,0}, Vec2 size={0,0}, u32 flags=0)
    {
        T* w = widgetBuffer.write<T>(widget);
        w->position = position;
        w->size = size;
        w->flags = flags;
        w->root = w->build();

        if (!parent->childFirst) {
            parent->childFirst = w;
        } else {
            parent->childLast->neighbor = w;
        }

        parent->childLast = w;

        return w;
    }
}
