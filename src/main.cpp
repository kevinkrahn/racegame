// unity build
#include "game.cpp"
#include "scene.cpp"
#include "renderer_opengl.cpp"
#include "datafile.cpp"
#include "resources.cpp"
#include "vehicle.cpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int main(int argc, char** argv)
{
    game.run();
}
