#include "renderer.h"
#include "game.h"
#include <glad/glad.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <fstream>
#include <sstream>

struct GLMesh
{
    GLuint vao, vbo, ebo;
    u32 numIndices;
};

struct GLShader
{
    GLuint program;
};

struct GLTexture
{
    GLuint tex;
};

struct RenderMesh
{
    u32 meshHandle;
    glm::mat4 worldTransform;
};

struct WorldInfo
{
    glm::vec3 sunDirection;
    f32 time;
    glm::vec3 sunColor;
} worldInfo;

std::vector<GLMesh> loadedMeshes;
std::vector<GLTexture> loadedTextures;
std::vector<Camera> cameras;
std::vector<RenderMesh> renderList;

glm::vec3 backgroundColor;

GLShader shader;
GLuint worldInfoUBO;

void glShaderSources(GLuint shader, std::string const& src, std::initializer_list<std::string_view> defines)
{
    std::ostringstream str;
    str << "#version 450\n";
    for (auto const& d : defines)
    {
        str << "#define " << d << '\n';
    }
    std::string tmp = str.str();
    const char* sources[] = { tmp.c_str(), src.c_str() };
    glShaderSource(shader, 2, sources, 0);
}

GLShader loadShader(const char* filename, bool useGeometryShader=false)
{
    std::ifstream file(filename);
    if (!file)
    {
        FATAL_ERROR("Cannot load shader file: ", filename);
    }

    std::stringstream stream;
    stream << file.rdbuf();
    file.close();
    std::string shaderStr = stream.str();

    GLint success, errorMessageLength;
    GLuint program = glCreateProgram();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSources(vertexShader, shaderStr, { "VERT" });
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &errorMessageLength);
        std::string errorMessage(errorMessageLength, ' ');
        glGetShaderInfoLog(vertexShader, errorMessageLength, 0, errorMessage.data());
        error("Vertex Shader Compilation Error: (", filename, ")\n", errorMessage, '\n');
    }
    glAttachShader(program, vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSources(fragmentShader, shaderStr, { "FRAG" });
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &errorMessageLength);
        std::string errorMessage(errorMessageLength, ' ');
        glGetShaderInfoLog(fragmentShader, errorMessageLength, 0, errorMessage.data());
        error("Fragment Shader Compilation Error: (", filename, ")\n", errorMessage, '\n');
    }
    glAttachShader(program, fragmentShader);

    GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
    if (useGeometryShader)
    {
        glShaderSources(geometryShader, shaderStr, { "GEOM" });
        glCompileShader(geometryShader);
        glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderiv(geometryShader, GL_INFO_LOG_LENGTH, &errorMessageLength);
            std::string errorMessage(errorMessageLength, ' ');
            glGetShaderInfoLog(geometryShader, errorMessageLength, 0, errorMessage.data());
            error("Geometry Shader Compilation Error: (", filename, ")\n", errorMessage, '\n');
        }
        glAttachShader(program, geometryShader);
    }

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &errorMessageLength);
        std::string errorMessage(errorMessageLength, ' ');
        glGetProgramInfoLog(program, errorMessageLength, 0, errorMessage.data());
        error("Shader Link Error: (", filename, ")\n", errorMessage, '\n');
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(geometryShader);

    glCreateBuffers(1, &worldInfoUBO);
    glNamedBufferData(worldInfoUBO, sizeof(WorldInfo), &worldInfo, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, worldInfoUBO);

    return { program };
}

#ifndef NDEBUG
static void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity,
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader = loadShader("shaders/shader.glsl", false);

    return window;
}

void Renderer::render(f32 deltaTime)
{
    worldInfo.time = (f32)getTime();
    glNamedBufferSubData(worldInfoUBO, 0, sizeof(WorldInfo), &worldInfo);

    // 3D
    glViewport(0, 0, game.windowWidth, game.windowHeight);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, 0.f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glUseProgram(shader.program);

    Camera const& camera = cameras[0];
    for (auto const& r : renderList)
    {
        glm::mat4 worldViewMatrix = camera.view * r.worldTransform;
        glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(worldViewMatrix));
        glm::mat4 worldViewProjectionMatrix = camera.projection * worldViewMatrix;
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(r.worldTransform));
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(worldViewProjectionMatrix));
        glUniformMatrix3fv(2, 1, GL_FALSE, glm::value_ptr(normalMatrix));

        GLMesh const& mesh = loadedMeshes[r.meshHandle];
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_INT, 0);
    }

    // 2D
    /*
    glViewport(0, 0, window_width, window_height);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    */

    SDL_GL_SwapWindow(game.window);
}

u32 Renderer::loadMesh(Mesh const& meshData)
{
    GLMesh mesh;
    glCreateBuffers(1, &mesh.vbo);
    glNamedBufferData(mesh.vbo, meshData.vertices.size() * sizeof(meshData.vertices[0]), meshData.vertices.data(), GL_STATIC_DRAW);

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

    mesh.numIndices = meshData.indices.size();
    loadedMeshes.push_back(mesh);
    return (u32)(loadedMeshes.size() - 1);
}

u32 Renderer::loadTexture(Texture const& texture)
{
    GLuint tex;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 0, GL_RGBA, texture.width, texture.height);
    glTextureSubImage2D(tex, 0, 0, 0, texture.width, texture.height, GL_RGBA,
            GL_UNSIGNED_BYTE, texture.data);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAX_ANISOTROPY, 8);
    glGenerateTextureMipmap(tex);
    loadedTextures.push_back({ tex });
    return loadedTextures.size() - 1;
}

void Renderer::setViewportCount(u32 viewports)
{
    cameras.resize(viewports);
}

Camera& Renderer::setViewportCamera(u32 index, glm::vec3 const& from, glm::vec3 const& to, f32 fov, f32 near, f32 far)
{
    Camera& cam = cameras[index];
    cam.position = from;
    cam.fov = fov;
    cam.near = near;
    cam.far = far;
    cam.view = glm::lookAt(from, to, glm::vec3(0, 0, 1));
    glm::vec2 dim = glm::vec2(game.windowWidth, game.windowHeight)
        * viewportLayout[cameras.size() - 1][index].scale;
    cam.aspectRatio = dim.x / dim.y;
    cam.projection = glm::perspective(glm::radians(fov), cam.aspectRatio, near, far);
    cam.viewProjection = cam.projection * cam.view;
    return cam;
}

void Renderer::drawMesh(Mesh const& mesh, glm::mat4 const& worldTransform)
{
    drawMesh(mesh.renderHandle, worldTransform);
}

void Renderer::drawMesh(u32 renderHandle, glm::mat4 const& worldTransform)
{
    renderList.push_back({ renderHandle, worldTransform });
}

void Renderer::setBackgroundColor(glm::vec3 color)
{
    backgroundColor = color;
}

void Renderer::addPointLight(glm::vec3 position, glm::vec3 color, f32 attenuation)
{
}

void Renderer::addSpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 color, f32 innerRadius, f32 outerRadius, f32 attenuation)
{
}

void Renderer::addDirectionalLight(glm::vec3 direction, glm::vec3 color)
{
    worldInfo.sunDirection = -glm::normalize(direction);
    worldInfo.sunColor = color;
}
