#pragma once

#include "misc.h"
#include "driver.h"
#include <memory>

class Game
{
    void initPhysX();

public:
    std::unique_ptr<class Renderer> renderer;

    struct
    {
        std::vector<Driver> drivers;
    } state;

    struct
    {
        u32 resolutionX = 1280;
        u32 resolutionY = 720;
        bool fullscreen = false;
        u32 maxFPS = 200;
        bool vsync = true;
        u32 shadowMapResolution = 2048;
        bool shadowsEnabled = true;
        bool ssaoEnabled = true;
        bool ssaoHighQuality = false; // TODO: Implement
        u32 msaaLevel = 2;
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
    std::unique_ptr<class Scene> currentScene;
    std::unique_ptr<class Scene> nextScene;

    void run();
    void changeScene(const char* sceneName);
} g_game;
