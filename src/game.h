#pragma once

#include "misc.h"
#include "renderer.h"
#include "scene.h"
#include "input.h"
#include "resources.h"
#include "driver.h"
#include "audio.h"
#include <memory>

class Game
{
    void initPhysX();

public:
    Renderer renderer;
    Input input;
    Resources resources;
    Audio audio;

    struct
    {
        SmallVec<Driver> drivers;
    } state;

    struct
    {
        u32 resolutionX = 1280;
        u32 resolutionY = 720;
        bool fullscreen = false;
        u32 maxFPS = 200;
        bool vsync = true;
        u32 shadowMapResolution = 2048;
        bool shadowsEnabled = false;
        bool ssaoEnabled = false;
    } config;

    struct
    {
        PxDefaultAllocator allocator;
        PxFoundation* foundation;
        PxPhysics* physics;
        PxDefaultCpuDispatcher* dispatcher;
        PxPvd* pvd;
        PxCooking* cooking;
    } physx;

    u32 windowWidth;
    u32 windowHeight;
    f32 realDeltaTime;
    f32 deltaTime;
    f64 currentTime = 0.0;
    f64 timeDilation = 1.0;
    u32 frameIndex = 0;

    SDL_Window* window = nullptr;
    std::unique_ptr<Scene> currentScene;

    void run();
    void changeScene(const char* sceneName);
} game;
