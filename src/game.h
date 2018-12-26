#pragma once

#include "renderer.h"
#include "misc.h"

class Game
{
public:
    Renderer renderer;

    struct Config
    {
        u32 resolutionX = 1280;
        u32 resolutionY = 720;
        bool fullscreen = false;
        u32 maxFPS = 200;
        bool vsync = true;
        f32 renderPercentage = 1.f;
    } config;

    u32 windowWidth = 0;
    u32 windowHeight = 0;

    SDL_Window* window = nullptr;

    void run();
} game;

#define FATAL_ERROR(...)
