#include "gui.h"

namespace gui
{
    void onBeginUpdate(f32 deltaTime)
    {
        widgetBuffer.clear();
        widgetCount = 0;
        inputCaptureWidgets.clear();

        root = widgetBuffer.write<Widget>(Widget("Root"));
        root->root = root;
        root->stateNode = &widgetStateNodeStorage.front();

        addInputCapture(root);
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

        if (inputCaptureStack.empty())
        {
            inputCaptureStack.push({ root->stateNode, nullptr });
        }

        auto& context = inputCaptureStack.back();

        Widget** activeInputCapture =
            inputCaptureWidgets.findIf([&](Widget* w) {
                return w->stateNode == context.inputCaptureStateNode;
            });
        assert(activeInputCapture);

        WidgetStateNode* previouslySelectedWidgetStateNode = context.selectedWidgetStateNode;
        for (auto const& ev : g_input.getEvents())
        {
            if (!(*activeInputCapture)->handleInputCaptureEvent(context, ev))
            {
                // TODO: What should happen in this case?
                //root->handleInputCaptureEvent(inputCaptureStack.front(), ev);
            }
        }

        if (context.selectedWidgetStateNode)
        {
            Widget* selectedWidget = findAncestorByStateNode(
                    *activeInputCapture, context.selectedWidgetStateNode);
            if (selectedWidget)
            {
                if (previouslySelectedWidgetStateNode != selectedWidget->stateNode)
                {
                    selectedWidget->flags |= WidgetFlags::STATUS_GAINED_SELECTED;
                }

                selectedWidget->flags |= WidgetFlags::STATUS_SELECTED;
            }
        }

        root->doRender(ctx);
    }

    void Widget::computeSize(Constraints const& constraints)
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

    void Widget::layout(WidgetContext& ctx)
    {
        for (Widget* child = childFirst; child; child = child->neighbor)
        {
            child->computedPosition = computedPosition + child->desiredPosition;
            child->layout(ctx);
        }
    }

    bool Widget::handleInputCaptureEvent(InputCaptureContext& ctx, InputEvent const& ev)
    {
        if (ev.type <= INPUT_MOUSE_MOVE)
        {
            // pipe mouse events to the widgets that the mouse is over
            handleMouseInput(ctx, ev);
        }

        Widget* selectedWidget = nullptr;
        if (ctx.selectedWidgetStateNode)
        {
            selectedWidget = findAncestorByStateNode(this, ctx.selectedWidgetStateNode);
        }
        else
        {
            selectedWidget = findAncestorByFlags(this,
                    WidgetFlags::SELECTABLE | WidgetFlags::DEFAULT_SELECTION);
            if (!selectedWidget)
            {
                selectedWidget = findAncestorByFlags(this, WidgetFlags::SELECTABLE);
            }
            ctx.selectedWidgetStateNode = selectedWidget->stateNode;
        }

        if (selectedWidget)
        {
            if (selectedWidget->handleInputEvent(ctx, this, ev))
            {
                return true;
            }

            i32 motionX = 0;
            i32 motionY = 0;
            switch (ev.type)
            {
                case INPUT_KEYBOARD_PRESSED:
                case INPUT_KEYBOARD_REPEAT:
                    if (ev.keyboard.key == KEY_LEFT) motionX = -1;
                    else if (ev.keyboard.key == KEY_RIGHT) motionX = 1;
                    else if (ev.keyboard.key == KEY_UP) motionY = -1;
                    else if (ev.keyboard.key == KEY_DOWN) motionY = 1;
                    break;
                case INPUT_CONTROLLER_BUTTON_PRESSED:
                case INPUT_CONTROLLER_BUTTON_REPEAT:
                    if (ev.controller.button == BUTTON_DPAD_LEFT) motionX = -1;
                    else if (ev.controller.button == BUTTON_DPAD_RIGHT) motionX = 1;
                    else if (ev.controller.button == BUTTON_DPAD_UP) motionY = -1;
                    else if (ev.controller.button == BUTTON_DPAD_DOWN) motionY = 1;
                    break;
                case INPUT_CONTROLLER_AXIS_TRIGGERED:
                case INPUT_CONTROLLER_AXIS_REPEAT:
                    if (ev.controller.axis == AXIS_RIGHT_X) motionX = sign(ev.controller.axisVal);
                    else if (ev.controller.axis == AXIS_RIGHT_Y) motionY = sign(ev.controller.axisVal);
                    break;
                default:
                    break;
            }

            if (motionX || motionY)
            {
                Widget* previousSelectedWidget = selectedWidget;

                Navigation nav = {};
                nav.motionX = motionX;
                nav.motionY = motionY;
                nav.xRef = selectedWidget->center().x + (selectedWidget->computedSize.x * motionX * 0.5f);
                nav.yRef = selectedWidget->center().y + (selectedWidget->computedSize.y * motionY * 0.5f);
                nav.fromPos = selectedWidget->center();
                nav.fromSize = selectedWidget->computedSize;
                navigate(nav);

                if (nav.minSelectTargetX)
                {
                    selectedWidget = nav.minSelectTargetX;
                }
                else if (nav.maxSelectTargetX)
                {
                    selectedWidget = nav.maxSelectTargetX;
                }
                if (nav.minSelectTargetY)
                {
                    selectedWidget = nav.minSelectTargetY;
                }
                else if (nav.maxSelectTargetY)
                {
                    selectedWidget = nav.maxSelectTargetY;
                }

                if (selectedWidget != previousSelectedWidget)
                {
                    ctx.selectedWidgetStateNode = selectedWidget->stateNode;
                    g_audio.playSound(g_res.getSound("select"), SoundType::MENU_SFX);
                }

                return true;
            }
        }

        return false;
    }

