#include "game.h"
#include "datafile.h"
#include "vehicle_data.h"
#include "renderer.h"
#include "scene.h"
#include "input.h"
#include "resources.h"
#include "audio.h"
#include "weapon.h"
#include "imgui.h"
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl3.h>

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
    cookingParams.midphaseDesc = PxMeshMidPhase::eBVH34;
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

    physx.materials.vehicle = physx.physics->createMaterial(0.1f, 0.1f, 0.38f);
    physx.materials.track   = physx.physics->createMaterial(0.3f, 0.3f, 0.1f);
    physx.materials.offroad = physx.physics->createMaterial(0.4f, 0.4f, 0.05f);
    physx.materials.generic = physx.physics->createMaterial(0.4f, 0.4f, 0.05f);
    physx.materials.railing = physx.physics->createMaterial(0.05f, 0.05f, 0.3f);
}

#ifndef NDEBUG
static void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity,
                            GLsizei length, const GLchar* message, const void* userParam)
{
    // Useless nvidia warnings
    if (id == 1 || id == 131169 || id == 131185 || id == 131218 || id == 131204)
    {
        return;
    }
    // Useless intel warning
    if (id == 8)
    {
        return;
    }
    println("OpenGL Debug (%i): %s", id, message);
    assert(severity != GL_DEBUG_SEVERITY_HIGH_ARB);
}
#endif

