// unity build

#include "game.cpp"
#include "scene.cpp"
#include "renderer.cpp"
#include "batcher.cpp"
#include "datafile.cpp"
#include "resources.cpp"
#include "texture.cpp"
#include "vehicle.cpp"
#include "vehicle_data.cpp"
#include "vehicle_physics.cpp"
#include "font.cpp"
#include "track_graph.cpp"
#include "motion_grid.cpp"
#include "particle_system.cpp"
#include "audio.cpp"
#include "driver.cpp"
#include "mesh.cpp"
#include "model.cpp"
#include "terrain.cpp"
#include "track.cpp"
#include "spline.cpp"
#include "dynamic_buffer.cpp"
#include "decal.cpp"
#include "menu.cpp"
#include "gui.cpp"
#include "weapon.cpp"
#include "entities/projectile.cpp"
#include "entities/mine.cpp"
#include "entities/flash.cpp"
#include "entities/static_mesh.cpp"
#include "entities/static_decal.cpp"
#include "entities/start.cpp"
#include "entities/booster.cpp"
#include "entities/oil.cpp"
#include "entities/glue.cpp"
#include "entities/entities.cpp"

#include "editor/editor_camera.cpp"
#include "editor/resource_manager.cpp"
#include "editor/editor.cpp"
#include "editor/model_editor.cpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_INCLUDE_LINE_GLSL
#define STB_INCLUDE_IMPLEMENTATION
#include <stb_include.h>

#if 1
int main(int argc, char** argv)
{
    g_game.run();
    return EXIT_SUCCESS;
}
#else
#include "util.h"
int main(int argc, char** argv)
{
    /*
    Array<i32> numbers = { 5, 4, 3, 1, 2, 6, 9, 8, 7, 0, 10 };
    numbers.stableSort();
    for (auto n : numbers)
    {
        print(n, '\n');
    }
    */

    Array<i32> numbers(1000);
    //i32 ns[] = { 346,3263,6346,326,346,1,7,104,10,4,54,4,4,44,4,4,16,1785,65,76,23,6432,5, 4, 3, 1, 2, 6, 9, 8, 7, 0, 10, 23, 2,3432,4,324,23 ,432, 2,23, 423,432, 654,64537,234,54,3,56534,256,236,537,254,7,57,5247,48,0,32,257,47,54,74,745,745,7 };
    i32 ns[] = { 500, 500, 500, 500, 400, 500, 400, 500, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1000, 2000, 2000, 200, 1000 };
    printf("%i elements\n", ARRAY_SIZE(ns));

    f64 startTime = getTime();
    for (u32 i=0; i<10000; ++i)
    {
        numbers.assign(ns, ns+ARRAY_SIZE(ns));
        numbers.stableSort();
    }
    print("insertion sort took: ", getTime() - startTime, " seconds\n");

    startTime = getTime();
    for (u32 i=0; i<10000; ++i)
    {
        numbers.assign(ns, ns+ARRAY_SIZE(ns));
        numbers.sort();
    }
    print("quick sort took: ", getTime() - startTime, " seconds\n");
}
#endif
