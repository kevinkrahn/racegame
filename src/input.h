#pragma once

#include <SDL2/SDL.h>
#include "misc.h"

// These values line up with SDL_Button* defines
enum MouseButton
{
    MOUSE_UNKNOWN = 0,
    MOUSE_LEFT,
    MOUSE_MIDDLE,
    MOUSE_RIGHT,
    MOUSE_X1,
    MOUSE_X2,
    MOUSE_BUTTON_COUNT
};

enum Key
{
    KEY_UNKNOWN = 0,
    KEY_A = 4,
    KEY_B = 5,
    KEY_C = 6,
    KEY_D = 7,
    KEY_E = 8,
    KEY_F = 9,
    KEY_G = 10,
    KEY_H = 11,
    KEY_I = 12,
    KEY_J = 13,
    KEY_K = 14,
    KEY_L = 15,
    KEY_M = 16,
    KEY_N = 17,
    KEY_O = 18,
    KEY_P = 19,
    KEY_Q = 20,
    KEY_R = 21,
    KEY_S = 22,
    KEY_T = 23,
    KEY_U = 24,
    KEY_V = 25,
    KEY_W = 26,
    KEY_X = 27,
    KEY_Y = 28,
    KEY_Z = 29,
    KEY_1 = 30,
    KEY_2 = 31,
    KEY_3 = 32,
    KEY_4 = 33,
    KEY_5 = 34,
    KEY_6 = 35,
    KEY_7 = 36,
    KEY_8 = 37,
    KEY_9 = 38,
    KEY_0 = 39,
    KEY_RETURN = 40,
    KEY_ESCAPE = 41,
    KEY_BACKSPACE = 42,
    KEY_TAB = 43,
    KEY_SPACE = 44,
    KEY_MINUS = 45,
    KEY_EQUALS = 46,
    KEY_LEFTBRACKET = 47,
    KEY_RIGHTBRACKET = 48,
    KEY_BACKSLASH = 49,
    KEY_NONUSHASH = 50,
    KEY_SEMICOLON = 51,
    KEY_APOSTROPHE = 52,
    KEY_GRAVE = 53,
    KEY_COMMA = 54,
    KEY_PERIOD = 55,
    KEY_SLASH = 56,
    KEY_CAPSLOCK = 57,
    KEY_F1 = 58,
    KEY_F2 = 59,
    KEY_F3 = 60,
    KEY_F4 = 61,
    KEY_F5 = 62,
    KEY_F6 = 63,
    KEY_F7 = 64,
    KEY_F8 = 65,
    KEY_F9 = 66,
    KEY_F10 = 67,
    KEY_F11 = 68,
    KEY_F12 = 69,
    KEY_PRINTSCREEN = 70,
    KEY_SCROLLLOCK = 71,
    KEY_PAUSE = 72,
    KEY_INSERT = 73,
    KEY_HOME = 74,
    KEY_PAGEUP = 75,
    KEY_DELETE = 76,
    KEY_END = 77,
    KEY_PAGEDOWN = 78,
    KEY_RIGHT = 79,
    KEY_LEFT = 80,
    KEY_DOWN = 81,
    KEY_UP = 82,
    KEY_LCTRL = 224,
    KEY_LSHIFT = 225,
    KEY_LALT = 226,
    KEY_LGUI = 227,
    KEY_RCTRL = 228,
    KEY_RSHIFT = 229,
    KEY_RALT = 230,
    KEY_RGUI = 231,

    KEY_COUNT = 512
};

enum ControllerButton
{
    BUTTON_A,
    BUTTON_B,
    BUTTON_X,
    BUTTON_Y,
    BUTTON_BACK,
    BUTTON_GUIDE,
    BUTTON_START,
    BUTTON_LEFT_STICK,
    BUTTON_RIGHT_STICK,
    BUTTON_LEFT_SHOULDER,
    BUTTON_RIGHT_SHOULDER,
    BUTTON_DPAD_UP,
    BUTTON_DPAD_DOWN,
    BUTTON_DPAD_LEFT,
    BUTTON_DPAD_RIGHT,