void Game::run()
{
#ifndef NDEBUG
    println("Debug mode");
#endif

    f64 loadStartTime = getTime();

    g_game.config.load();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) != 0)
    {
        FATAL_ERROR("SDL_Init Error: %s", SDL_GetError())
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

    u32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
    if (config.graphics.fullscreen)
    {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
    window = SDL_CreateWindow("The Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            config.graphics.resolutionX, config.graphics.resolutionY, flags);
    if (!window)
    {
        FATAL_ERROR("Failed to create SDL window: %s", SDL_GetError())
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context)
    {
        FATAL_ERROR("Failed to create OpenGL context: %s", SDL_GetError())
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
    renderer->init();

    // init Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 130");
    io.Fonts->AddFontFromFileTTF("font.ttf", 16.f);
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize = 1.f;
    style.FrameBorderSize = 0.f;
    style.PopupBorderSize = 1.f;
    style.WindowRounding = 2.f;
    style.FrameRounding = 2.f;
    style.TabRounding = 6.f;
    style.Colors[2] = { 0.02f, 0.02f, 0.02f, 0.92f };

    g_input.init(window);
    g_audio.init();
    initPhysX();
    g_res.load();
    initializeVehicleData();
    registerEntities();

    println("Loaded resources in %.2f seconds", getTime() - loadStartTime);

    changeScene("race1");
    menu.showMainMenu();

    deltaTime = 1.f / (f32)config.graphics.maxFPS;
    SDL_Event event;

    g_tmpMem.clear();
    while (true)
    {
        f64 frameStartTime = getTime();
        g_input.onFrameBegin();

        while (SDL_PollEvent(&event) != 0)
        {
            g_input.handleEvent(event);
            ImGui_ImplSDL2_ProcessEvent(&event);
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

        if (shouldUnloadScene)
        {
            if (currentScene)
            {
                currentScene->onEnd();
                currentScene.reset(nullptr);
            }
            shouldUnloadScene = false;
        }
        if (nextScene)
        {
            if (currentScene)
            {
                currentScene->onEnd();
            }
            currentScene = std::move(nextScene);
            currentScene->onStart();
            //menu.showRaceResults();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        if (isEditing)
        {
            resourceManager->onUpdate(renderer.get(), deltaTime);
        }
        if (currentScene)
        {
            currentScene->onUpdate(renderer.get(), deltaTime);
        }
        menu.onUpdate(renderer.get(), deltaTime);
        checkDebugKeys();
        ImGui::Render();
        renderer->render(deltaTime);
        if (currentScene)
        {
            currentScene->onEndUpdate();
        }
        g_input.onFrameEnd();

        frameIndex = (frameIndex + 1) % MAX_BUFFERED_FRAMES;
        ++frameCount;

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        cpuTime = getTime() - frameStartTime;
        if (!isTimedBlockTrackingPaused)
        {
            previousFrameTimedBlocks = std::move(timedBlocks);
            previousCpuTime = cpuTime;
        }
        timedBlocks.clear();

        SDL_GL_SwapWindow(g_game.window);

        if (!config.graphics.vsync)
        {
            f64 minFrameTime = 1.0 / (f64)config.graphics.maxFPS;
            auto frameEndTime = frameStartTime + minFrameTime;
            while (getTime() < frameEndTime)
            {
                _mm_pause();
            }
        }

        g_tmpMem.clear();

        const f64 maxDeltaTime = 1.f / 30.f;
        f64 delta = min(getTime() - frameStartTime, maxDeltaTime);
        realDeltaTime = (f32)delta;
        deltaTime = (f32)(delta * timeDilation);
        currentTime += delta * timeDilation;

        if (realDeltaTime > allTimeHighestDeltaTime)
        {
            allTimeHighestDeltaTime = realDeltaTime;
        }
        if (realDeltaTime < allTimeLowestDeltaTime)
        {
            allTimeLowestDeltaTime = realDeltaTime;
        }
        recentHighestDeltaTime = FLT_MIN;
        averageDeltaTime = 0.f;
        for (u32 i=0; i<ARRAY_SIZE(deltaTimeHistory)-1; ++i)
        {
            deltaTimeHistory[i] = deltaTimeHistory[i+1];
            averageDeltaTime += deltaTimeHistory[i];
            if (deltaTimeHistory[i] > recentHighestDeltaTime)
            {
                recentHighestDeltaTime = deltaTimeHistory[i];
            }
        }
        deltaTimeHistory[ARRAY_SIZE(deltaTimeHistory)-1] = realDeltaTime;
        averageDeltaTime += realDeltaTime;
        averageDeltaTime /= ARRAY_SIZE(deltaTimeHistory);
    }

    g_audio.close();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

Scene* Game::changeScene(const char* sceneName)
{
    if (sceneName)
    {
        println("Loading scene: %s", sceneName);
        i64 guid = g_res.getTrackGuid(sceneName);
        return changeScene(guid);
    }
    else
    {
        i64 emptyGuid = 0;
        return changeScene(emptyGuid);
    }
}

Scene* Game::changeScene(i64 guid)
{
    if (guid != 0)
    {
        auto trackData = g_res.getTrackData(guid);
        Scene* scene = new Scene(trackData);
        nextScene.reset(scene);
        return scene;
    }
    else
    {
        Scene* scene = new Scene(nullptr);
        nextScene.reset(scene);
        return scene;
    }
}

void Game::saveGame()
{
    DataFile::Value data = DataFile::makeDict();
    Serializer s(data, false);
    state.serialize(s);
    DataFile::save(data, "savedgame.sav");
}

void Game::loadGame()
{
    DataFile::Value data = DataFile::load("savedgame.sav");
    Serializer s(data, true);
    state.serialize(s);
}

void Game::checkDebugKeys()
{
    if (g_input.isKeyPressed(KEY_F1))
    {
        isDebugOverlayEnabled = !isDebugOverlayEnabled;
    }

    if (g_input.isKeyPressed(KEY_F2))
    {
        isPhysicsDebugVisualizationEnabled = !isPhysicsDebugVisualizationEnabled;
    }

    if (g_input.isKeyPressed(KEY_F3))
    {
        isTrackGraphDebugVisualizationEnabled = !isTrackGraphDebugVisualizationEnabled;
    }

    if (g_input.isKeyPressed(KEY_F6))
    {
        isMotionGridDebugVisualizationEnabled = !isMotionGridDebugVisualizationEnabled;
    }

    if (g_input.isKeyPressed(KEY_F7))
    {
        isPathVisualizationEnabled = !isPathVisualizationEnabled;
    }

    if (g_input.isKeyPressed(KEY_F8))
    {
        isDebugCameraEnabled = !isDebugCameraEnabled;
    }

    if (g_input.isKeyPressed(KEY_F9))
    {
        renderer->reloadShaders();
    }

    if (isDebugOverlayEnabled)
    {
        if (isImGuiMetricsWindowOpen)
        {
            ImGui::ShowMetricsWindow(&isImGuiMetricsWindowOpen);
        }
        if (isImGuiDemoWindowOpen)
        {
            ImGui::ShowDemoWindow(&isImGuiDemoWindowOpen);
        }

        ImGui::Begin("Debug", &isDebugOverlayEnabled);

        if (ImGui::Button("ImGui Metrics"))
        {
            isImGuiMetricsWindowOpen = !isImGuiMetricsWindowOpen;
        }
        ImGui::SameLine();
        if (ImGui::Button("ImGui Demo"))
        {
            isImGuiDemoWindowOpen = !isImGuiDemoWindowOpen;
        }

        ImGui::Text("FPS: %.3f", 1.f / g_game.realDeltaTime);
        ImGui::Text("Average FPS: %.3fms", 1.f / g_game.averageDeltaTime);
        ImGui::Text("Frame Time: %.3fms", g_game.realDeltaTime * 1000);
        ImGui::Text("CPU Time: %.3fms", g_game.cpuTime * 1000);
        ImGui::Text("Swap Wait Time: %.3fms", (g_game.realDeltaTime - g_game.cpuTime) * 1000);
        ImGui::Text("Average Frame Time: %.3fms", g_game.averageDeltaTime * 1000);
        ImGui::Text("Highest Frame Time: %.3fms", g_game.allTimeHighestDeltaTime * 1000);
        ImGui::Text("Lowest Frame Time: %.3fms", g_game.allTimeLowestDeltaTime * 1000);
        ImGui::PlotLines("Frame Times", g_game.deltaTimeHistory, ARRAY_SIZE(g_game.deltaTimeHistory),
                0, nullptr, 0.f, 0.04f, { 0, 80 });
        ImGui::Text("Frame Temp-Memory Usage: %.3fkb", g_tmpMem.pos / 1024.f);
        ImGui::Text("Resolution: %ix%i", g_game.config.graphics.resolutionX, g_game.config.graphics.resolutionY);
        ImGui::Text("Time Dilation: %f", g_game.timeDilation);
        // TODO: count draw calls
        //ImGui::Text("Renderables: %i", renderer->getRenderablesCount());

        ImGui::Gap();

        ImGui::Checkbox("Timing Collection Paused", &isTimedBlockTrackingPaused);
        for (auto& pair : previousFrameTimedBlocks)
        {
            ImGui::Text("%5.2f %s", (pair.value / previousCpuTime) * 100.f, pair.key);
        }

        ImGui::Gap();

        ImGui::Checkbox("Debug Camera", &isDebugCameraEnabled);
        ImGui::Checkbox("Physics Visualization", &isPhysicsDebugVisualizationEnabled);
        ImGui::Checkbox("Track Graph Visualization", &isTrackGraphDebugVisualizationEnabled);
        ImGui::Checkbox("Motion Grid Visualization", &isMotionGridDebugVisualizationEnabled);
        ImGui::Checkbox("Path Visualization", &isPathVisualizationEnabled);

        if (currentScene)
        {
            currentScene->showDebugInfo();
        }

        ImGui::End();
    }
}
