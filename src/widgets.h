#pragma once

#include "gui.h"
#include "2d.h"
#include "font.h"

namespace gui
{
    WIDGET(Panel) {};

    WIDGET(Text)
    {
        Font* font;
        const char* text;
        Vec4 color;
        HAlign halign = HAlign::LEFT;
        VAlign valign = VAlign::TOP;

        Text(Font* font, const char* text, Vec4 color=Vec4(1.0),
                HAlign halign=HAlign::LEFT, VAlign valign=VAlign::TOP)
            : font(font), text(text), color(color), halign(halign), valign(valign) {}

        virtual void layout(Constraints const& constraints) override
        {
            desiredSize = font->stringDimensions(text);

            computedSize.x = clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth);
            computedSize.y = clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight);

            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->computedPosition = this->computedPosition + child->desiredPosition;
                child->layout(constraints);
            }
        }

        virtual void onRender(Renderer* renderer) override
        {
            ui::text(font, text, computedPosition, Vec3(1.f), 1.f);
        }
    };

    WIDGET(Image) { };

    WIDGET(Container)
    {
        Insets padding;
        Insets margin;
        Constraints additionalConstraints;
        Vec4 backgroundColor = Vec4(0);

        Container() {}
        Container(Insets const& padding, Insets const& margin = {},
                Constraints const& additionalConstraints = {}, Vec4 const& backgroundColor = Vec4(0))
            : padding(padding), margin(margin), additionalConstraints(additionalConstraints),
              backgroundColor(backgroundColor) {}

        Container(Vec4 const& backgroundColor = Vec4(0))
            : backgroundColor(backgroundColor) {}

        virtual void layout(Constraints const& constraints) override
        {
            computedSize.x = clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth);
            computedSize.y = clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight);
            computedSize.x =
                clamp(computedSize.x, additionalConstraints.minWidth, additionalConstraints.maxWidth);
            computedSize.y =
                clamp(computedSize.y, additionalConstraints.minHeight, additionalConstraints.maxHeight);

            f32 horizontalInset = padding.left + padding.right + margin.left + margin.right;
            f32 verticalInset = padding.top + padding.bottom + margin.top + margin.bottom;

            Constraints childConstraints;
            childConstraints.minWidth = 0.f;
            childConstraints.maxWidth = max(computedSize.x - horizontalInset, 0.f);
            childConstraints.minHeight = 0.f;
            childConstraints.maxHeight = max(computedSize.y - verticalInset, 0.f);

            assert(childConstraints.minWidth <= childConstraints.maxWidth);
            assert(childConstraints.minHeight <= childConstraints.maxHeight);

            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->computedPosition = this->computedPosition + child->desiredPosition
                    + Vec2(padding.left + margin.left, padding.top + margin.top);
                child->layout(childConstraints);
            }
        }

        virtual void onRender(Renderer* renderer) override
        {
            if (backgroundColor.a > 0.f)
            {
                ui::rect(0, nullptr, computedPosition + Vec2(margin.left, margin.top),
                    computedSize - Vec2(margin.left + margin.right, margin.top + margin.bottom),
                    backgroundColor.rgb, backgroundColor.a);
            }
        }
    };

    WIDGET(Row)
    {
        virtual void layout(Constraints const& constraints) override
        {
            computedSize.y = clamp(desiredSize.y, constraints.minHeight, constraints.maxHeight);

            // TODO: Would it be better to store the count?
            u32 count = 0;
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                ++count;
            }

            Constraints childConstraints;
            childConstraints.minWidth = 0.f;
            childConstraints.maxWidth = constraints.maxWidth / count;
            childConstraints.minHeight = 0.f;
            childConstraints.maxHeight = computedSize.y;

            assert(childConstraints.minWidth <= childConstraints.maxWidth);
            assert(childConstraints.minHeight <= childConstraints.maxHeight);

            f32 offsetX = 0.f;
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->computedPosition = this->computedPosition + Vec2(offsetX, 0);
                child->layout(childConstraints);
                assert(child->computedSize.x < 10000.f);
                offsetX += child->computedSize.x;
            }

            computedSize.x =
                clamp(max(desiredSize.x, offsetX), constraints.minWidth, constraints.maxWidth);
        }
    };

    WIDGET(Column)
    {
        virtual void layout(Constraints const& constraints) override
        {
            computedSize.x = clamp(desiredSize.x, constraints.minWidth, constraints.maxWidth);

            // TODO: Would it be better to store the count?
            u32 count = 0;
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                ++count;
            }

            Constraints childConstraints;
            childConstraints.minWidth = 0.f;
            childConstraints.maxWidth = computedSize.x;
            childConstraints.minHeight = 0.f;
            childConstraints.maxHeight = constraints.maxHeight / count;

            assert(childConstraints.minWidth <= childConstraints.maxWidth);
            assert(childConstraints.minHeight <= childConstraints.maxHeight);

            f32 offsetY = 0.f;
            for (Widget* child = childFirst; child; child = child->neighbor)
            {
                child->computedPosition = this->computedPosition + Vec2(0, offsetY);
                child->layout(childConstraints);
                assert(child->computedSize.y < 10000.f);
                offsetY += child->computedSize.y;
            }

            computedSize.y =
                clamp(max(desiredSize.y, offsetY), constraints.minHeight, constraints.maxHeight);
        }
    };

    WIDGET(Grid) { };

    WIDGET(ResponsiveStack) { };

    WIDGET(ScrollPanel) { };

    WIDGET(TextButton) { };

    WIDGET(IconButton) { };

    WIDGET(TextEdit) { };

    WIDGET(TextArea) { };

    WIDGET(ColorPicker) { };

    WIDGET(Toggle) { };

    WIDGET(RadioButton) { };

    WIDGET(Slider) { };

    WIDGET(Select) { };

    WIDGET(Tabs) { };

    WIDGET(Window) {  };

    void widgetsDemo()
    {
        auto container = add(&gui::root, Container({}, {}, {}, Vec4(1, 0, 0, 1.f)), Vec2(200), Vec2(350));
        container = add(container, Container(Insets(20), Insets(20), {}, Vec4(0, 0, 0, 0.8f)), Vec2(0, 0));

        Font* font1 = &g_res.getFont("font", 30);
        Font* font2 = &g_res.getFont("font_bold", 40);

        auto column = add(container, Column());
        add(column, Text(font1, "Hello"));
        add(column, Text(font2, "World!"));
        add(column, Text(font1, "Hello"));
        add(column, Text(font2, "World!"));
        auto row = add(column, Row());
        add(row, Text(font1, "Hello"));
        add(row, Text(font2, "World"));
    }
};
