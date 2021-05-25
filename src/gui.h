#pragma once

#include "math.h"
#include "misc.h"
#include "config.h"
#include "font.h"
#include "input.h"
#include "renderer.h"

namespace gui
{
    namespace WidgetFlags
    {
        enum
        {
            BLOCK_INPUT = 1 << 0,

            STATUS_MOUSE_OVER = 1 << 1,
            STATUS_MOUSE_DOWN = 1 << 2,
            STATUS_SELECTED = 1 << 3,
            STATUS_FOCUSED = 1 << 4,
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

    struct InputStatus
    {
        Vec2 mousePos;
        bool mousePressed;
        bool mouseHandled;
    };

    struct RenderContext
    {
        Renderer* renderer;
        StrBuf& keyBuf;
        f32 deltaTime;
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
        // 0 = size to content; INFINITY = take up as much space as possible
        Vec2 desiredSize = { INFINITY, INFINITY };
        u32 flags = 0;

        Widget* build() { return this; }
        virtual void computeSize(Constraints const& constraints)
        {
            Vec2 maxDim(0, 0);

            if (childFirst)
            {
                f32 w = desiredSize.x > 0.f
                    ? clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth)
                    : constraints.maxWidth;

                f32 h = desiredSize.y > 0.f
                    ? clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight)
                    : constraints.maxHeight;

                Constraints childConstraints;
                childConstraints.minWidth = 0.f;
                childConstraints.maxWidth = w;
                childConstraints.minHeight = 0.f;
                childConstraints.maxHeight = h;

                for (Widget* child = childFirst; child; child = child->neighbor)
                {
                    child->computeSize(childConstraints);
                    maxDim = max(maxDim, child->computedSize);
                }
            }

            if (desiredSize.x == 0.f)
            {
                assert(childFirst);
                desiredSize.x = maxDim.x;
            }
            computedSize.x = clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth);

            if (desiredSize.y == 0.f)
            {
                assert(childFirst);
                desiredSize.y = maxDim.y;
            }
            computedSize.y = clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight);
        }

        void handleInput(InputStatus& input)
        {
            bool isMouseOver =
                pointInRectangle(input.mousePos, computedPosition, computedPosition + computedSize);

            if (isMouseOver)
            {
                flags |= WidgetFlags::STATUS_MOUSE_OVER;
            }

            if (!input.mouseHandled)
            {
                if (g_input.isMouseButtonPressed(MOUSE_LEFT))
                {
                    onClick(input);
                    flags |= WidgetFlags::STATUS_MOUSE_DOWN;
                }
            }

            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->handleInput(input);
            }
        }

        virtual void layout()
        {
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->computedPosition = computedPosition + child->desiredPosition;
                child->layout();
            }
        }

        virtual void render(RenderContext& ctx) {}

        void doRender(RenderContext& ctx)
        {
            this->render(ctx);
            u32 prevSize = ctx.keyBuf.size();
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->doRender(ctx);
                ctx.keyBuf.setSize(prevSize);
            }
        }

        virtual void onClick(InputStatus& input) {}

        virtual void onGainedFocus() {}
        virtual void onLostFocus() {}
    };

    // TODO: make the Widget small enough
    //static_assert(sizeof(Widget) == 64);

    Buffer widgetBuffer;
    Buffer widgetStateBuffer;
    Map<Str64, void*> widgetStateMap;
    Widget root;
    u32 frameCount;
    u32 widgetCount;

    template <typename T>
    struct WidgetState
    {
        T state;
        u32 frameCountWhenLastAccessed = 0;
    };

    // TODO: what about modal dialogs?

    void init()
    {
        widgetBuffer.resize(megabytes(1), 16);
        widgetStateBuffer.resize(megabytes(1), 8);
    }

    void onBeginUpdate(f32 deltaTime)
    {
        root = Widget{};
        root.root = &root;
        widgetBuffer.clear();
        widgetCount = 0;
    }

    void onUpdate(Renderer* renderer, i32 w, i32 h, f32 deltaTime, u32 count)
    {
        frameCount = count;

        f32 aspectRatio = (f32)w / (f32)h;
        f32 baseHeight = 720;
        root.desiredSize = { baseHeight * aspectRatio, baseHeight };

        root.computeSize(Constraints());
        root.layout();

        InputStatus inputStatus;
        inputStatus.mousePressed = g_input.isMouseButtonPressed(MOUSE_LEFT);
        inputStatus.mousePos = g_input.getMousePosition();
        inputStatus.mouseHandled = false;
        root.handleInput(inputStatus);

        static StrBuf buf;
        buf.clear();
        buf.write("root");
        RenderContext ctx{renderer, buf, deltaTime};
        root.doRender(ctx);
    }

    template <typename T>
    T* add(Widget* parent, T const& widget, Vec2 position={0,0}, Vec2 size={INFINITY,INFINITY},
            u32 flags=0)
    {
        T* w = widgetBuffer.write<T>(widget);
        w->desiredPosition = position;
        w->desiredSize = size;
        w->flags = flags;
        w->root = w;
        w->root = w->build();

        if (!parent->root->childFirst)
        {
            parent->root->childFirst = w;
        }
        else
        {
            parent->root->childLast->neighbor = w;
        }

        parent->root->childLast = w;

        ++widgetCount;

        return w;
    }

    // TODO: This could be greatly optimized. Should build a tree for the key on the way down the render tree
    template <typename T>
    inline T* getState(StrBuf const& buf)
    {
        Str64* s = (Str64*)buf.data(); // HACK (Str64 happens to be nothing but an array of characters)
        void** statePtr = widgetStateMap.get(*s);
        if (!statePtr)
        {
            WidgetState<T>* newState = widgetStateBuffer.writeDefault<WidgetState<T>>();
            widgetStateMap.set(*s, newState);
            newState->frameCountWhenLastAccessed = frameCount;
            return &newState->state;
        }

        WidgetState<T>* state = *(WidgetState<T>**)statePtr;

        // if the state for this key was not accessed last frame, reset it
        if (state->frameCountWhenLastAccessed < frameCount - 1)
        {
            state->state = T{};
        }
        state->frameCountWhenLastAccessed = frameCount;
        return &state->state;
    }
}
