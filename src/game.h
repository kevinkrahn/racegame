#pragma once

#include "renderer.h"
#include "misc.h"
#include "scene.h"
#include "input.h"
#include "resources.h"
#include <memory>

class Game
{
public:
    Renderer renderer;
    Input input;
    Resources resources;

    struct Config
    {
        u32 resolutionX = 1280;
        u32 resolutionY = 720;
        bool fullscreen = false;
        u32 maxFPS = 200;
        bool vsync = true;
        f32 renderPercentage = 1.f;
    } config;

    u32 windowWidth;
    u32 windowHeight;
    f32 deltaTime = 0.f;

    SDL_Window* window = nullptr;
    std::unique_ptr<Scene> currentScene;

    void run();
    void changeScene(const char* sceneName);
} game;