    void Widget::navigate(Navigation& nav)
    {
        for (Widget* child = childFirst; child; child=child->neighbor)
        {
            if (child->flags & WidgetFlags::SELECTABLE)
            {
                Vec2 c = child->center();
                Vec2 s = child->computedSize;
                if (nav.motionX)
                {
                    f32 xDist = nav.motionX * ((c.x - (s.x * nav.motionX) * 0.5f) - nav.xRef);
                    //f32 yDist = absolute((c.y + s.y * 0.5f) - (nav.fromPos.y + nav.fromSize.y * 0.5f)) * 0.5f;
                    f32 yDist = absolute(c.y - nav.fromPos.y) * 0.5f;
                    if (xDist > 0.f && xDist + yDist < nav.minDistX)
                    {
                        nav.minDistX = xDist + yDist;
                        nav.minSelectTargetX = child;
                    }
                    if (xDist + yDist < nav.maxDistX)
                    {
                        nav.maxDistX = xDist + yDist;
                        nav.maxSelectTargetX = child;
                    }
                }
                if (nav.motionY)
                {
                    f32 yDist = nav.motionY * ((c.y - (s.y * nav.motionY) * 0.5f) - nav.yRef);
                    //f32 xDist = absolute((c.x + s.x * 0.5f) - (nav.fromPos.x + nav.fromSize.x * 0.5f)) * 0.5f;
                    f32 xDist = absolute(c.x - nav.fromPos.x) * 0.5f;
                    if (yDist > 0.f && yDist + xDist < nav.minDistY)
                    {
                        nav.minDistY = yDist + xDist;
                        nav.minSelectTargetY = child;
                    }
                    if (yDist + xDist < nav.maxDistY)
                    {
                        nav.maxDistY = yDist + xDist;
                        nav.maxSelectTargetY = child;
                    }
                }
            }
            child->navigate(nav);
        }
    }

    bool Widget::handleMouseInput(InputCaptureContext& ctx, InputEvent const& input)
    {
        bool isMouseOver =
            pointInRectangle(input.mouse.mousePos, computedPosition, computedPosition + computedSize);

        if (!isMouseOver)
        {
            return false;
        }

        if (flags & WidgetFlags::BLOCK_INPUT)
        {
            flags |= WidgetFlags::STATUS_MOUSE_OVER;
            return true;
        }

        for (Widget* child = childFirst; child; child = child->neighbor)
        {
            if (child->handleMouseInput(ctx, input))
            {
                return true;
            }
        }

        return false;
    }

    void Widget::doRender(WidgetContext& ctx)
    {
        // TODO: Don't perform scaling like this; doesn't work well with fonts
        computedSize *= ctx.scale;
        computedPosition *= ctx.scale;
        this->render(ctx);

        for (Widget* child = childFirst; child; child = child->neighbor)
        {
            child->doRender(ctx);
        }
    }

    void Widget::pushID(const char* id)
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

    Widget* findAncestorByStateNode(Widget* w, WidgetStateNode* state)
    {
        if (w->stateNode == state)
        {
            return w;
        }
        for (Widget* child = w->childFirst; child; child = child->neighbor)
        {
            Widget* t = findAncestorByStateNode(child, state);
            if (t)
            {
                return t;
            }
        }
        return nullptr;
    }

    Widget* findAncestorByFlags(Widget* w, u32 flags)
    {
        if ((w->flags & flags) == flags)
        {
            return w;
        }
        for (Widget* child = w->childFirst; child; child = child->neighbor)
        {
            Widget* t = findAncestorByFlags(child, flags);
            if (t)
            {
                return t;
            }
        }
        return nullptr;
    }

    Widget* findParent(Widget* w, u32 flags)
    {
        for (Widget* p = w; p; p = p->parent)
        {
            if ((p->flags & flags) == flags)
            {
                return p;
            }
        }
        return nullptr;
    }
};
