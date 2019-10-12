#include "game.h"
#include "datafile.h"
#include "vehicle_data.h"
#include "renderer.h"
#include "scene.h"
#include "input.h"
#include "resources.h"
#include "audio.h"
#include "gui.h"
#include "weapon.h"
#include <chrono>
#include <iostream>

void Game::initPhysX()
{
    static class : public PxErrorCallback
    {
    public:
        virtual void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line)
        {
            error(file, ": ", line, ": ", message, '\n');
        }
    } errorCallback;

    physx.foundation = PxCreateFoundation(PX_PHYSICS_VERSION, physx.allocator, errorCallback);
    physx.pvd = PxCreatePvd(*physx.foundation);
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
    physx.pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

    physx.physics = PxCreatePhysics(PX_PHYSICS_VERSION, *physx.foundation, PxTolerancesScale(), true, physx.pvd);

    PxCookingParams cookingParams = PxCookingParams(PxTolerancesScale());
    // TODO: investigate the ideal cooking params (load time vs runtime performance)
    /*
    cookingParams.buildGPUData = false;
    cookingParams.buildTriangleAdjacencies = false;
    cookingParams.meshPreprocessParams = PxMeshPreprocessingFlags(
        PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH |
        PxMeshPreprocessingFlag::eFORCE_32BIT_INDICES |
        PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE);
    cookingParams.suppressTriangleMeshRemapTable = true;
    */
    physx.cooking = PxCreateCooking(PX_PHYSICS_VERSION, *physx.foundation, cookingParams);

    const u32 PHYSICS_THREADS = 0;
    physx.dispatcher = PxDefaultCpuDispatcherCreate(PHYSICS_THREADS);

    PxInitVehicleSDK(*physx.physics);
    PxVehicleSetBasisVectors(PxVec3(0, 0, 1), PxVec3(1, 0, 0));
    PxVehicleSetUpdateMode(PxVehicleUpdateMode::eVELOCITY_CHANGE);
    const f32 POINT_REJECT_ANGLE = PxPi / 4.0f;
    const f32 NORMAL_REJECT_ANGLE = PxPi / 4.0f;
    PxVehicleSetSweepHitRejectionAngles(POINT_REJECT_ANGLE, NORMAL_REJECT_ANGLE);
    PxVehicleSetMaxHitActorAcceleration(80.f);
}

#ifndef NDEBUG
static void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity,
                            GLsizei length, const GLchar* message, const void* userParam)
{
    if (id == 1 || id == 131169 || id == 131185 || id == 131218 || id == 131204)
    {
        return;
    }
    print("OpenGL Debug (", id, "): ", message, '\n');
    //assert(severity != GL_DEBUG_SEVERITY_HIGH_ARB);
}
#endif

void Game::run()
{
#ifndef NDEBUG
    print("Debug mode\n");
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) != 0)
    {
        FATAL_ERROR("SDL_Init Error: ", SDL_GetError())
    }

    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#else
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_NO_ERROR);
#endif

    window = SDL_CreateWindow("The Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            config.graphics.resolutionX, config.graphics.resolutionY, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window)
    {
        FATAL_ERROR("Failed to create SDL window: ", SDL_GetError())
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context)
    {
        FATAL_ERROR("Failed to create OpenGL context: ", SDL_GetError())
    }

    gladLoadGLLoader(SDL_GL_GetProcAddress);

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    glDebugMessageCallbackARB(glDebugOutput, nullptr);
    glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif

    SDL_GL_SetSwapInterval(g_game.config.graphics.vsync ? 1 : 0);

    i32 w, h;
    SDL_GL_GetDrawableSize(window, &w, &h);
    windowWidth = w;
    windowHeight = h;

    renderer.reset(new Renderer());
    renderer->init(config.graphics.resolutionX, config.graphics.resolutionY);

    g_input.init(window);
    g_audio.init();
    initPhysX();
    g_resources.load();
    initializeWeapons();

    changeScene("tracks/saved_scene.dat");
    //changeScene(nullptr);

    deltaTime = 1.f / (f32)config.graphics.maxFPS;
    SDL_Event event;
    while (true)
    {
        auto frameStartTime = std::chrono::high_resolution_clock::now();
        g_input.onFrameBegin();

        while (SDL_PollEvent(&event) != 0)
        {
            g_input.handleEvent(event);
            if (event.type == SDL_QUIT)
            {
                shouldExit = true;
            }
        }

        if (g_input.isKeyPressed(KEY_F4))
        {
            config.graphics.fullscreen = !config.graphics.fullscreen;
            SDL_SetWindowFullscreen(window, config.graphics.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        }

        if (shouldExit)
        {
            break;
        }

        i32 w, h;
        SDL_GL_GetDrawableSize(window, &w, &h);
        if (w != (i32)windowWidth || h != (i32)windowHeight)
        {
            windowWidth = w;
            windowHeight = h;
            renderer->updateFullscreenFramebuffers();
        }

        if (nextScene)
        {
            if (currentScene)
            {
                currentScene->onEnd();
            }
            currentScene = std::move(nextScene);
            currentScene->onStart();
        }

        g_gui.beginFrame();
        currentScene->onUpdate(renderer.get(), deltaTime);
        menu.onUpdate(renderer.get(), deltaTime);
        g_gui.endFrame();
        renderer->render(deltaTime);
        g_input.onFrameEnd();

        frameIndex = (frameIndex + 1) % MAX_BUFFERED_FRAMES;
        ++frameCount;

        using seconds = std::chrono::duration<f64, std::ratio<1>>;
        if (!config.graphics.vsync)
        {
            f64 minFrameTime = 1.0 / (f64)config.graphics.maxFPS;
            auto frameEndTime = frameStartTime + seconds(minFrameTime);
            while (std::chrono::high_resolution_clock::now() < frameEndTime)
            {
                _mm_pause();
            }
        }

        tempMem.clear();

        const f64 maxDeltaTime = 1.f / 30.f;
        f64 delta = glm::min(seconds(std::chrono::high_resolution_clock::now() -
                    frameStartTime).count(), maxDeltaTime);
        realDeltaTime = (f32)delta;
        deltaTime = (f32)(delta * timeDilation);
        currentTime += delta * timeDilation;
    }

    g_audio.close();
    SDL_Quit();
}

Scene* Game::changeScene(const char* sceneName)
{
    if (sceneName)
    {
        print("Loading scene: ", sceneName, '\n');
    }
    Scene* scene = new Scene(sceneName);
    nextScene.reset(scene);
    return scene;
}
