#include "game.h"
#include "datafile.h"
#include <chrono>
#include <iostream>

void Game::run()
{
#ifndef NDEBUG
    print("Debug mode\n");
#endif

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        FATAL_ERROR("SDL_Init Error: ", SDL_GetError())
    }

    DataFile::Value val = DataFile::load("world.dat");
    print(val, '\n');

    window = renderer.initWindow("The Game", config.resolutionX, config.resolutionY);

    SDL_Event event;
    bool shouldExit = false;
    while(true)
    {
        auto frameStartTime = std::chrono::high_resolution_clock::now();
        input.onFrameBegin();

        while(SDL_PollEvent(&event) != 0)
        {
            input.handleEvent(event);
            if(event.type == SDL_QUIT)
            {
                shouldExit = true;
            }
        }

        if (input.isKeyPressed(KEY_F4))
        {
            config.fullscreen = !config.fullscreen;
            SDL_SetWindowFullscreen(window, config.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        }

        if(shouldExit || input.isKeyPressed(KEY_ESCAPE))
        {
            break;
        }

        renderer.render(deltaTime);
        input.onFrameEnd();

        using seconds = std::chrono::duration<f64, std::ratio<1>>;
        if (!game.config.vsync)
        {
            f64 minFrameTime = 1.0 / (f64)game.config.maxFPS;
            auto frameEndTime = frameStartTime + seconds(minFrameTime);
            while (std::chrono::high_resolution_clock::now() < frameEndTime)
            {
                _mm_pause();
            }
        }

        deltaTime = (f32)seconds(std::chrono::high_resolution_clock::now() - frameStartTime).count();
    }

    SDL_Quit();
}

void Game::changeScene(const char* sceneName)
{
    if (currentScene)
    {
        currentScene->onEnd();
    }
    currentScene.reset(new Scene());
}

/*
#include "PxPhysicsAPI.h"
using namespace physx;

class PhysXErrorCallback : public PxErrorCallback
{
public:
    virtual void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line)
    {
        std::cerr << file << ": " << line << ": " << message << '\n';
    }
};

PhysXErrorCallback gErrorCallback;
PxDefaultAllocator gAllocator;
PxFoundation* gFoundation = NULL;
PxPhysics* gPhysics = NULL;
PxDefaultCpuDispatcher* gDispatcher = NULL;
PxPvd* gPvd = NULL;
PxCooking* gCooking = NULL;

void initPhysX()
{
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

	gPvd = PxCreatePvd(*gFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
	gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);
	gCooking = PxCreateCooking(PX_PHYSICS_VERSION, *gFoundation, PxCookingParams(PxTolerancesScale()));

	const unsigned NUM_PHYSICS_THREADS = 1;
	gDispatcher = PxDefaultCpuDispatcherCreate(NUM_PHYSICS_THREADS);
}
*/
