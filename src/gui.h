#pragma once

#include "math.h"
#include "misc.h"
#include "config.h"
#include "font.h"
#include "input.h"
#include "renderer.h"

namespace gui
{
    struct Widget;

    namespace WidgetFlags
    {
        enum
        {
            BLOCK_INPUT = 1 << 0,

            STATUS_MOUSE_OVER      = 1 << 16,
            STATUS_MOUSE_DOWN      = 1 << 17,
            STATUS_MOUSE_PRESSED   = 1 << 18,
            STATUS_MOUSE_RELEASED  = 1 << 19,
            STATUS_SELECTED        = 1 << 20,
            STATUS_GAINED_SELECTED = 1 << 21,
            STATUS_LOST_SELECTED   = 1 << 22,
            STATUS_FOCUSED         = 1 << 23,
            STATUS_GAINED_FOCUSED  = 1 << 24,
            STATUS_LOST_FOCUSED    = 1 << 25,
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
        Constraints(f32 width, f32 height)
            : minWidth(width), maxWidth(width),
              minHeight(height), maxHeight(height) {}
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

    struct MouseInput
    {
        Vec2 mousePos;
        bool mousePressed : 1;
        bool mouseDown : 1;
        bool mouseReleased : 1;
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

    struct WidgetContext
    {
        Renderer* renderer;
        f32 deltaTime;
        f32 aspectRatio;
        Vec2 referenceScreenSize;
        Vec2 actualScreenSize;
        f32 scale;
    };

    const auto NOOP = []{};
    using noop_t = void(*)();

    const f32 REFERENCE_HEIGHT = 1080.f;

    u32 widgetCount;
    u32 frameCount;

    Buffer widgetBuffer;
    Buffer widgetStateBuffer;
    SmallArray<WidgetStateNode, 1024> widgetStateNodeStorage;
    Widget* root = nullptr;
    Widget* selectedWidget = nullptr;
    Widget* focusedWidget = nullptr;
    SmallArray<Widget*> selectionContexts;

    struct Widget
    {
        // TODO: Use u32 byte offset into widgetBuffer instead of pointers
        // if buffer is aligned to 16-byte boundaries I could even use a u16
        Widget* root = nullptr;
        Widget* childFirst = nullptr;
        Widget* childLast = nullptr;
        Widget* neighbor = nullptr;
        WidgetStateNode* stateNode = nullptr;

        Vec2 computedSize = { 0, 0 };
        // 0 = size to content; INFINITY = take up as much space as possible
        Vec2 desiredSize = { INFINITY, INFINITY };

        Vec2 computedPosition = { 0, 0 };
        Vec2 desiredPosition = { 0, 0 };

        u32 flags = 0;

#ifndef NDEBUG
        // for debug purposes
        const char* name = "";
        Widget* parent = nullptr;
#endif

#ifndef NDEBUG
        Widget(const char* name) : name(name) {}
#else
        Widget(const char* name) {}
#endif

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

        void handleMouseInput(MouseInput& input)
        {
            bool isMouseOver =
                pointInRectangle(input.mousePos, computedPosition, computedPosition + computedSize);

            if (!isMouseOver)
            {
                return;
            }

            if (flags & WidgetFlags::BLOCK_INPUT)
            {
                flags |= WidgetFlags::STATUS_MOUSE_OVER;

                if (input.mousePressed)
                {
                    flags |= WidgetFlags::STATUS_MOUSE_PRESSED;
                }

                if (input.mouseDown)
                {
                    flags |= WidgetFlags::STATUS_MOUSE_DOWN;
                }

                if (input.mouseReleased)
                {
                    flags |= WidgetFlags::STATUS_MOUSE_RELEASED;
                }

                return;
            }

            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->handleMouseInput(input);
            }
        }

        virtual void layout(WidgetContext& ctx)
        {
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->computedPosition = computedPosition + child->desiredPosition;
                child->layout(ctx);
            }
        }

        virtual void render(WidgetContext& ctx) {}