    BUTTON_COUNT
};

enum ControllerAxis
{
    AXIS_LEFT_X,
    AXIS_LEFT_Y,
    AXIS_RIGHT_X,
    AXIS_RIGHT_Y,
    AXIS_TRIGGER_LEFT,
    AXIS_TRIGGER_RIGHT,

    AXIS_COUNT
};

struct MouseEvent
{
    Vec2 mousePos;
    Vec2 mouseDelta;
    MouseButton button;
};

struct MouseWheelEvent
{
    i32 scrollX;
    i32 scrollY;
};

struct KeyboardEvent
{
    Key key;
};

struct ControllerEvent
{
    class Controller* controller;
    i32 controllerID;
    f32 axisVal;
    ControllerAxis axis;
    ControllerButton button;
};

enum InputEventType
{
    INPUT_MOUSE_BUTTON_PRESSED,
    INPUT_MOUSE_BUTTON_DOWN,
    INPUT_MOUSE_BUTTON_RELEASED,
    INPUT_MOUSE_WHEEL,
    INPUT_MOUSE_MOVE,
    INPUT_KEYBOARD_PRESSED,
    INPUT_KEYBOARD_DOWN,
    INPUT_KEYBOARD_RELEASED,
    INPUT_KEYBOARD_REPEAT,
    INPUT_CONTROLLER_BUTTON_PRESSED,
    INPUT_CONTROLLER_BUTTON_DOWN,
    INPUT_CONTROLLER_BUTTON_RELEASED,
    INPUT_CONTROLLER_BUTTON_REPEAT,
    INPUT_CONTROLLER_AXIS_TRIGGERED,
    INPUT_CONTROLLER_AXIS,
    INPUT_CONTROLLER_AXIS_RELEASED,
    INPUT_CONTROLLER_AXIS_REPEAT,
};

struct InputEvent
{
    InputEventType type;
    union
    {
        MouseEvent mouse;
        MouseWheelEvent mouseWheel;
        KeyboardEvent keyboard;
        ControllerEvent controller;
    };
};

const f32 CONTROLLER_BEGIN_REPEAT_DELAY = 0.4f;
const f32 CONTROLLER_REPEAT_INTERVAL = 0.15f;

class Controller
{
    friend class Input;

private:
    Str64 guid;
    SDL_GameController* controller = nullptr;
    SDL_Haptic* haptic = nullptr;
    f32 axis[AXIS_COUNT] = {};
    bool axisTriggered[AXIS_COUNT] = {};;
    bool axisReleased[AXIS_COUNT] = {};;

    f32 repeatTimer = 0.f;
    // TODO: Store these as bitsets
    bool buttonDown[BUTTON_COUNT] = {};
    bool buttonPressed[BUTTON_COUNT] = {};
    bool buttonReleased[BUTTON_COUNT] = {};
    bool anyButtonPressed = false;

public:
    Controller(SDL_GameController* controller, SDL_Haptic* haptic, Str64 const& guid)
        : controller(controller), haptic(haptic), guid(guid) {}

    SDL_GameController* getController() const { return controller; }
    SDL_Haptic* getHaptic() const { return haptic; }

    void playHaptic(f32 strength = 0.5f, u32 lengthMs = 500)
    {
        if (haptic)
        {
            SDL_HapticRumblePlay(haptic, strength, lengthMs);
        }
    }

    bool isButtonDown(u32 button) const
    {
        assert(button < BUTTON_COUNT);
        return buttonDown[button];
    }

    bool isButtonPressed(u32 button) const
    {
        assert(button < BUTTON_COUNT);
        return buttonPressed[button];
    }

    bool isButtonReleased(u32 button) const
    {
        assert(button < BUTTON_COUNT);
        return buttonReleased[button];
    }

