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

            STATUS_MOUSE_OVER      = 1 << 1,
            STATUS_MOUSE_DOWN      = 1 << 2,
            STATUS_MOUSE_PRESSED   = 1 << 3,
            STATUS_MOUSE_RELEASED  = 1 << 4,
            STATUS_SELECTED        = 1 << 5,
            STATUS_FOCUSED         = 1 << 6,
            STATUS_GAINED_FOCUSED  = 1 << 7,
            STATUS_LOST_FOCUSED    = 1 << 8,
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
        Constraints(Constraints && other) = default;
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
        bool mousePressed : 1;
        bool mouseDown : 1;
        bool mouseReleased : 1;
        bool inputHandled : 1;
    };

    template <typename T>
    struct WidgetState
    {
        T state;
        u32 frameCountWhenLastAccessed = 0;
    };

    struct WidgetStateNode
    {
        Str32 name;
        // TODO: Use u16 offset into widgetStateNodeStorage instead of pointers
        WidgetStateNode* childFirst = nullptr;
        WidgetStateNode* childLast = nullptr;
        WidgetStateNode* neighbor = nullptr;
        void* state = nullptr;
#ifndef NDEBUG
        WidgetStateNode* parent = nullptr;
#endif
    };
    //static_assert(sizeof(WidgetStateNode) == 64);

    struct RenderContext
    {
        Renderer* renderer;
        WidgetStateNode* currentWidgetStateNode;
        f32 deltaTime;
        u32 stateDepth = 0;
    };

    struct Widget
    {
        // TODO: Use u32 byte offset into widgetBuffer instead of pointers
        // if buffer is aligned to 16-byte boundaries I could even use a u16
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

            computedSize.x = desiredSize.x > 0.f
                ? clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth)
                : maxDim.x;
            computedSize.y = desiredSize.y > 0.f
                ? clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight)
                : maxDim.y;
        }

        void handleInput(InputStatus& input)
        {
            if (flags & WidgetFlags::BLOCK_INPUT)
            {
                bool isMouseOver =
                    pointInRectangle(input.mousePos, computedPosition, computedPosition + computedSize);

                if (isMouseOver)
                {
                    flags |= WidgetFlags::STATUS_MOUSE_OVER;

                    if (input.mousePressed)
                    {
                        flags |= WidgetFlags::STATUS_MOUSE_PRESSED;
                    }

                    return;
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

            WidgetStateNode* prevNode = ctx.currentWidgetStateNode;
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->doRender(ctx);
                ctx.currentWidgetStateNode = prevNode;
            }
        }
    };

    // TODO: make the Widget small enough
    //static_assert(sizeof(Widget) == 64);

    Buffer widgetBuffer;
    Buffer widgetStateBuffer;
    SmallArray<WidgetStateNode, 1024> widgetStateNodeStorage;
    WidgetStateNode rootWidgetStateNode;
    Widget root;
    u32 frameCount;
    u32 widgetCount;

    void pushID(RenderContext& ctx, const char* id)
    {
        ctx.stateDepth++;

        for (WidgetStateNode* child = ctx.currentWidgetStateNode->childFirst;
             child; child = child->neighbor)
        {
            if (strcmp(id, child->name.data()) == 0)
            {
                ctx.currentWidgetStateNode = child;
                return;
            }
        }

        widgetStateNodeStorage.push(WidgetStateNode{ Str32(id) });
        WidgetStateNode* newNode = &widgetStateNodeStorage.back();
        newNode->parent = ctx.currentWidgetStateNode;

        if (!ctx.currentWidgetStateNode->childFirst)
        {
            ctx.currentWidgetStateNode->childFirst = newNode;
        }
        else
        {
            ctx.currentWidgetStateNode->childLast->neighbor = newNode;
        }

        ctx.currentWidgetStateNode->childLast = newNode;
        ctx.currentWidgetStateNode = newNode;
    }

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
        inputStatus.mouseDown = g_input.isMouseButtonDown(MOUSE_LEFT);
        inputStatus.mouseReleased = g_input.isMouseButtonReleased(MOUSE_LEFT);
        inputStatus.mousePos = g_input.getMousePosition();
        inputStatus.inputHandled = false;
        root.handleInput(inputStatus);

        rootWidgetStateNode.name = "root";

        RenderContext ctx;
        ctx.renderer = renderer;
        ctx.currentWidgetStateNode = &rootWidgetStateNode;
        ctx.deltaTime = deltaTime;

        root.doRender(ctx);
    }

    template <typename T>
    T* add(Widget* parent, T const& widget, Vec2 size={INFINITY,INFINITY}, Vec2 position={0,0},
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

    template <typename T>
    inline T* getState(RenderContext& ctx)
    {
        if (ctx.currentWidgetStateNode->state)
        {
            WidgetState<T>* state = (WidgetState<T>*)ctx.currentWidgetStateNode->state;

            // if the state for this key was not accessed last frame, reset it
            if (state->frameCountWhenLastAccessed < frameCount - 1)
            {
                state->state = T{};
            }
            state->frameCountWhenLastAccessed = frameCount;

            return &state->state;
        }

        WidgetState<T>* newState = widgetStateBuffer.writeDefault<WidgetState<T>>();
        newState->frameCountWhenLastAccessed = frameCount;
        ctx.currentWidgetStateNode->state = (void*)newState;

        return &newState->state;
    }
}