        void doRender(WidgetContext& ctx)
        {
            computedSize *= ctx.scale;
            computedPosition *= ctx.scale;
            this->render(ctx);

            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->doRender(ctx);
            }
        }

        void pushID(const char* id)
        {
            for (WidgetStateNode* child = stateNode->childFirst;
                child; child = child->neighbor)
            {
                if (strcmp(id, child->name.data()) == 0)
                {
                    stateNode = child;
                    return;
                }
            }

            widgetStateNodeStorage.push(WidgetStateNode{ Str32(id) });
            WidgetStateNode* newNode = &widgetStateNodeStorage.back();
#ifndef NDEBUG
            newNode->parent = stateNode;
#endif

            if (!stateNode->childFirst)
            {
                stateNode->childFirst = newNode;
            }
            else
            {
                stateNode->childLast->neighbor = newNode;
            }

            stateNode->childLast = newNode;
            stateNode = newNode;
        }

        template <typename T>
        inline T* getState()
        {
            if (stateNode->state)
            {
                WidgetState<T>* state = (WidgetState<T>*)stateNode->state;

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
            stateNode->state = (void*)newState;

            return &newState->state;
        }
    };

    // TODO: make the Widget small enough
    //static_assert(sizeof(Widget) == 64);

    // TODO: what about modal dialogs?

    void init()
    {
        widgetBuffer.resize(megabytes(1), 16);
        widgetStateBuffer.resize(megabytes(1), 8);
        widgetStateNodeStorage.push(WidgetStateNode{ Str32("root") });
    }

    void onBeginUpdate(f32 deltaTime)
    {
        widgetBuffer.clear();
        selectionContexts.clear();
        widgetCount = 0;

        root = widgetBuffer.write<Widget>(Widget("Root"));
        root->root = root;
        root->stateNode = &widgetStateNodeStorage.front();
    }

    void onUpdate(Renderer* renderer, i32 w, i32 h, f32 deltaTime, u32 count)
    {
        frameCount = count;

        f32 aspectRatio = (f32)w / (f32)h;
        Vec2 referenceScreenSize(REFERENCE_HEIGHT * aspectRatio, REFERENCE_HEIGHT);
        Vec2 actualScreenSize((f32)w, (f32)h);
        Vec2 sizeMultiplier = actualScreenSize / referenceScreenSize;

        WidgetContext ctx;
        ctx.renderer = renderer;
        ctx.deltaTime = deltaTime;
        ctx.aspectRatio = aspectRatio;
        ctx.referenceScreenSize = referenceScreenSize;
        ctx.actualScreenSize = actualScreenSize;
        ctx.scale = (f32)h / REFERENCE_HEIGHT;

        root->desiredSize = { (f32)w, (f32)h };
        root->computeSize(Constraints());
        root->layout(ctx);

        MouseInput mouseInput;
        mouseInput.mousePressed = g_input.isMouseButtonPressed(MOUSE_LEFT);
        mouseInput.mouseDown = g_input.isMouseButtonDown(MOUSE_LEFT);
        mouseInput.mouseReleased = g_input.isMouseButtonReleased(MOUSE_LEFT);
        mouseInput.mousePos = g_input.getMousePosition() / actualScreenSize * referenceScreenSize;

        root->handleMouseInput(mouseInput);

        if (selectedWidget)
        {
        }

        root->doRender(ctx);
    }

    template <typename T>
    T* add(Widget* parent, T const& widget, Vec2 size={INFINITY,INFINITY}, Vec2 position={0,0},
            u32 flags=0)
    {
        T* w = widgetBuffer.write<T>(widget);
        w->desiredPosition = position;
        w->desiredSize = size;
        w->flags = flags;
        w->stateNode = parent->stateNode;
        w->root = w;
        w->root = w->build();
#ifndef NDEBUG
        w->parent = parent;
#endif

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

    void setDefaultSelection(Widget* widget)
    {

    }

    // TODO: Add push api.
    // push(Container())
    // pushSize()
    // pushPosition()
    //
    // add(Button())
    // add(Button())
    // add(Button())
    // add(Button())
    // ...
    // popPosition()
    // popSize()
    // pop()
}
