#pragma once

#include "renderer.h"
#include "misc.h"
#include "scene.h"
#include "input.h"
#include "resources.h"
#include <memory>
#include <PxPhysicsAPI.h>

class PhysXErrorCallback : public PxErrorCallback
{
public:
    virtual void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line)
    {
        error(file, ": ", line, ": ", message, '\n');
    }
};

class Game
{
    void initPhysX();

public:
    Renderer renderer;
    Input input;
    Resources resources;

    struct
    {
        u32 resolutionX = 1280;
        u32 resolutionY = 720;
        bool fullscreen = false;
        u32 maxFPS = 200;
        bool vsync = true;
        f32 renderPercentage = 1.f;
    } config;

    struct
    {
        PhysXErrorCallback errorCallback;
        PxDefaultAllocator allocator;
        PxFoundation* foundation;
        PxPhysics* physics;
        PxDefaultCpuDispatcher* dispatcher;
        PxPvd* pvd;
        PxCooking* cooking;
    } physx;

    u32 windowWidth;
    u32 windowHeight;
    f32 deltaTime;

    SDL_Window* window = nullptr;
    std::unique_ptr<Scene> currentScene;

    void run();
    void changeScene(const char* sceneName);
} game;
