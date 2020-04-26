// unity build

#if _WIN32
#include <windows.h>
#endif

#include "game.cpp"
#include "scene.cpp"
#include "renderer.cpp"
#include "datafile.cpp"
#include "resources.cpp"
#include "texture.cpp"
#include "material.cpp"
#include "vehicle.cpp"
#include "vehicle_data.cpp"
#include "font.cpp"
#include "track_graph.cpp"
#include "motion_grid.cpp"
#include "smoke_particles.cpp"
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

#if 1
int main(int argc, char** argv)
{
    g_game.run();
    return EXIT_SUCCESS;
}

#else
int main(int argc, char** argv)
{
    int hello = 123;
    std::string yo = "bro";
    glm::vec2 p{1, 2};
    std::vector<i64> numbers = { 1, 2, 3, 4 };

    struct Thing
    {
        f32 a;
        f32 b;

        void serialize(Serializer& s)
        {
            s.field(a);
            s.field(b);
        }
    } thing = { 123.123f, 3.1415926535f };
    std::vector<Thing> things = { {1,2}, {3,4}, {5,6} };
    std::unique_ptr<f32> heapNumber(new f32(4444.55555f));
    std::unique_ptr<i32> heapNumber2(new i32(42));
    std::vector<std::unique_ptr<int>> heapNumbers;
    heapNumbers.push_back(std::make_unique<int>(0));
    heapNumbers.push_back(std::make_unique<int>(2));
    heapNumbers.push_back(std::make_unique<int>(4));
    heapNumbers.push_back(std::make_unique<int>(6));
    heapNumbers.push_back(std::make_unique<int>(8));

    auto dict = DataFile::makeDict();

    Serializer s(dict, false);
    s.field(hello);
    s.value("yo", yo);
    s.field(p);
    s.field(thing);
    s.field(numbers);
    s.field(things);
    s.field(heapNumber);
    s.field(heapNumber2);
    s.field(heapNumbers);

    print(dict);

    hello = 0;
    yo = "yo";
    p = { 0, 0 };
    thing = { 0, 0 };
    numbers = {};
    things = {};
    Serializer s2(dict, true);
    s2.field(hello);
    s2.field(yo);
    s2.field(p);
    s2.field(thing);
    s2.field(numbers);
    s2.field(things);
    s2.field(heapNumbers);

    print("hello: ", hello, '\n');
    print("yo: ", yo, '\n');
    print("p: ", p, '\n');
    print("thing: ", thing.a, ", ", thing.b, '\n');
    print("numbers: ");
    for (auto& n : numbers)
    {
        print(n, ", ");
    }
    print('\n');
    print("things: ");
    for (auto& t : things)
    {
        print("{ ", t.a, ", ", t.b, " }, ");
    }
    print('\n');
    print("heapNumbers: ");
    for (auto& t : heapNumbers)
    {
        print(*t, ", ");
    }
    print('\n');

    return EXIT_SUCCESS;
}
#endif