    bool isAnyButtonPressed() const { return anyButtonPressed; }

    f32 getAxis(u32 index) const
    {
        assert(index < AXIS_COUNT);
        return axis[index];
    }

    bool isAxisTriggered(u32 index) const
    {
        assert(index < AXIS_COUNT);
        return axisTriggered[index];
    }

    bool isAxisReleased(u32 index) const
    {
        assert(index < AXIS_COUNT);
        return axisReleased[index];
    }

    Str64 const& getGuid() const { return guid; }
};

#define NO_CONTROLLER UINT32_MAX

class Input
{
private:
    bool mouseButtonDown[MOUSE_BUTTON_COUNT];
    bool mouseButtonPressed[MOUSE_BUTTON_COUNT];
    bool mouseButtonReleased[MOUSE_BUTTON_COUNT];

    // TODO: Store these as bitsets
    bool keyDown[KEY_COUNT];
    bool keyPressed[KEY_COUNT];
    bool keyReleased[KEY_COUNT];
    bool keyRepeat[KEY_COUNT];

    SDL_Window* window;

    bool mouseMoved = false;
    i32 mouseScrollX;
    i32 mouseScrollY;
    // TODO: it would be faster and take less space to make this a list
    // there will never be more than a few controllers, so a linear search
    // would be faster than a map lookup
    Map<i32, Controller> controllers;

    //Str64 inputText;
    f32 joystickDeadzone = 0.08f;

    Array<InputEvent> events;

public:
    void init(SDL_Window* window)
    {
        // This is initialization is unnecessary because SDL creates events for each
        // controller that is already plugged in when the game starts.
        /*
        u32 numJoysticks = SDL_NumJoysticks();
        for (u32 i=0; i<numJoysticks; ++i)
        {
            if (SDL_IsGameController(i))
            {
                SDL_GameController* controller = SDL_GameControllerOpen(i);
                SDL_JoystickID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
                controllers.set(id, controller);
            }
        }
        */
        this->window = window;
    }

    void beginTextInput()
    {
        SDL_StartTextInput();
    }

    void stopTextInput()
    {
        SDL_StopTextInput();
    }

    Map<i32, Controller>& getControllers() { return controllers; }

    Controller* getController(i32 id)
    {
        return controllers.get(id);
    }

    i32 getControllerId(Str64 const& guid)
    {
        for (auto& ctl : controllers)
        {
            if (ctl.value.guid == guid)
            {
                return ctl.key;
            }
        }
        return NO_CONTROLLER;
    }

    bool isMouseButtonDown(u32 button)
    {
        assert(button < MOUSE_BUTTON_COUNT);
        return mouseButtonDown[button] || mouseButtonPressed[button];
    }

    bool isMouseButtonPressed(u32 button)
    {
        assert(button < MOUSE_BUTTON_COUNT);
        return mouseButtonPressed[button];
    }

    bool isMouseButtonReleased(u32 button)
    {
        assert(button < MOUSE_BUTTON_COUNT);
        return mouseButtonReleased[button];
    }

    bool isKeyDown(u32 key)
    {
        assert(key < KEY_COUNT);
        return keyDown[key] || keyPressed[key];
    }

    bool isKeyPressed(u32 key, bool allowRepeat=false)
    {
        assert(key < KEY_COUNT);
        return keyPressed[key] || (allowRepeat && keyRepeat[key]);
    }

    bool isKeyReleased(u32 key)
    {
        assert(key < KEY_COUNT);
        return keyReleased[key];
    }

    bool isKeyRepeated(u32 key)
    {
        assert(key < KEY_COUNT);
        return keyRepeat[key];
    }

    Vec2 getMousePosition()
    {
        i32 x, y;
        SDL_GetMouseState(&x, &y);
        return Vec2(x, y);
    }

    void setMousePosition(i32 x, i32 y)
    {
        SDL_WarpMouseInWindow(window, x, y);
    }

