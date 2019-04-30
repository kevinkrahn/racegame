// unity build

#if _WIN32
#include <windows.h>
#endif

#include "game.cpp"
#include "scene.cpp"
#include "renderer_opengl.cpp"
#include "datafile.cpp"
#include "resources.cpp"
#include "vehicle.cpp"
#include "vehicle_data.cpp"
#include "font.cpp"
#include "track_graph.cpp"
#include "particle_system.cpp"
#include "audio.cpp"
#include "driver.cpp"
#include "mesh.cpp"
#include "editor.cpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int main(int argc, char** argv)
{
    game.run();
    return EXIT_SUCCESS;
}
