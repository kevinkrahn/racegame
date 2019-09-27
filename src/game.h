#pragma once

#include "misc.h"
#include "driver.h"
#include "menu.h"
#include "config.h"
#include "buffer.h"
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
        PxDefaultAllocator allocator;
        PxFoundation* foundation;
        PxPhysics* physics;
        PxDefaultCpuDispatcher* dispatcher;
        PxPvd* pvd;
        PxCooking* cooking;
    } physx;

    Config config;

    u32 windowWidth;
    u32 windowHeight;
    f32 realDeltaTime;
    f32 deltaTime;
    f64 currentTime = 0.0;
    f64 timeDilation = 1.0; // TODO: use this somewhere
    u64 frameCount = 0;
    u32 frameIndex = 0;
    bool shouldExit = false;
    bool isEditing = false;

    SDL_Window* window = nullptr;
    std::unique_ptr<class Scene> currentScene;
    std::unique_ptr<class Scene> nextScene;
    Menu menu;
    Buffer tempMem = Buffer(megabytes(4), 16);

    void run();
    Scene* changeScene(const char* sceneName);
} g_game;

template <typename... Args>
char* tstr(Args const&... args)
{
    std::string s = str(args...);
    u8* mem = g_game.tempMem.writeBytes((void*)s.data(), s.size() + 1);
    return reinterpret_cast<char*>(mem);
}
