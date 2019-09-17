#pragma once

#include "misc.h"
#include "driver.h"
#include "menu.h"
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
        u32 msaaLevel = 0;
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
    f64 timeDilation = 1.0; // TODO: use this somewhere
    u32 frameIndex = 0;
    bool shouldExit = false;
    bool isEditing = false;

    SDL_Window* window = nullptr;
    std::unique_ptr<class Scene> currentScene;
    std::unique_ptr<class Scene> nextScene;
    Menu menu;

    void run();
    Scene* changeScene(const char* sceneName);
} g_game;
