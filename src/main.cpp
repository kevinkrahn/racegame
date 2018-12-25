#include <glad/glad.h>
#include <SDL2/SDL.h>

#include <iostream>

#include "PxPhysicsAPI.h"

#ifndef NDEBUG
void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity,
                            GLsizei length, const GLchar* message, const void* userParam)
{
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204)
    {
        return;
    }
    std::cout << "OpenGL Debug (" << id << "): " <<  message << '\n';
    //assert(severity != GL_DEBUG_SEVERITY_HIGH_ARB);
}
#endif


int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return EXIT_FAILURE;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    std::cout << "Debug mode" << std::endl;
#endif
    //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    int width = 1280;
    int height = 720;
    SDL_Window* window = SDL_CreateWindow("The Game",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            width, height,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if(!window)
    {
        std::cout << "Could not create SDL window: " << SDL_GetError() << std::endl;
        return EXIT_FAILURE;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if(!context)
    {
        std::cout << "Could not create GL context: " << SDL_GetError() << std::endl;
        return EXIT_FAILURE;
    }

    gladLoadGLLoader(SDL_GL_GetProcAddress);

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    glDebugMessageCallbackARB(glDebugOutput, nullptr);
    glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif

    //SDL_ShowCursor(SDL_DISABLE);
    bool vsync = true;
    SDL_GL_SetSwapInterval(vsync ? 1 : 0);

    //glEnable(GL_MULTISAMPLE);
    //glEnable(GL_CULL_FACE);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

        // 3D
        glViewport(0, 0, width, height);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0, 0, 0, 0);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        // 2D
        /*
        glViewport(0, 0, window_width, window_height);
        glDepthMask(GL_FALSE);
        glDisable(GL_DEPTH_TEST);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        */

        SDL_GL_SwapWindow(window);

        //auto deltaTime = seconds(std::chrono::high_resolution_clock::now() - start_time).count();
    }

    SDL_Quit();

    return EXIT_SUCCESS;
}

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
