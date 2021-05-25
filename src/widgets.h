#pragma once

#include "gui.h"
#include "2d.h"
#include "font.h"

namespace gui
{
    struct Stack : public Widget {};

    struct Text : public Widget
    {
        Font* font;
        const char* text;
        Vec4 color;

        Text(Font* font, const char* text, Vec4 color=Vec4(1.0))
            : font(font), text(text), color(color) {}

        virtual void computeSize(Constraints const& constraints) override
        {
            desiredSize = font->stringDimensions(text);
            computedSize.x = clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth);
            computedSize.y = clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight);

            assert(childFirst == nullptr);
            assert(constraints.maxWidth > 0);
            assert(constraints.maxHeight > 0);
        }

        virtual void render(RenderContext& ctx) override
        {
            ui::text(font, text, computedPosition, Vec3(1.f), 1.f);
        }
    };

    struct Image : public Widget
    {
        Texture* tex;
        bool preserveAspectRatio = true;

        Image(Texture* tex, bool preserveAspectRatio=true)
            : tex(tex), preserveAspectRatio(preserveAspectRatio) {}
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

        virtual void render(RenderContext& ctx) override
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

        Container() {}
        Container(Insets const& padding, Insets const& margin = {},
                Constraints const& additionalConstraints = {}, Vec4 const& backgroundColor = Vec4(0),
                HAlign halign = HAlign::LEFT, VAlign valign = VAlign::TOP)
            : padding(padding), margin(margin), additionalConstraints(additionalConstraints),
              backgroundColor(backgroundColor), halign(halign), valign(valign) {}

        Container(Vec4 const& backgroundColor = Vec4(0))
            : backgroundColor(backgroundColor) {}

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

                childFirst->computeSize(childConstraints);

                // TODO: Should be a standard way to specify that a widget has at most one child
                assert(!childFirst->neighbor);
            }

            if (desiredSize.x == 0.f)
            {
                assert(childFirst);
                desiredSize.x = childFirst->computedSize.x + horizontalInset;
            }
            computedSize.x = clamp(desiredSize.x, actualConstraints.minWidth, actualConstraints.maxWidth);

            if (desiredSize.y == 0.f)
            {
                assert(childFirst);
                desiredSize.y = childFirst->computedSize.y + verticalInset;
            }
            computedSize.y = clamp(desiredSize.y, actualConstraints.minHeight, actualConstraints.maxHeight);
        }

        virtual void layout() override
        {
            if (childFirst)
            {
                Vec2 childPos(0, 0);

                if (halign == HAlign::LEFT)
                {
                    childPos.x = margin.left + padding.left;
                }
                else if (halign == HAlign::RIGHT)
                {
                    childPos.x =
                        computedSize.x - childFirst->computedSize.x - margin.right - padding.right;
                }
                else if (halign == HAlign::CENTER)
                {
                    childPos.x = computedSize.x * 0.5f - childFirst->computedSize.x * 0.5f;
                }

                if (valign == VAlign::TOP)
                {
                    childPos.y = margin.top + padding.top;
                }
                else if (valign == VAlign::BOTTOM)
                {
                    childPos.y =
                        computedSize.y - childFirst->computedSize.y - margin.bottom - padding.bottom;
                }
                else if (valign == VAlign::CENTER)
                {
                    childPos.y = computedSize.y * 0.5f - childFirst->computedSize.y * 0.5f;
                }

                childFirst->computedPosition =
                    computedPosition + childFirst->desiredPosition + childPos;
                childFirst->layout();
            }
        }

        virtual void render(RenderContext& ctx) override
        {
            if (backgroundColor.a > 0.f)
            {
                ui::rect(0, nullptr, computedPosition + Vec2(margin.left, margin.top),
                    computedSize - Vec2(margin.left + margin.right, margin.top + margin.bottom),
                    backgroundColor.rgb, backgroundColor.a);
            }
        }
    };

    struct Row : public Widget
    {
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
                totalWidth += child->computedSize.x;
                maxHeight = max(maxHeight, child->computedSize.y);
                assert(child->desiredSize.x != INFINITY);
            }

            if (desiredSize.x == 0.f)
            {
                desiredSize.x = totalWidth;
            }
            computedSize.x = clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth);

            if (desiredSize.y == 0.f)
            {
                desiredSize.y = maxHeight;
            }
            computedSize.y = clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight);
        }

        virtual void layout() override
        {
            f32 offset = 0;
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->computedPosition =
                    computedPosition + child->desiredPosition + Vec2(offset, 0.f);
                child->layout();
                offset += child->computedSize.x;
            }
        }
    };

    struct Column : public Widget
    {
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
                totalHeight += child->computedSize.y;
                maxWidth = max(maxWidth, child->computedSize.x);
                assert(child->desiredSize.y != INFINITY);
            }

            if (desiredSize.x == 0.f)
            {
                desiredSize.x = maxWidth;
            }
            computedSize.x = clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth);

            if (desiredSize.y == 0.f)
            {
                desiredSize.y = totalHeight;
            }
            computedSize.y = clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight);
        }

        virtual void layout() override
        {
            f32 offset = 0;
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->computedPosition =
                    computedPosition + child->desiredPosition + Vec2(0.f, offset);
                child->layout();
                offset += child->computedSize.y;
            }
        }
    };

    struct Grid : public Widget { };

    struct FlexibleColumn : public Widget { };

    struct FlexibleRow : public Widget { };

    struct ScrollPanel : public Widget
    {
        // TODO: unlink the children that won't be visible, in the earliest pass possible, so they
        // are not processed by later stages
    };

    template <typename C>
    struct Button : public Widget
    {
        C clickCallback;
        Container* bg;

        Button(C const& callback) : clickCallback(callback) {}

        struct ButtonState
        {
            f32 hoverIntensity = 0.f;
        };

        Widget* build()
        {
            auto border = add(this, Container(Insets(4), {}, {}, Vec4(1)), Vec2(0), desiredSize);
            bg = add(border,
                    Container(Insets(10), {}, {}, Vec4(0, 0, 0, 1), HAlign::CENTER, VAlign::CENTER),
                    Vec2(0), desiredSize);
            return bg;
        }

        virtual void render(RenderContext& ctx) override
        {
            ctx.keyBuf.write(".btn");
            ButtonState* state = getState<ButtonState>(ctx.keyBuf);
            state->hoverIntensity += ctx.deltaTime + computedPosition.y * 0.0001f;
            bg->backgroundColor = Vec4(Vec3((sinf(state->hoverIntensity) + 1.f) * 0.5f), 1.f);
        }
    };

    template <typename C>
    struct TextButton : public Button<C>
    {
        const char* text;

        TextButton(const char* text, C const& callback=[]{}) : Button<C>(callback), text(text) {}

        Widget* build()
        {
            auto btn = Button<C>::build();
            Font* font = &g_res.getFont("font", 30);
            add(btn, Text(font, text));
            return btn;
        }

        virtual void render(RenderContext& ctx) override
        {
            ctx.keyBuf.write(".txtbtn.");
            ctx.keyBuf.write(text);
            Button<C>::render(ctx);
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

    void widgetsDemo()
    {
        add(&gui::root, Container({}, {}, {}, Vec4(0, 1, 0, 0.5f)), Vec2(50), Vec2(20));
        add(&gui::root, Container({}, {}, {}, Vec4(0, 1, 0, 0.5f)), Vec2(100), Vec2(40));

        auto container = add(&gui::root,
                Container({}, {}, {}, Vec4(1, 0, 0, 1.f), HAlign::CENTER, VAlign::CENTER),
                Vec2(200), Vec2(350, 450));
        container = add(container, Container(Insets(20), Insets(20), {}, Vec4(0, 0, 0, 0.8f)), Vec2(0), Vec2(0));

        Font* font1 = &g_res.getFont("font", 30);
        Font* font2 = &g_res.getFont("font_bold", 40);

        auto column = add(container, Column(), Vec2(0), Vec2(0));
        auto c = add(column, Container(Insets(5)), Vec2(0), Vec2(0));
        add(c, Text(font1, "Hello"));
        c = add(column, Container(Insets(5)), Vec2(0), Vec2(0));
        add(c, Text(font2, "World!"));
        c = add(column, Container(Insets(5)), Vec2(0), Vec2(0));
        add(c, Text(font1, "Hello"));
        c = add(column, Container(Insets(5)), Vec2(0), Vec2(0));
        add(c, Text(font2, "World!"));
        c = add(column, Container(Insets(5)), Vec2(0), Vec2(0));
        auto row = add(c, Row(), Vec2(0), Vec2(0));
        add(row, Text(font1, "Oh my"));
        add(row, Text(font2, "GOODNESS"));

        c = add(column, Container(Insets(5)), Vec2(0), Vec2(0));
        add(c, TextButton("Click Me!!!", [] { println("Hello world!"); }), Vec2(0), Vec2(0, 0));

        c = add(column, Container(Insets(5)), Vec2(0), Vec2(0));
        add(c, TextButton("Click Me Also!!!", [] { println("Greetings, Universe!"); }), Vec2(0), Vec2(0, 0));

        c = add(column, Container(Insets(5)), Vec2(0), Vec2(0));
        add(c, TextButton("Goodbye", [] { println("Farewell, comos!"); }), Vec2(0), Vec2(0, 0));
    }
};
