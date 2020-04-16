// unity build

#if _WIN32
#include <windows.h>
#endif

#include "game.cpp"
#include "scene.cpp"
#include "renderer.cpp"
#include "datafile.cpp"
#include "resources.cpp"
#include "vehicle.cpp"
#include "vehicle_data.cpp"
#include "font.cpp"
#include "track_graph.cpp"
#include "motion_grid.cpp"
#include "smoke_particles.cpp"
#include "audio.cpp"
#include "driver.cpp"
#include "mesh.cpp"
#include "editor.cpp"
#include "editor_camera.cpp"
#include "terrain.cpp"
#include "track.cpp"
#include "dynamic_buffer.cpp"
#include "decal.cpp"
#include "menu.cpp"
#include "gui.cpp"
#include "weapon.cpp"
#include "entities/projectile.cpp"
#include "entities/mine.cpp"
#include "entities/flash.cpp"
#include "entities/static_mesh.cpp"
#include "entities/tree.cpp"
#include "entities/static_decal.cpp"
#include "entities/start.cpp"
#include "entities/booster.cpp"
#include "entities/oil.cpp"
#include "entities/glue.cpp"
#include "entities/entities.cpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int main(int argc, char** argv)
{
    g_game.run();
    return EXIT_SUCCESS;
}
