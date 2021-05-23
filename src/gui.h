#pragma once

#include "math.h"
#include "misc.h"
#include "config.h"
#include "font.h"
#include "input.h"
#include "renderer.h"

//#define WIDGET(name) struct ###name : public gui::_Widget<#name>
#define WIDGET(name) struct name : public gui::Widget

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

    struct Constraints
    {
        f32 minWidth = 0;
        f32 maxWidth = INFINITY;
        f32 minHeight = 0;
        f32 maxHeight = INFINITY;

        Constraints() {}
        Constraints(Constraints const& other) = default;
        Constraints(Constraints&& other) = default;
        Constraints(f32 minWidth, f32 maxWidth=INFINITY, f32 minHeight=INFINITY, f32 maxHeight=INFINITY)
            : minWidth(minWidth), maxWidth(maxWidth), minHeight(minHeight), maxHeight(maxHeight) {}
    };

    struct Insets
    {
        f32 left = 0.f;
        f32 top = 0.f;
        f32 right = 0.f;
        f32 bottom = 0.f;

        Insets() {}
        Insets(Insets const& other) = default;
        Insets(Insets && other) = default;
        explicit Insets(f32 v)
            : left(v), top(v), right(v), bottom(v) {}
        Insets(f32 left, f32 top, f32 right, f32 bottom)
            : left(left), top(top), right(right), bottom(bottom) {}
        Insets(f32 horizontal, f32 vertical)
            : left(horizontal), top(vertical), right(horizontal), bottom(vertical) {}
    };

    struct Widget
    {
        Widget* root = nullptr;
        Widget* childFirst = nullptr;
        Widget* childLast = nullptr;
        Widget* neighbor = nullptr;
        Vec2 computedPosition = { 0, 0 };
        Vec2 computedSize = { 0, 0 };
        Vec2 desiredPosition = { 0, 0 };
        Vec2 desiredSize = { INFINITY, INFINITY };
        u32 flags = 0;

        Widget* build() { return this; }
        virtual void layout(Constraints const& constraints)
        {
            computedSize.x = clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth);
            computedSize.y = clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight);

            Constraints childConstraints;
            childConstraints.minWidth = 0.f;
            childConstraints.maxWidth = computedSize.x;
            childConstraints.minHeight = 0.f;
            childConstraints.maxHeight = computedSize.y;

            assert(childConstraints.minWidth <= childConstraints.maxWidth);
            assert(childConstraints.minHeight <= childConstraints.maxHeight);

            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->computedPosition = this->computedPosition + child->desiredPosition;
                child->layout(childConstraints);
            }
        }
        void render(Renderer* renderer)
        {
            this->onRender(renderer);
            for (Widget* child = childFirst; child; child = child->neighbor) {
                child->render(renderer);
            }
        }

        virtual void onRender(Renderer* renderer) {}
        virtual void onClick() {}
        virtual void onGainedFocus() {}
        virtual void onLostFocus() {}
    };
    // TODO: make the Widget small enough
    //static_assert(sizeof(Widget) == 64);

    Buffer widgetBuffer;
    Widget root;
    // TODO: what about modal dialogs?

    void init()
    {
        widgetBuffer.resize(megabytes(4), 16);
    }

    void onBeginUpdate(f32 deltaTime)
    {
        root = Widget{};
        root.root = &root;
        widgetBuffer.clear();
    }

    void onUpdate(Renderer* renderer, i32 w, i32 h, f32 deltaTime)
    {
        f32 aspectRatio = (f32)w / (f32)h;
        f32 baseHeight = 720;
        root.desiredSize = { baseHeight * aspectRatio, baseHeight };
        root.desiredPosition = { 0, 0 };
        root.computedSize = root.desiredSize;
        root.computedPosition = root.desiredPosition;

        //Vec2 screenDimMultiplier = Vec2((f32)w, (f32)h) / root.computedSize;

        root.layout(Constraints());
        root.render(renderer);
    }

    template <typename T>
    T* add(Widget* parent, T const& widget, Vec2 position={0,0}, Vec2 size={INFINITY,INFINITY},
            u32 flags=0)
    {
        T* w = widgetBuffer.write<T>(widget);
        w->desiredPosition = position;
        w->desiredSize = size;
        w->flags = flags;
        w->root = w->build();

        if (!parent->root->childFirst) {
            parent->root->childFirst = w;
        } else {
            parent->root->childLast->neighbor = w;
        }

        parent->root->childLast = w;

        return w;
    }
}
