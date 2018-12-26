#pragma once

#include <SDL2/SDL.h>
#include "misc.h"
#include <vector>
#include <glm/vec3.hpp>

struct Mesh
{
    std::vector<f32> vertices;
    std::vector<u32> indices;
};

class Renderer
{
public:
    SDL_Window* initWindow(const char* name, u32 width, u32 height);
    void render(f32 deltaTime);
    void pointLight(glm::vec3 position, glm::vec3 color, f32 attenuation);
    void spotLight(glm::vec3 position, glm::vec3 color, f32 innerRadius, f32 outerRadius, f32 attenuation);
    void directionalLight(glm::vec3 direction, glm::vec3 color);
    void drawMesh(Mesh* mesh);
};
