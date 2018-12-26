#pragma once

#include <SDL2/SDL.h>
#include "misc.h"
#include <vector>

class Renderer
{
public:
    SDL_Window* initWindow(const char* name, u32 width, u32 height);
    void render(f32 deltaTime);
};

struct Mesh
{
    std::vector<f32> vertices;
    std::vector<u32> indices;
};
