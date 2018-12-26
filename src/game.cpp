#include "game.h"
#include <iostream>

void Game::run()
{
#ifndef NDEBUG
    std::cout << "Debug mode" << std::endl;
#endif

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        FATAL_ERROR("SDL_Init Error: ", SDL_GetError())
    }

    window = renderer.initWindow("The Game", config.resolutionX, config.resolutionY);
    //SDL_ShowCursor(SDL_DISABLE);

    SDL_Event event;
    bool shouldExit = false;
    bool fullscreen = false;
    while(true)
    {
        while(SDL_PollEvent(&event) != 0)
        {
            if(event.type == SDL_QUIT)
            {
                shouldExit = true;
            }
            else if(event.type == SDL_KEYDOWN && !event.key.repeat)
            {
                if(event.key.keysym.sym == SDLK_ESCAPE)
                {
                    shouldExit = true;
                }
                else if(event.key.keysym.sym == SDLK_F4)
                {
                    fullscreen = !fullscreen;
                    SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                }
            }
        }

        if(shouldExit)
        {
            break;
        }

        renderer.render(0.f);

        //auto deltaTime = seconds(std::chrono::high_resolution_clock::now() - start_time).count();
    }

    SDL_Quit();
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
