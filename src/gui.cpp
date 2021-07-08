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
        root->stateNode = &widgetStateNodeStorage[0];
        root->addFlags(WidgetFlags::INPUT_CAPTURE);

        nullWidget = widgetBuffer.write<Widget>(Widget("Null Widget"));
        nullWidget->root = nullWidget;
        nullWidget->stateNode = &widgetStateNodeStorage[1];

        addInputCapture(root);
        if (inputCaptureStack.empty())
        {
            inputCaptureStack.push({ root->stateNode, nullptr });
        }
    }

    void onUpdate(Renderer* renderer, i32 w, i32 h, f32 deltaTime, u32 count)
    {
        InputCaptureContext context = inputCaptureStack.back();

        frameCount = count;

        const f32 REFERENCE_HEIGHT = 1080.f;
        f32 aspectRatio = (f32)w / (f32)h;
        Vec2 referenceScreenSize(REFERENCE_HEIGHT * aspectRatio, REFERENCE_HEIGHT);
        Vec2 actualScreenSize((f32)w, (f32)h);

        GuiContext ctx;
        ctx.deltaTime = deltaTime;
        ctx.aspectRatio = aspectRatio;
        ctx.referenceScreenSize = referenceScreenSize;
        ctx.actualScreenSize = actualScreenSize;

        root->desiredSize = { (f32)w, (f32)h };
        root->computeSize(Constraints());
        root->layout(ctx);

        Widget** activeInputCapture =
            inputCaptureWidgets.findIf([&](Widget* w) {
                return w->stateNode == context.inputCaptureStateNode;
            });
        assert(activeInputCapture);

        WidgetStateNode* previouslySelectedWidgetStateNode = context.selectedWidgetStateNode;
        Widget* selectedWidget =
            (*activeInputCapture)->findSelectedWidget(context.selectedWidgetStateNode);
        if (selectedWidget)
        {
            context.selectedWidgetStateNode = selectedWidget->stateNode;
        }

        for (auto const& ev : g_input.getEvents())
        {
            if (!(*activeInputCapture)->handleInputCaptureEvent(ctx, context, ev, selectedWidget))
            {
                // TODO: loop through other input captures and see if they will handle the event
                // e.g. selecting something with the mouse
            }
        }

        if (context.selectedWidgetStateNode)
        {
            if (previouslySelectedWidgetStateNode != context.selectedWidgetStateNode)
            {
                selectedWidget = findAncestorByStateNode(
                        *activeInputCapture, context.selectedWidgetStateNode);
                selectedWidget->flags |= WidgetFlags::STATUS_GAINED_SELECTED;
            }
            if (previousInputCaptureContext.inputCaptureStateNode != context.inputCaptureStateNode
                    && selectedWidget)
            {
                selectedWidget->flags |= WidgetFlags::STATUS_GAINED_SELECTED;
            }
            if (selectedWidget)
            {
                selectedWidget->flags |= WidgetFlags::STATUS_SELECTED;
                selectedWidget->handleStatus();
            }
        }
        previousInputCaptureContext = context;

        // if the input context the frame started with is still active, then assign the selected widget
        if (inputCaptureStack.back().inputCaptureStateNode == context.inputCaptureStateNode)
        {
            inputCaptureStack.back().selectedWidgetStateNode = context.selectedWidgetStateNode;
        }

        RenderContext rtx;
        rtx.renderer = renderer;
        rtx.transform = Mat4(1.f);
        rtx.alpha = 1.f;
        rtx.scissorPos = Vec2(0, 0);
        rtx.scissorSize = actualScreenSize;
        rtx.priority = 0;
        root->render(ctx, rtx);
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

    void Widget::layout(GuiContext const& gtx)
    {
        for (Widget* child = childFirst; child; child = child->neighbor)
        {
            child->positionWithinParent = child->desiredPosition;
            child->computedPosition = computedPosition + child->positionWithinParent;
            child->layout(gtx);
        }
    }

    Widget* Widget::findSelectedWidget(WidgetStateNode* stateNode)
    {
        Widget* selectedWidget = nullptr;
        if (stateNode)
        {
            if (this->stateNode == stateNode)
            {
                selectedWidget = this;
            }
            else
            {
                selectedWidget = findAncestorByStateNode(this, stateNode);
            }
        }

        if (!stateNode || !selectedWidget)
        {
            u32 matchFlags = WidgetFlags::SELECTABLE | WidgetFlags::DEFAULT_SELECTION;
            if ((this->flags & matchFlags) == matchFlags)
            {
                selectedWidget = this;
            }
            else
            {
                selectedWidget = findAncestorByFlags(this, matchFlags, WidgetFlags::DISABLED);
                if (!selectedWidget)
                {
                    if ((this->flags & WidgetFlags::SELECTABLE) == WidgetFlags::SELECTABLE)
                    {
                        selectedWidget = this;
                    }
                    else
                    {
                        // select the first selectable widget by default
                        selectedWidget = findAncestorByFlags(this, WidgetFlags::SELECTABLE,
                                WidgetFlags::DISABLED);
                    }
                }
            }
        }

        return selectedWidget;
    }

    bool Widget::handleInputCaptureEvent(GuiContext const& gtx, InputCaptureContext& ctx,
            InputEvent const& ev, Widget* selectedWidget)
    {
        if (ev.type == INPUT_MOUSE_MOVE ||
            ev.type == INPUT_MOUSE_BUTTON_PRESSED ||
            ev.type == INPUT_MOUSE_BUTTON_DOWN ||
            ev.type == INPUT_MOUSE_BUTTON_RELEASED)
        {
            // pipe mouse events to the widgets that the mouse is over
            InputEvent mouseEvent = ev;
            return handleMouseInput(gtx, ctx, mouseEvent);
        }

        if (selectedWidget)
        {
            if (selectedWidget->handleInputEvent(gtx, ctx, this, ev))
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

        return handleInputEvent(gtx, ctx, this, ev);
    }

    bool Widget::handleInputEvent(GuiContext const& gtx,
            InputCaptureContext& ctx, Widget* inputCapture, InputEvent const& ev)
    {
        for (Widget* child = childFirst; child; child=child->neighbor)
        {
            if (child->handleInputEvent(gtx, ctx, inputCapture, ev))
            {
                return true;
            }
        }
        return false;
    }

    void Widget::navigate(Navigation& nav)
    {
        for (Widget* child = childFirst; child; child=child->neighbor)
        {
            if ((child->flags & WidgetFlags::SELECTABLE) && !(child->flags & WidgetFlags::DISABLED))
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
            if (!(child->flags & WidgetFlags::INPUT_CAPTURE))
            {
                child->navigate(nav);
            }
        }
    }

    bool Widget::handleMouseInput(GuiContext const& gtx, InputCaptureContext& ctx, InputEvent const& input)
    {
        bool isMouseOver =
            pointInRectangle(input.mouse.mousePos, computedPosition, computedPosition + computedSize);

        if (!isMouseOver)
        {
            return false;
        }

        for (Widget* child = childFirst; child; child = child->neighbor)
        {
            if (child->handleMouseInput(gtx, ctx, input))
            {
                return true;
            }
        }

        return false;
    }

    void Widget::render(GuiContext const& ctx, RenderContext const& rtx)
    {
        for (Widget* child = childFirst; child; child = child->neighbor)
        {
            child->render(ctx, rtx);
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
        for (Widget* child = w->childFirst; child; child = child->neighbor)
        {
            if (child->stateNode == state)
            {
                return child;
            }
            if (!(child->flags & (WidgetFlags::INPUT_CAPTURE | WidgetFlags::DISABLED)))
            {
                Widget* t = findAncestorByStateNode(child, state);
                if (t)
                {
                    return t;
                }
            }
        }
        return nullptr;
    }

    Widget* findAncestorByFlags(Widget* w, u32 matchFlags, u32 unmatchFlags)
    {
        for (Widget* child = w->childFirst; child; child = child->neighbor)
        {
            if ((child->flags & matchFlags) == matchFlags &&
                    (~child->flags & unmatchFlags) == unmatchFlags)
            {
                return child;
            }

            if (!(child->flags & (WidgetFlags::INPUT_CAPTURE | WidgetFlags::DISABLED)))
            {
                Widget* t = findAncestorByFlags(child, matchFlags, unmatchFlags);
                if (t)
                {
                    return t;
                }
            }
        }
        return nullptr;
    }

    /*
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
    */
};
