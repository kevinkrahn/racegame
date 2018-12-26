#pragma once

#include <SDL2/SDL.h>
#include <glm/vec2.hpp>
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

class Input
{
private:
    bool mouseButtonDown[MOUSE_BUTTON_COUNT];
    bool mouseButtonPressed[MOUSE_BUTTON_COUNT];
    bool mouseButtonReleased[MOUSE_BUTTON_COUNT];

    bool keyDown[KEY_COUNT];
    bool keyPressed[KEY_COUNT];
    bool keyReleased[KEY_COUNT];

    i32 mouseScrollX;
    i32 mouseScrollY;

    void pressMouseButton(u32 button)
    {
        assert(button < MOUSE_BUTTON_COUNT);
        mouseButtonDown[button] = true;
        mouseButtonPressed[button] = true;
    }

    void releaseMouseButton(u32 button)
    {
        assert(button < MOUSE_BUTTON_COUNT);
        mouseButtonDown[button] = false;
        mouseButtonReleased[button] = true;
    }

    void pressKey(u32 key)
    {
        assert(key < KEY_COUNT);
        keyDown[key] = true;
        keyPressed[key] = true;
    }

    void releaseKey(u32 key)
    {
        assert(key < KEY_COUNT);
        keyDown[key] = false;
        keyReleased[key] = true;
    }

public:
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

    bool isKeyPressed(u32 key)
    {
        assert(key < KEY_COUNT);
        return keyPressed[key];
    }

    bool isKeyReleased(u32 key)
    {
        assert(key < KEY_COUNT);
        return keyReleased[key];
    }

    glm::vec2 getMousePosition()
    {
        i32 x, y;
        SDL_GetMouseState(&x, &y);
        return glm::vec2(x, y);
    }

    void onFrameBegin()
    {
    }

    void onFrameEnd()
    {
        memset(keyPressed, 0, sizeof(keyPressed));
        memset(keyReleased, 0, sizeof(keyReleased));
        memset(mouseButtonPressed, 0, sizeof(mouseButtonPressed));
        memset(mouseButtonReleased, 0, sizeof(mouseButtonReleased));

        mouseScrollX = 0;
        mouseScrollY = 0;
    }

    void handleEvent(SDL_Event const& e)
    {
        switch(e.type)
        {
            case SDL_KEYDOWN:
            {
                if (e.key.repeat == 0)
                {
                    pressKey(e.key.keysym.scancode);
                }
            } break;
            case SDL_KEYUP:
            {
                releaseKey(e.key.keysym.scancode);
            } break;
            case SDL_MOUSEBUTTONDOWN:
            {
                pressMouseButton(e.button.button);
            } break;
            case SDL_MOUSEBUTTONUP:
            {
                releaseMouseButton(e.button.button);
            } break;
            case SDL_MOUSEWHEEL:
            {
                i32 flip = e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1;
                mouseScrollX += e.wheel.x * flip;
                mouseScrollY += e.wheel.y * flip;
            } break;
        }
    }
};
