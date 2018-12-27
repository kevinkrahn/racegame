#include "renderer.h"
#include "game.h"
#include <glad/glad.h>

struct GLMesh
{
    GLuint vao, vbo, ebo;
};

std::vector<GLMesh> meshes;

#ifndef NDEBUG
void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity,
                            GLsizei length, const GLchar* message, const void* userParam)
{
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204)
    {
        return;
    }
    print("OpenGL Debug (", id, "): ", message, '\n');
    //assert(severity != GL_DEBUG_SEVERITY_HIGH_ARB);
}
#endif

SDL_Window* Renderer::initWindow(const char* name, u32 width, u32 height)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#else
    //SDL_GL_SetAttribute(SDL_GL_CONTEXT_NO_ERROR);
#endif
    //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    //SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    SDL_Window* window = SDL_CreateWindow(name,
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if(!window)
    {
        FATAL_ERROR("Failed to create SDL window: ", SDL_GetError())
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if(!context)
    {
        FATAL_ERROR("Failed to create OpenGL context: ", SDL_GetError())
    }

    gladLoadGLLoader(SDL_GL_GetProcAddress);

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    glDebugMessageCallbackARB(glDebugOutput, nullptr);
    glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif

    SDL_GL_SetSwapInterval(game.config.vsync ? 1 : 0);

    //glEnable(GL_MULTISAMPLE);
    //glEnable(GL_CULL_FACE);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return window;
}

void Renderer::render(f32 deltaTime)
{
    // 3D
    glViewport(0, 0, game.windowWidth, game.windowHeight);
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

    SDL_GL_SwapWindow(game.window);
}

u32 Renderer::loadMesh(Mesh const& meshData)
{
    GLMesh mesh;

    glCreateBuffers(1, &mesh.vbo);
    glNamedBufferData(mesh.vbo, meshData.vertices.size() * meshData.stride, meshData.vertices.data(), GL_STATIC_DRAW);

    glCreateBuffers(1, &mesh.ebo);
    glNamedBufferData(mesh.ebo, meshData.indices.size() * sizeof(meshData.indices[0]), meshData.indices.data(), GL_STATIC_DRAW);

    enum
    {
        POSITION_BIND_INDEX,
        NORMAL_BIND_INDEX,
        COLOR_BIND_INDEX,
        TEXCOORD_BIND_INDEX
    };

    glCreateVertexArrays(1, &mesh.vao);
    glVertexArrayVertexBuffer(mesh.vao, 0, mesh.vbo, 0, meshData.stride);
    glVertexArrayElementBuffer(mesh.vao, mesh.ebo);

    glEnableVertexArrayAttrib(mesh.vao, POSITION_BIND_INDEX);
    glVertexArrayAttribFormat(mesh.vao, POSITION_BIND_INDEX, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(mesh.vao, POSITION_BIND_INDEX, 0);

    glEnableVertexArrayAttrib(mesh.vao, NORMAL_BIND_INDEX);
    glVertexArrayAttribFormat(mesh.vao, NORMAL_BIND_INDEX, 3, GL_FLOAT, GL_FALSE, 12);
    glVertexArrayAttribBinding(mesh.vao, NORMAL_BIND_INDEX, 0);

    glEnableVertexArrayAttrib(mesh.vao, COLOR_BIND_INDEX);
    glVertexArrayAttribFormat(mesh.vao, COLOR_BIND_INDEX, 3, GL_FLOAT, GL_FALSE, 24);
    glVertexArrayAttribBinding(mesh.vao, COLOR_BIND_INDEX, 0);

    glEnableVertexArrayAttrib(mesh.vao, TEXCOORD_BIND_INDEX);
    glVertexArrayAttribFormat(mesh.vao, TEXCOORD_BIND_INDEX, 2, GL_FLOAT, GL_FALSE, 32);
    glVertexArrayAttribBinding(mesh.vao, TEXCOORD_BIND_INDEX, 0);

    meshes.push_back(mesh);
    return (u32)(meshes.size() - 1);
}