    bool didMouseMove() const
    {
        return mouseMoved;
    }

    f32 getMouseScroll()
    {
        return (f32)mouseScrollY;
    }

    Vec2 getMovement()
    {
        Vec2 result = Vec2(
                (f32)isKeyDown(KEY_RIGHT) - (f32)isKeyDown(KEY_LEFT),
                (f32)isKeyDown(KEY_UP) - (f32)isKeyDown(KEY_DOWN));
        for (auto& pair : getControllers())
        {
            Controller& controller = pair.value;
            result.x += (f32)controller.isButtonDown(BUTTON_DPAD_RIGHT) -
                        (f32)controller.isButtonDown(BUTTON_DPAD_LEFT);
            result.x += controller.getAxis(AXIS_LEFT_X);
            result.y += (f32)controller.isButtonDown(BUTTON_DPAD_UP) -
                        (f32)controller.isButtonDown(BUTTON_DPAD_DOWN);
            result.y += controller.getAxis(AXIS_LEFT_Y);
        }
        return result;
    }

    void onFrameEnd(f32 deltaTime)
    {
        memset(keyPressed, 0, sizeof(keyPressed));
        memset(keyReleased, 0, sizeof(keyReleased));
        memset(keyRepeat, 0, sizeof(keyRepeat));
        memset(mouseButtonPressed, 0, sizeof(mouseButtonPressed));
        memset(mouseButtonReleased, 0, sizeof(mouseButtonReleased));
        for (auto& controller : controllers)
        {
            memset(controller.value.buttonPressed, 0, sizeof(controller.value.buttonPressed));
            memset(controller.value.buttonReleased, 0, sizeof(controller.value.buttonReleased));
            memset(controller.value.axisTriggered, 0, sizeof(controller.value.axisTriggered));
            memset(controller.value.axisReleased, 0, sizeof(controller.value.axisReleased));
            controller.value.anyButtonPressed = false;
            controller.value.repeatTimer = max(controller.value.repeatTimer - deltaTime, 0.f);
        }
        mouseMoved = false;

        mouseScrollX = 0;
        mouseScrollY = 0;

        for (auto it = events.begin(); it != events.end();)
        {
            bool erase = false;
            switch (it->type)
            {
                case INPUT_MOUSE_MOVE:
                case INPUT_MOUSE_WHEEL:
                case INPUT_MOUSE_BUTTON_PRESSED:
                case INPUT_MOUSE_BUTTON_RELEASED:
                case INPUT_KEYBOARD_PRESSED:
                case INPUT_KEYBOARD_RELEASED:
                case INPUT_KEYBOARD_REPEAT:
                case INPUT_CONTROLLER_AXIS_RELEASED:
                case INPUT_CONTROLLER_AXIS_TRIGGERED:
                case INPUT_CONTROLLER_AXIS_REPEAT:
                case INPUT_CONTROLLER_BUTTON_PRESSED:
                case INPUT_CONTROLLER_BUTTON_REPEAT:
                case INPUT_CONTROLLER_BUTTON_RELEASED:
                    erase = true;
                    break;
                case INPUT_MOUSE_BUTTON_DOWN:
                    erase |= !mouseButtonDown[it->mouse.button];
                    break;
                case INPUT_KEYBOARD_DOWN:
                    erase |= !keyDown[it->keyboard.key];
                    break;
                case INPUT_CONTROLLER_BUTTON_DOWN:
                    erase |= !it->controller.controller->buttonDown[it->controller.button];
                    if (it->controller.controller->buttonDown[it->controller.button]
                            && it->controller.controller->repeatTimer == 0.f)
                    {
                        it->controller.controller->repeatTimer = CONTROLLER_REPEAT_INTERVAL;
                        InputEvent ev = *it;
                        ev.type = INPUT_CONTROLLER_BUTTON_REPEAT;
                        events.push(ev);
                    }
                    break;
                case INPUT_CONTROLLER_AXIS:
                    erase = true;
                    if (absolute(it->controller.controller->axis[it->controller.axis]) >= joystickDeadzone
                            && it->controller.controller->repeatTimer == 0.f)
                    {
                        it->controller.controller->repeatTimer = CONTROLLER_REPEAT_INTERVAL;
                        InputEvent ev = *it;
                        ev.type = INPUT_CONTROLLER_AXIS_REPEAT;
                        events.push(ev);
                    }
                    break;
            }
            if (erase)
            {
                it = events.erase(it);
            }
            else
            {
                ++it;
            }
        }

        assert(events.size() < 100);
    }

