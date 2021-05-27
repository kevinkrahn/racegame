#pragma once

#include "gui.h"
#include "2d.h"
#include "font.h"

#define CALLBACK(className, name) \
    void(*name ## Callback)(void*) = nullptr; \
    void* name ## CallbackData = nullptr; \
    template <typename _F_> \
    className* name(_F_ const& cb) { \
        name ## CallbackData = (void*)widgetBuffer.write<_F_>(cb); \
        name ## Callback = [](void* t){ (*(_F_*)t)(); }; \
        return this; \
    }

#define INVOKE_CALLBACK(name) \
    if (name ## Callback) { name ## Callback(name ## CallbackData); }

namespace gui
{
    struct Stack : public Widget {};

    struct Text : public Widget
    {
        Font* font;
        const char* text;
        Vec4 color;

        Text(Font* font, const char* text, Vec4 color=Vec4(1.0))
            : Widget("Text"), font(font), text(text), color(color) {}

        virtual void computeSize(Constraints const& constraints) override
        {
            desiredSize = font->stringDimensions(text);
#if 0
            computedSize.x = clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth);
            computedSize.y = clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight);
#else
            computedSize = desiredSize;
#endif

            assert(childFirst == nullptr);
            assert(constraints.maxWidth > 0);
            assert(constraints.maxHeight > 0);
        }

        virtual void render(WidgetContext& ctx) override
        {
            ui::text(font, text, computedPosition, Vec3(1.f), 1.f);
        }
    };

    struct Image : public Widget
    {
        Texture* tex;
        bool preserveAspectRatio = true;

        Image(Texture* tex, bool preserveAspectRatio=true)
            : Widget("Image"), tex(tex), preserveAspectRatio(preserveAspectRatio) {}
        virtual void computeSize(Constraints const& constraints) override
        {
            if (desiredSize.x == 0.f)
            {
                desiredSize.x = (f32)tex->width;
            }
            if (desiredSize.y == 0.f)
            {
                desiredSize.y = (f32)tex->height;
            }

            if (preserveAspectRatio)
            {
                if (desiredSize.x < constraints.minWidth)
                {
                    f32 aspectRatio = (f32)tex->height / (f32)tex->width;
                    desiredSize.x = constraints.minWidth;
                    desiredSize.y = constraints.minWidth * aspectRatio;
                }
                if (desiredSize.x > constraints.maxWidth)
                {
                    f32 aspectRatio = (f32)tex->height / (f32)tex->width;
                    desiredSize.x = constraints.maxWidth;
                    desiredSize.y = constraints.maxWidth * aspectRatio;
                }
                if (desiredSize.y < constraints.minHeight)
                {
                    f32 aspectRatio = (f32)tex->width / (f32)tex->height;
                    desiredSize.y = constraints.minHeight;
                    desiredSize.x = constraints.minHeight * aspectRatio;
                }
                if (desiredSize.y > constraints.maxHeight)
                {
                    f32 aspectRatio = (f32)tex->width / (f32)tex->height;
                    desiredSize.y = constraints.maxHeight;
                    desiredSize.x = constraints.maxHeight * aspectRatio;
                }
                computedSize.x = desiredSize.x;
                computedSize.y = desiredSize.y;
            }
            else
            {
                computedSize.x = clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth);
                computedSize.y = clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight);
            }

            assert(childFirst == nullptr);
        }

        virtual void render(WidgetContext& ctx) override
        {
            ui::rect(0, tex, computedPosition, computedSize);
        }
    };

    struct Container : public Widget
    {
        Insets padding;
        Insets margin;
        Constraints additionalConstraints;
        Vec4 backgroundColor = Vec4(0);
        HAlign halign = HAlign::LEFT;
        VAlign valign = VAlign::TOP;

        Container() : Widget("Container") {}
        Container(Insets const& padding, Insets const& margin = {},
                Constraints const& additionalConstraints = {}, Vec4 const& backgroundColor = Vec4(0),
                HAlign halign = HAlign::LEFT, VAlign valign = VAlign::TOP)
            : Widget("Contaienr"), padding(padding), margin(margin), additionalConstraints(additionalConstraints),
              backgroundColor(backgroundColor), halign(halign), valign(valign) {}
        Container(Vec4 const& backgroundColor = Vec4(0))
            : Widget("Container"), backgroundColor(backgroundColor) {}

        virtual void computeSize(Constraints const& constraints) override
        {
            f32 horizontalInset = padding.left + padding.right + margin.left + margin.right;
            f32 verticalInset = padding.top + padding.bottom + margin.top + margin.bottom;

            Constraints actualConstraints;
            actualConstraints.minWidth =
                clamp(additionalConstraints.minWidth, constraints.minWidth, constraints.maxWidth);
            actualConstraints.maxWidth =
                clamp(additionalConstraints.maxWidth, constraints.minWidth, constraints.maxWidth);
            actualConstraints.minHeight =
                clamp(additionalConstraints.minHeight, constraints.minHeight, constraints.maxHeight);
            actualConstraints.maxHeight =
                clamp(additionalConstraints.maxHeight, constraints.minHeight, constraints.maxHeight);

            Vec2 maxDim(0, 0);
            if (childFirst)
            {
                f32 w = desiredSize.x > 0.f
                    ? clamp(desiredSize.x, actualConstraints.minWidth, actualConstraints.maxWidth)
                    : actualConstraints.maxWidth;

                f32 h = desiredSize.y > 0.f
                    ? clamp(desiredSize.y, actualConstraints.minHeight, actualConstraints.maxHeight)
                    : actualConstraints.maxHeight;

                Constraints childConstraints;
                childConstraints.minWidth = 0.f;
                childConstraints.maxWidth = max(w - horizontalInset, 0.f);
                childConstraints.minHeight = 0.f;
                childConstraints.maxHeight = max(h - verticalInset, 0.f);

                for (Widget* child = childFirst; child; child = child->neighbor)
                {
                    child->computeSize(childConstraints);
                    maxDim = max(maxDim, child->computedSize);
                }
            }

            computedSize.x = desiredSize.x > 0.f
                ? clamp(desiredSize.x, actualConstraints.minWidth, actualConstraints.maxWidth)
                : maxDim.x + horizontalInset;
            computedSize.y = desiredSize.y > 0.f
                ? clamp(desiredSize.y, actualConstraints.minHeight, actualConstraints.maxHeight)
                : maxDim.y + verticalInset;
        }

        virtual void layout(WidgetContext& ctx) override
        {
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                Vec2 childPos(0, 0);

                if (halign == HAlign::LEFT)
                {
                    childPos.x = margin.left + padding.left;
                }
                else if (halign == HAlign::RIGHT)
                {
                    childPos.x =
                        computedSize.x - child->computedSize.x - margin.right - padding.right;
                }
                else if (halign == HAlign::CENTER)
                {
                    childPos.x = computedSize.x * 0.5f - child->computedSize.x * 0.5f;
                }

                if (valign == VAlign::TOP)
                {
                    childPos.y = margin.top + padding.top;
                }
                else if (valign == VAlign::BOTTOM)
                {
                    childPos.y =
                        computedSize.y - child->computedSize.y - margin.bottom - padding.bottom;
                }
                else if (valign == VAlign::CENTER)
                {
                    childPos.y = computedSize.y * 0.5f - child->computedSize.y * 0.5f;
                }

                child->computedPosition =
                    computedPosition + child->desiredPosition + childPos;
                child->layout(ctx);
            }
        }

        virtual void render(WidgetContext& ctx) override
        {
            if (backgroundColor.a > 0.f)
            {
                ui::rectBlur(0, nullptr, computedPosition + Vec2(margin.left, margin.top),
                    computedSize - Vec2(margin.left + margin.right, margin.top + margin.bottom),
                    backgroundColor);
            }
        }
    };

    struct Row : public Widget
    {
        f32 itemSpacing;

        Row(f32 itemSpacing=0.f) : Widget("Row"), itemSpacing(itemSpacing) {}

        virtual void computeSize(Constraints const& constraints) override
        {
            Constraints childConstraints;
            childConstraints.minWidth = 0.f;
            childConstraints.maxWidth = constraints.maxWidth;
            childConstraints.minHeight = 0.f;
            childConstraints.maxHeight = constraints.maxHeight;

            f32 totalWidth = 0.f;
            f32 maxHeight = 0.f;
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->computeSize(childConstraints);
                totalWidth += child->computedSize.x + itemSpacing;
                maxHeight = max(maxHeight, child->computedSize.y);
                assert(child->desiredSize.x != INFINITY);
            }

            computedSize.x = desiredSize.x > 0.f
                ? clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth)
                : totalWidth - itemSpacing;
            computedSize.y = desiredSize.y > 0.f
                ? clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight)
                : maxHeight;
        }

        virtual void layout(WidgetContext& ctx) override
        {
            f32 offset = 0;
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                // TODO: collapse adjacent margins
                child->computedPosition =
                    computedPosition + child->desiredPosition + Vec2(offset, 0.f);
                child->layout(ctx);
                offset += child->computedSize.x + itemSpacing;
            }
        }
    };

    struct Column : public Widget
    {
        f32 itemSpacing;

        Column(f32 itemSpacing=0.f) : Widget("Row"), itemSpacing(itemSpacing) {}

        virtual void computeSize(Constraints const& constraints) override
        {
            Constraints childConstraints;
            childConstraints.minWidth = 0.f;
            childConstraints.maxWidth = constraints.maxWidth;
            childConstraints.minHeight = 0.f;
            childConstraints.maxHeight = constraints.maxHeight;

            f32 totalHeight = 0.f;
            f32 maxWidth = 0.f;
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->computeSize(childConstraints);
                totalHeight += child->computedSize.y + itemSpacing;
                maxWidth = max(maxWidth, child->computedSize.x);
                assert(child->desiredSize.y != INFINITY);
            }

            computedSize.x = desiredSize.x > 0.f
                ? clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth)
                : maxWidth;
            computedSize.y = desiredSize.y > 0.f
                ? clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight)
                : totalHeight - itemSpacing;
        }

        virtual void layout(WidgetContext& ctx) override
        {
            f32 offset = 0;
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                // TODO: collapse adjacent margins
                child->computedPosition =
                    computedPosition + child->desiredPosition + Vec2(0.f, offset);
                child->layout(ctx);
                offset += child->computedSize.y + itemSpacing;
            }
        }
    };

    struct Grid : public Widget
    {
        u32 columns;
        f32 itemSpacing;

        Grid(u32 columns, f32 itemSpacing=0.f)
            : Widget("Grid"), columns(columns), itemSpacing(itemSpacing) {}

        Widget* build()
        {
            assert(desiredSize.x > 0.f);
            return this;
        }

        virtual void computeSize(Constraints const& constraints) override
        {
            computedSize.x = clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth);

            Constraints childConstraints;
            childConstraints.minWidth = 0.f;
            childConstraints.maxWidth = computedSize.x / columns;
            childConstraints.minHeight = 0.f;
            childConstraints.maxHeight = constraints.maxHeight;

            f32 totalHeight = 0.f;
            f32 maxRowHeight = 0.f;
            u32 rowChildCount = 0;
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->computeSize(childConstraints);
                maxRowHeight = max(maxRowHeight, child->computedSize.y);

                if (++rowChildCount == columns)
                {
                    totalHeight += maxRowHeight + itemSpacing;
                    maxRowHeight = 0.f;
                    rowChildCount = 0;
                }

                assert(child->desiredSize.y != INFINITY);
            }

            computedSize.y = desiredSize.y > 0.f
                ? clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight)
                : totalHeight - itemSpacing;
        }

        virtual void layout(WidgetContext& ctx) override
        {
            f32 totalHeight = 0.f;
            f32 maxRowHeight = 0.f;
            u32 rowChildCount = 0;
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->computedPosition =
                    computedPosition + Vec2(rowChildCount * computedSize.x / columns, totalHeight);
                child->layout(ctx);

                maxRowHeight = max(maxRowHeight, child->computedSize.y);

                if (++rowChildCount == columns)
                {
                    totalHeight += maxRowHeight + itemSpacing;
                    maxRowHeight = 0.f;
                    rowChildCount = 0;
                }
            }
        }
    };

    struct FlexibleColumn : public Widget { };

    struct FlexibleRow : public Widget { };

    struct ScrollPanel : public Widget
    {
        // TODO: unlink the children that won't be visible, in the earliest pass possible, so they
        // are not processed by later stages
    };

    namespace ButtonFlags
    {
        enum
        {
            BACK,
            TRIGGER_INPUT_CAPTURE,
        };
    };

    struct Button : public Widget
    {
        Container* bg;
        Container* border;
        u32 buttonFlags;

        Button(u32 buttonFlags=0, const char* name="Button")
            : Widget(name), buttonFlags(buttonFlags) {}

        struct ButtonState
        {
            f32 hoverIntensity = 0.f;
            f32 hoverTimer = 0.f;
            f32 hoverValue = 0.f;
            bool isPressed = false;
        };

        Widget* build()
        {
            pushID("btn");

            flags |= WidgetFlags::BLOCK_INPUT | WidgetFlags::SELECTABLE;

            border = (Container*)add(this, Container(Insets(4), {}, {}, Vec4(1)))->size(desiredSize);
            bg = (Container*)add(border,
                    Container(Insets(10), {}, {}, Vec4(0, 0, 0, 1), HAlign::CENTER, VAlign::CENTER))
                ->size(desiredSize);
            return bg;
        }

        CALLBACK(Button, onPress);
        CALLBACK(Button, onSelect);

        void press()
        {
            getState<ButtonState>(this)->isPressed = true;
            INVOKE_CALLBACK(onPress);
        }

        virtual bool handleInputEvent(
                InputCaptureContext& ctx, Widget* inputCapture, InputEvent const& ev) override
        {
            if (ctx.selectedWidgetStateNode == stateNode)
            {
                if (ev.type == INPUT_KEYBOARD_PRESSED)
                {
                    if (ev.keyboard.key == KEY_RETURN
                            || (ev.keyboard.key == KEY_ESCAPE && (buttonFlags & ButtonFlags::BACK)))
                    {
                        press();
                        return true;
                    }
                }
                else if (ev.type == INPUT_CONTROLLER_BUTTON_PRESSED)
                {
                    if (ev.controller.button == BUTTON_B && (buttonFlags & ButtonFlags::BACK))
                    {
                        press();
                        return true;
                    }
                }
            }

            return false;
        }

        virtual bool handleMouseInput(InputCaptureContext& ctx, InputEvent const& input)
            override
        {
            bool isMouseOver =
                pointInRectangle(input.mouse.mousePos, computedPosition, computedPosition + computedSize);

            if (!isMouseOver)
            {
                return false;
            }

            ctx.selectedWidgetStateNode = stateNode;

            if (input.type == INPUT_MOUSE_BUTTON_PRESSED && input.mouse.button == MOUSE_LEFT)
            {
                press();
            }

            return true;
        }

        virtual void render(WidgetContext& ctx) override
        {
            ButtonState* state = getState<ButtonState>(this);

            if (flags & (WidgetFlags::STATUS_SELECTED))
            {
                INVOKE_CALLBACK(onSelect);

                state->hoverIntensity = min(state->hoverIntensity + ctx.deltaTime * 4.f, 1.f);
                state->hoverTimer += ctx.deltaTime;
                state->hoverValue = 0.05f + (sinf(state->hoverTimer * 4.f) + 1.f) * 0.5f * 0.1f;
            }
            else
            {
                state->hoverIntensity = max(state->hoverIntensity - ctx.deltaTime * 2.f, 0.f);
                state->hoverTimer = 0.f;
                state->hoverValue = max(state->hoverValue - ctx.deltaTime, 0.f);
            }

            const Vec4 COLOR_SELECTED = Vec4(1.f, 0.6f, 0.05f, 1.f);
            const Vec4 COLOR_NOT_SELECTED = Vec4(1.f);

            if (state->isPressed)
            {
                border->backgroundColor = COLOR_SELECTED;
                bg->backgroundColor = COLOR_SELECTED;
            }
            else
            {
                border->backgroundColor = mix(COLOR_NOT_SELECTED, COLOR_SELECTED, state->hoverIntensity);
                bg->backgroundColor = Vec4(Vec3(state->hoverValue * state->hoverIntensity), 1.f);
            }
        }
    };

    struct TextButton : public Button
    {
        const char* text;

        TextButton(const char* text, u32 buttonFlags=0)
            : Button(buttonFlags, "TextButton"), text(text) {}

        Widget* build()
        {
            pushID(text);
            auto btn = Button::build();
            Font* font = &g_res.getFont("font", 30);
            add(btn, Text(font, text));
            return btn;
        }
    };

    struct IconButton : public Widget { };

    struct TextEdit : public Widget { };

    struct TextArea : public Widget { };

    struct ColorPicker : public Widget { };

    struct Toggle : public Widget { };

    struct RadioButton : public Widget { };

    struct Slider : public Widget { };

    struct Select : public Widget { };

    struct Tabs : public Widget { };

    struct Window : public Widget {  };

    struct Animation : public Widget
    {
        f32 animationLength;
        bool isDirectionForward = true;

        struct AnimationState
        {
            f32 animationProgress = 0.f;
            bool finished = false;
        };

        Animation(f32 animationLength = 1.f, bool isDirectionForward=true, const char* name="Animation")
            : Widget(name), animationLength(animationLength), isDirectionForward(isDirectionForward) {}

        CALLBACK(Animation, onAnimationEnd);
        CALLBACK(Animation, onAnimationReverseEnd);

        AnimationState* animate()
        {
            auto state = getState<AnimationState>(this);
            if (isDirectionForward)
            {
                state->animationProgress =
                    min(state->animationProgress + g_game.deltaTime * (1.f / animationLength), 1.f);
                if (state->animationProgress == 1.f && !state->finished)
                {
                    INVOKE_CALLBACK(onAnimationEnd);
                    state->finished = true;
                }
            }
            else
            {
                state->animationProgress =
                    max(state->animationProgress - g_game.deltaTime * (1.f / animationLength), 0.f);
                if (state->animationProgress == 0.f && !state->finished)
                {
                    INVOKE_CALLBACK(onAnimationReverseEnd);
                    state->finished = true;
                }
            }
            return state;
        }
    };

    struct SlideAnimation : public Animation
    {
        SlideAnimation(f32 animationLength = 1.f, bool isDirectionForward=true)
            : Animation(animationLength, isDirectionForward, "SlideAnimation") {}

        Widget* build()
        {
            this->pushID("SlideAnimation");
            return this;
        }

        virtual void layout(WidgetContext& ctx) override
        {
            auto state = this->animate();

            //f32 t = easeInOutBezier(state->animationProgress);
            f32 t = easeInOutParametric(state->animationProgress);
            for (Widget* child = this->childFirst; child; child = child->neighbor)
            {
                child->computedPosition = this->computedPosition + child->desiredPosition
                    - Vec2(0, (1.f - t) * g_game.windowHeight);
                child->layout(ctx);
            }
        }
    };

    struct FadeAnimation : public Animation
    {
        FadeAnimation(f32 animationLength = 1.f, bool isDirectionForward=true)
            : Animation(animationLength, isDirectionForward, "FadeAnimation") {}

        Widget* build()
        {
            this->pushID("FadeAnimation");
            return this;
        }

        virtual void layout(WidgetContext& ctx) override
        {
            auto state = this->animate();
            for (Widget* child = this->childFirst; child; child = child->neighbor)
            {
                child->computedPosition = this->computedPosition + child->desiredPosition;
                child->layout(ctx);
            }
        }
    };

    struct InputCapture : public Widget
    {
        InputCapture() : Widget("InputCapture") {}

        Widget* build()
        {
            pushID("InputCapture");
            addInputCapture(this);
            return this;
        }
    };

    void widgetsDemo()
    {
        static bool animateIn = true;

        auto root = add(gui::root, SlideAnimation(1.f, animateIn))
            ->onAnimationEnd([]{
                println("Animation intro finished!");
            })->onAnimationReverseEnd([]{
                println("Animation outro finished!");
            });

        auto container = add(root, Container({}, {}, {}, Vec4(1, 0, 0, 1.f), HAlign::CENTER, VAlign::CENTER))
            ->size(500, 0)->position(200, 200);
        container = add(container, Container(Insets(20), Insets(20), {}, Vec4(0, 0, 0, 0.8f)))
            ->size(Vec2(0));

        Font* font1 = &g_res.getFont("font", 30);
        Font* font2 = &g_res.getFont("font_bold", 40);

        auto column = add(container, Column())
            ->size(Vec2(0));
        auto c = add(column, Container(Insets(5)))
            ->size(Vec2(0));
        add(c, Text(font1, "Hello"));
        c = add(column, Container(Insets(5)))
            ->size(Vec2(0));
        add(c, Text(font2, "World!"));
        c = add(column, Container(Insets(5)))
            ->size(Vec2(0));
        add(c, Text(font1, "Hello"));
        c = add(column, Container(Insets(5)))
            ->size(Vec2(0));
        add(c, Text(font2, "World!"));
        c = add(column, Container(Insets(5)))
            ->size(Vec2(0));
        auto row = add(c, Row())
            ->size(Vec2(0));
        add(row, Text(font1, "Oh my"));
        add(row, Text(font2, "GOODNESS"));

        c = add(column, Container(Insets(5)))
            ->size(Vec2(0))->position(Vec2(0));

        add(c, TextButton("Click Me!!!"))
            ->onPress([] { println("Hello world!"); })
            ->size(0,0)->position(0,0);

        c = add(column, Container(Insets(5)))
            ->size(0,0)->position(0,0);
        add(c, TextButton("Click Me Also!!!"))
            ->onPress([] { println("Greetings, Universe!"); })
            ->size(Vec2(0));

        c = add(column, Container(Insets(5)))
            ->size(0,0)->position(0,0);
        add(c, TextButton("Goodbye", ButtonFlags::BACK))
            ->onPress([] {
                println("Farewell, comos!");
                animateIn = false;
            })
            ->size(0,0)
            ->position(0,0)
            ->addFlags(WidgetFlags::DEFAULT_SELECTION);

        auto gridContainer = add(root,
                Container(Insets(20), {}, {}, Vec4(0, 1, 0, 0.25f), HAlign::CENTER, VAlign::CENTER))
            ->size(600, 400)->position(800, 200);
        auto grid = add(gridContainer, Grid(4));
        for (u32 i=0; i<8; ++i)
        {
            auto container = add(grid,
                Container(Insets(10), {}, {}, Vec4(0), HAlign::CENTER, VAlign::CENTER))
                ->size(INFINITY, 80);
            add(container, TextButton(tmpStr("Button %i", i)))->onPress([]{
                println("clicked");
            });
        }
    }
};
