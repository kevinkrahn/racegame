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
            // for external widget API
            DEFAULT_SELECTION = 1 << 1,
            DISABLED          = 1 << 2,

            // internal
            BLOCK_INPUT       = 1 << 8,
            SELECTABLE        = 1 << 9,
            INPUT_CAPTURE     = 1 << 10,

            // status (set during layout and input phase, read during render phase)
            STATUS_MOUSE_OVER      = 1 << 16,
            STATUS_SELECTED        = 1 << 20,
            STATUS_GAINED_SELECTED = 1 << 21,
            STATUS_LOST_SELECTED   = 1 << 22,
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

        Constraints& operator = (Constraints const&) = default;
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

        Insets& operator = (Insets const&) = default;
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

    struct GuiContext
    {
        f32 deltaTime;
        f32 aspectRatio;
        Vec2 referenceScreenSize;
        Vec2 actualScreenSize;
    };

    struct RenderContext
    {
        Renderer* renderer;
        Mat4 transform;
        f32 alpha;
        Vec2 scissorPos;
        Vec2 scissorSize;
    };

    // Use pointers to WidgetStateNodes to identify the input captures
    // Can't use pointers to Widgets because they are not preserved across frames
    struct InputCaptureContext
    {
        WidgetStateNode* inputCaptureStateNode = nullptr;
        WidgetStateNode* selectedWidgetStateNode = nullptr;
    };

    u32 widgetCount;
    u32 frameCount;

    Buffer widgetBuffer;
    Buffer widgetStateBuffer;
    SmallArray<WidgetStateNode, 1024> widgetStateNodeStorage;
    Widget* root = nullptr;
    Widget* nullWidget = nullptr;
    SmallArray<Widget*, 64> inputCaptureWidgets;
    SmallArray<InputCaptureContext> inputCaptureStack;
    f32 guiScale = 1.f;

    struct Widget
    {
        // TODO: Use u32 byte offset into widgetBuffer instead of pointers
        // if buffer is aligned to 16-byte boundaries I could even use a u16
        Widget* root = nullptr;
        Widget* childFirst = nullptr;
        Widget* childLast = nullptr;
        Widget* neighbor = nullptr;
#ifndef NDEBUG
        Widget* parent = nullptr;
#endif
        WidgetStateNode* stateNode = nullptr;

        Vec2 computedSize = { 0, 0 };
        // 0 = size to content; INFINITY = take up as much space as possible
        Vec2 desiredSize = { INFINITY, INFINITY };

        Vec2 computedPosition = { 0, 0 };
        // TODO: Remove this. It doesn't seem to be necessary and it takes up space
        Vec2 positionWithinParent = { 0, 0 };
        Vec2 desiredPosition = { 0, 0 };

        u32 flags = 0;

#ifndef NDEBUG
        // for debug purposes
        const char* name = "";
#endif

#ifndef NDEBUG
        Widget(const char* name) : name(name) {}
#else
        Widget(const char* name) {}
#endif
        Vec2 center() const { return computedPosition + computedSize * 0.5f; }
        void pushID(const char* id);
        void navigate(struct Navigation& nav);
        Widget* build() { return this; }
        Widget* absolutePosition(Vec2 position) { desiredPosition = position; return this; }
        Widget* absolutePosition(f32 x, f32 y) { return absolutePosition(Vec2(x, y)); }
        Widget* position(Vec2 position) { return absolutePosition(position * guiScale); }
        Widget* position(f32 x, f32 y) { return absolutePosition(Vec2(x, y) * guiScale); }
        Widget* size(Vec2 size) { desiredSize = size * guiScale; return this;}
        Widget* size(f32 sizeX, f32 sizeY) { return size(Vec2(sizeX, sizeY)); }
        Widget* addFlags(u32 flags) { this->flags |= flags; return this; }
        Widget* findSelectedWidget(WidgetStateNode* stateNode);

        virtual void computeSize(Constraints const& constraints);
        virtual bool handleInputCaptureEvent(InputCaptureContext& ctx, InputEvent const& ev,
                Widget* selectedWidget);
        virtual bool handleInputEvent(InputCaptureContext& ctx, Widget* inputCapture, InputEvent const& ev);
        virtual bool handleMouseInput(InputCaptureContext& ctx, InputEvent& input);
        virtual void layout(GuiContext& ctx);
        virtual void render(GuiContext& ctx, RenderContext& rtx);

        template <typename T>
        T* add(T const& widget)
        {
            Widget* parent = this->root;

            T* w = widgetBuffer.write<T>(widget);
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
    };
    // TODO: make the Widget small enough
    //static_assert(sizeof(Widget) == 64);

    struct Navigation
    {
        f32 motionX;
        f32 motionY;
        f32 xRef;
        f32 yRef;
        Vec2 fromPos;
        Vec2 fromSize;
        Widget* minSelectTargetX = nullptr;
        Widget* minSelectTargetY = nullptr;
        Widget* maxSelectTargetX = nullptr;
        Widget* maxSelectTargetY = nullptr;
        f32 minDistX = FLT_MAX;
        f32 minDistY = FLT_MAX;
        f32 maxDistX = FLT_MIN;
        f32 maxDistY = FLT_MIN;
    };

    void init()
    {
        widgetBuffer.resize(megabytes(1), 16);
        widgetStateBuffer.resize(megabytes(1), 8);
        widgetStateNodeStorage.push(WidgetStateNode{ Str32("root") });
        widgetStateNodeStorage.push(WidgetStateNode{ Str32("null") });
    }

    void addInputCapture(Widget* w)
    {
        assert(w->flags & WidgetFlags::INPUT_CAPTURE);
        inputCaptureWidgets.push(w);
    }

    void clearInputCaptures()
    {
        inputCaptureStack.resize(1);
    }

    void pushInputCapture(Widget* w)
    {
        assert(w->flags & WidgetFlags::INPUT_CAPTURE);
        inputCaptureStack.push({ w->stateNode, nullptr });
    }

    void popInputCapture()
    {
        // if the input capture stack is empty that means someone popped the stack too many times
        // the root should always remain on the stack
        assert(inputCaptureStack.size() > 1);
        inputCaptureStack.pop();
    }

    bool isActiveInputCapture(Widget* w)
    {
        assert(w->flags & WidgetFlags::INPUT_CAPTURE);
        return !inputCaptureStack.empty()
            && inputCaptureStack.back().inputCaptureStateNode == w->stateNode;
    }

    bool makeActive(Widget* w)
    {
        if (!isActiveInputCapture(w))
        {
            pushInputCapture(w);
            return true;
        }
        return false;
    }

    void onBeginUpdate(f32 deltaTime);
    void onUpdate(Renderer* renderer, i32 w, i32 h, f32 deltaTime, u32 count);

    template <typename T>
    inline T* getState(Widget* w)
    {
        if (w->stateNode->state)
        {
            WidgetState<T>* state = (WidgetState<T>*)w->stateNode->state;

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
        w->stateNode->state = (void*)newState;

        return &newState->state;
    }

    Widget* findAncestorByStateNode(Widget* w, WidgetStateNode* state);
    Widget* findAncestorByFlags(Widget* w, u32 matchFlags, u32 unmatchFlags);
    Widget* findParent(Widget* w, u32 flags);
}
