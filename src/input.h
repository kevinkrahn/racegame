#pragma once

#include <SDL2/SDL.h>
#include "misc.h"

// These values line up with SDL_Button* defines
enum MouseButton
{
    MOUSE_LEFT = 1,
    MOUSE_MIDDLE,
    MOUSE_RIGHT,
    MOUSE_X1,
    MOUSE_X2,
    MOUSE_BUTTON_COUNT
};

enum Keys
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

class Controller
{
    friend class Input;

private:
    SDL_GameController* controller = nullptr;
    bool buttonDown[BUTTON_COUNT] = {};
    bool buttonPressed[BUTTON_COUNT] = {};
    bool buttonReleased[BUTTON_COUNT] = {};
    f32 axis[AXIS_COUNT] = {};
    bool anyButtonPressed = false;
    Str64 guid;

public:
    explicit Controller(SDL_GameController* controller, Str64 const& guid)
        : controller(controller), guid(guid) {}

    SDL_GameController* getController() const { return controller; }

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

    Str64 const& getGuid() const { return guid; }
};

#define NO_CONTROLLER UINT32_MAX

class Input
{
private:
    bool mouseButtonDown[MOUSE_BUTTON_COUNT];
    bool mouseButtonPressed[MOUSE_BUTTON_COUNT];
    bool mouseButtonReleased[MOUSE_BUTTON_COUNT];

    bool keyDown[KEY_COUNT];
    bool keyPressed[KEY_COUNT];
    bool keyReleased[KEY_COUNT];
    bool keyRepeat[KEY_COUNT];

    SDL_Window* window;

    bool mouseMoved = false;
    i32 mouseScrollX;
    i32 mouseScrollY;
    Map<i32, Controller> controllers;

    //Str64 inputText;
    f32 joystickDeadzone = 0.08f;

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

    Map<i32, Controller> const& getControllers() const { return controllers; }

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

    void onFrameBegin()
    {
        //inputText.clear();
    }

    void onFrameEnd()
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
            controller.value.anyButtonPressed = false;
        }
        mouseMoved = false;

        mouseScrollX = 0;
        mouseScrollY = 0;
    }

    //Str64 const& getInputText() const { return inputText; }

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
                }
                else
                {
                    keyRepeat[key] = true;
                }
            } break;
            case SDL_KEYUP:
            {
                u32 key = e.key.keysym.scancode;
                keyDown[key] = false;
                keyReleased[key] = true;
            } break;
            case SDL_MOUSEBUTTONDOWN:
            {
                u32 button = e.button.button;
                mouseButtonDown[button] = true;
                mouseButtonPressed[button] = true;
            } break;
            case SDL_MOUSEBUTTONUP:
            {
                u32 button = e.button.button;
                mouseButtonDown[button] = false;
                mouseButtonReleased[button] = true;
            } break;
            case SDL_MOUSEWHEEL:
            {
                i32 flip = e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1;
                mouseScrollX += e.wheel.x * flip;
                mouseScrollY += e.wheel.y * flip;
            } break;
            case SDL_MOUSEMOTION:
            {
                mouseMoved = true;
            } break;
            case SDL_CONTROLLERDEVICEADDED:
            {
                SDL_GameController* controller = SDL_GameControllerOpen(e.cdevice.which);
                SDL_JoystickID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
                Str64 buffer;
                SDL_JoystickGetGUIDString(
                        SDL_JoystickGetGUID(
                            SDL_GameControllerGetJoystick(controller)), buffer.data(), sizeof(buffer));
                println("Controller added: %i, guid: %s, name: %s", id, buffer.data(),
                        SDL_GameControllerName(controller));
                controllers.set(id, Controller(controller, buffer));
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
                    f32 val = value / 32768.f;
                    ctl->axis[axis] = absolute(val) < joystickDeadzone ? 0.f : val;
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
            } break;
            case SDL_TEXTINPUT:
            {
                //inputText += e.text.text;
            } break;
        }
    }
} g_input;