    //Str64 const& getInputText() const { return inputText; }

    Array<InputEvent> const& getEvents() const { return events; }

    void handleEvent(SDL_Event const& e)
    {
        switch(e.type)
        {
            case SDL_KEYDOWN:
            {
                u32 key = e.key.keysym.scancode;
                if (e.key.repeat == 0)
                {
                    keyDown[key] = true;
                    keyPressed[key] = true;

                    InputEvent ev{ INPUT_KEYBOARD_PRESSED };
                    ev.keyboard = { (Key)key };
                    events.push(ev);

                    ev.type = INPUT_KEYBOARD_DOWN;
                    events.push(ev);
                }
                else
                {
                    keyRepeat[key] = true;

                    InputEvent ev{ INPUT_KEYBOARD_REPEAT };
                    ev.keyboard = { (Key)key };
                    events.push(ev);
                }
            } break;
            case SDL_KEYUP:
            {
                u32 key = e.key.keysym.scancode;
                keyDown[key] = false;
                keyReleased[key] = true;

                InputEvent ev{ INPUT_KEYBOARD_RELEASED };
                ev.keyboard = { (Key)key };
                events.push(ev);
            } break;
            case SDL_MOUSEBUTTONDOWN:
            {
                u32 button = e.button.button;
                mouseButtonDown[button] = true;
                mouseButtonPressed[button] = true;

                InputEvent ev{ INPUT_MOUSE_BUTTON_PRESSED };
                ev.mouse = { Vec2((f32)e.button.x, (f32)e.button.y), {}, (MouseButton)button };
                events.push(ev);

                ev.type = INPUT_MOUSE_BUTTON_DOWN;
                events.push(ev);
            } break;
            case SDL_MOUSEBUTTONUP:
            {
                u32 button = e.button.button;
                mouseButtonDown[button] = false;
                mouseButtonReleased[button] = true;

                InputEvent ev{ INPUT_MOUSE_BUTTON_RELEASED };
                ev.mouse = { Vec2((f32)e.button.x, (f32)e.button.y), {}, (MouseButton)button };
                events.push(ev);
            } break;
            case SDL_MOUSEWHEEL:
            {
                i32 flip = e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1;
                mouseScrollX += e.wheel.x * flip;
                mouseScrollY += e.wheel.y * flip;

                InputEvent ev{ INPUT_MOUSE_WHEEL };
                ev.mouseWheel = { mouseScrollX, mouseScrollY };
                events.push(ev);
            } break;
            case SDL_MOUSEMOTION:
            {
                mouseMoved = true;

                InputEvent ev{ INPUT_MOUSE_MOVE };
                ev.mouse = {
                    Vec2((f32)e.motion.x, (f32)e.motion.y),
                    Vec2((f32)e.motion.xrel, (f32)e.motion.yrel),
                    MOUSE_UNKNOWN,
                };
                events.push(ev);
            } break;
            case SDL_CONTROLLERDEVICEADDED:
            {
                SDL_GameController* controller = SDL_GameControllerOpen(e.cdevice.which);
                SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller);
                SDL_JoystickID id = SDL_JoystickInstanceID(joystick);
                SDL_Haptic* haptic = SDL_HapticOpenFromJoystick(joystick);
                if (!haptic)
                {
                    println("Failed to initialize haptics: %s", SDL_GetError());
                }
                else if (SDL_HapticRumbleInit(haptic) != 0)
                {
                    println("Failed to initialize rumble: %s", SDL_GetError());
                    haptic = nullptr;
                }
                Str64 buffer;
                SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joystick), buffer.data(), sizeof(buffer));
                println("Controller added: %i, guid: %s, name: %s, haptics: %s", id, buffer.data(),
                        SDL_GameControllerName(controller), haptic ? "yes" : "no");
                controllers.set(id, Controller(controller, haptic, buffer));
            } break;
            case SDL_CONTROLLERDEVICEREMOVED:
            {
                controllers.erase(e.cdevice.which);
                println("Controller removed: %i", e.cdevice.which);
            } break;
            case SDL_CONTROLLERDEVICEREMAPPED:
            {
                println("Controller remapped: %i", e.cdevice.which);
            } break;
            case SDL_CONTROLLERAXISMOTION:
            {
                SDL_JoystickID which = e.caxis.which;
                u8 axis = e.caxis.axis;
                i16 value = e.caxis.value;
                auto ctl = controllers.get(which);
                if (ctl)
                {
                    f32 prevAl = ctl->axis[axis];
                    f32 val = value / 32768.f;
                    ctl->axis[axis] = absolute(val) < joystickDeadzone ? 0.f : val;

                    if (absolute(val) >= joystickDeadzone)
                    {
                        InputEvent ev{ INPUT_CONTROLLER_AXIS };
                        ev.controller = { ctl, which, ctl->axis[axis], (ControllerAxis)axis };
                        events.push(ev);

                        // TODO: fix repeat (this doesn't work because the axis event doesn't fire
                        // continuously, only on changes)
                        if (absolute(prevAl) < joystickDeadzone)
                        {
                            InputEvent ev{ INPUT_CONTROLLER_AXIS_TRIGGERED };
                            ev.controller = { ctl, which, ctl->axis[axis], (ControllerAxis)axis };
                            events.push(ev);

                            ctl->repeatTimer = CONTROLLER_BEGIN_REPEAT_DELAY;
                        }
                    }
                    else if (absolute(prevAl) >= joystickDeadzone)
                    {
                        InputEvent ev{ INPUT_CONTROLLER_AXIS_RELEASED };
                        ev.controller = { ctl, which, ctl->axis[axis], (ControllerAxis)axis };
                        events.push(ev);
                    }
                }
            } break;
            case SDL_CONTROLLERBUTTONDOWN:
            {
                SDL_JoystickID which = e.cbutton.which;
                u8 button = e.cbutton.button;
                auto ctl = controllers.get(which);
                if (ctl)
                {
                    ctl->buttonPressed[button] = true;
                    ctl->buttonDown[button] = true;
                    ctl->anyButtonPressed = true;
                    ctl->repeatTimer = CONTROLLER_BEGIN_REPEAT_DELAY;

                    InputEvent ev{ INPUT_CONTROLLER_BUTTON_PRESSED };
                    ev.controller = { ctl, which, 0.f, ControllerAxis(0), (ControllerButton)button };
                    events.push(ev);

                    ev.type = INPUT_CONTROLLER_BUTTON_DOWN;
                    events.push(ev);
                }
            } break;
            case SDL_CONTROLLERBUTTONUP:
            {
                SDL_JoystickID which = e.cbutton.which;
                u8 button = e.cbutton.button;
                auto ctl = controllers.get(which);
                if (ctl)
                {
                    ctl->buttonDown[button] = false;
                    ctl->buttonReleased[button] = true;
                }
                InputEvent ev{ INPUT_CONTROLLER_BUTTON_RELEASED };
                ev.controller = { ctl, which, 0.f, ControllerAxis(0), (ControllerButton)button };
                events.push(ev);
            } break;
            case SDL_TEXTINPUT:
            {
                //inputText += e.text.text;
            } break;
        }
    }
} g_input;
