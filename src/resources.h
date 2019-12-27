#pragma once

#include "datafile.h"
#include "math.h"
#include "font.h"
#include "audio.h"
#include "mesh.h"
#include "texture.h"
#include <vector>
#include <map>
#include <string>

constexpr u8 whiteBytes[] = { 255, 255, 255, 255 };
constexpr u8 identityNormalBytes[] = { 128, 128, 255, 255 };
struct Textures
{
    Texture white = Texture("white", 1, 1, (u8*)whiteBytes, Texture::Format::RGBA8);
    Texture identityNormal = Texture("identityNormal", 1, 1, (u8*)identityNormalBytes, Texture::Format::RGBA8);
    Texture ammotick = Texture("textures/ammotick.png", false);
    Texture bark = Texture("textures/bark.png");
    Texture booster = Texture("textures/booster.png", false);
    Texture bouncer_projectile = Texture("textures/bouncer_projectile.png", false);
    Texture cactus = Texture("textures/cactus.png");
    Texture checkers = Texture("textures/checkers.png", false);
    Texture checkers_fade = Texture("textures/checkers_fade.png");
    Texture cheveron = Texture("textures/cheveron.png", false);
    Texture circle = Texture("textures/circle.png", false);
    Texture cloud_shadow = Texture("textures/cloud_shadow.png");
    Texture concrete = Texture("textures/concrete.jpg");
    Texture desert = Texture("textures/desert.jpg");
    Texture desert_stone = Texture("textures/desert_stone.jpg");
    Texture flare = Texture("textures/flare.png", false);
    Texture flash = Texture("textures/flash.png", false);
    Texture wood = Texture("textures/wood.jpg");
    Texture grass = Texture("textures/grass.jpg");
    Texture iconbg = Texture("textures/iconbg.png", false);
    Texture lava = Texture("textures/lava.jpg");
    Texture leaves = Texture("textures/leaves.png");
    Texture plant1 = Texture("textures/plant1.png");
    Texture plant2 = Texture("textures/plant2.png");
    Texture rock = Texture("textures/rock.jpg");
    Texture rock_moss = Texture("textures/rock_moss.jpg");
    Texture rumble = Texture("textures/rumble.png");
    Texture sand = Texture("textures/sand.jpg");
    Texture smoke = Texture("textures/smoke.png", false);
    Texture snow = Texture("textures/snow.jpg");
    Texture tarmac = Texture("textures/tarmac.jpg");
    Texture tiremarks = Texture("textures/tiremarks.png");
    Texture weapon_iconbg = Texture("textures/weapon_iconbg.png", false);
    Texture weapon_iconbg_selected = Texture("textures/weapon_iconbg_selected.png", false);
    Texture billboard_1 = Texture("textures/billboard_1.jpg");
    Texture billboard_2 = Texture("textures/billboard_2.jpg");
    Texture billboard_3 = Texture("textures/billboard_3.jpg");
    Texture billboard_4 = Texture("textures/billboard_4.jpg");
    Texture oil_normal = Texture("textures/oil_normal.png", false, Texture::Format::RGBA8);
    Texture flames = Texture("textures/flames.png");

    Texture icon_decoration = Texture("textures/decoration_icon.png", false);
    Texture icon_blaster = Texture("textures/icons/blaster_icon.png", false);
    Texture icon_bouncer = Texture("textures/icons/bouncer_icon.png", false);
    Texture icon_glow = Texture("textures/icons/glow_icon.png", false);
    Texture icon_armor = Texture("textures/icons/icon_armor.png", false);
    Texture icon_kinetic_armor = Texture("textures/icons/icon_kinetic_armor.png", false);
    Texture icon_underplating = Texture("textures/icons/icon_underplating.png", false);
    Texture icon_drivetrain = Texture("textures/icons/icon_drivetrain.png", false);
    Texture icon_engine = Texture("textures/icons/icon_engine.png", false);
    Texture icon_flag = Texture("textures/icons/icon_flag.png", false);
    Texture icon_pistons = Texture("textures/icons/icon_pistons.png", false);
    Texture icon_spikes = Texture("textures/icons/icon_spikes.png", false);
    Texture icon_spraycan = Texture("textures/icons/icon_spraycan.png", false);
    Texture icon_stats = Texture("textures/icons/icon_stats.png", false);
    Texture icon_stats2 = Texture("textures/icons/icon_stats2.png", false);
    Texture icon_suspension = Texture("textures/icons/icon_suspension.png", false);
    Texture icon_weight = Texture("textures/icons/icon_weight.png", false);
    Texture icon_wheel = Texture("textures/icons/icon_wheel.png", false);
    Texture icon_jumpjet = Texture("textures/icons/jumpjet_icon.png", false);
    Texture icon_left_turn_track = Texture("textures/icons/left_turn_track_icon.png", false);
    Texture icon_right_turn_track = Texture("textures/icons/right_turn_track_icon.png", false);
    Texture icon_mg = Texture("textures/icons/mg_icon.png", false);
    Texture icon_mine = Texture("textures/icons/mine_icon.png", false);
    Texture icon_missile = Texture("textures/icons/missile_icon.png", false);
    Texture icon_oil = Texture("textures/icons/oil.png", false);
    Texture icon_glue = Texture("textures/icons/glue.png", false);
    Texture icon_rocketbooster = Texture("textures/icons/rocketbooster_icon.png", false);
    Texture icon_straight_track = Texture("textures/icons/straight_track_icon.png", false);
    Texture icon_terrain = Texture("textures/icons/terrain_icon.png", false);
    Texture icon_track = Texture("textures/icons/track_icon.png", false);
    Texture icon_sell = Texture("textures/icons/icon_sell.png", false);

    Texture decal_arrow = Texture("textures/decals/arrow.png");
    Texture decal_arrow_left = Texture("textures/decals/arrow_left.png");
    Texture decal_arrow_right = Texture("textures/decals/arrow_right.png");
    Texture decal_crack = Texture("textures/decals/crack.png");
    Texture decal_grunge1 = Texture("textures/decals/grunge1.png");
    Texture decal_grunge2 = Texture("textures/decals/grunge2.png");
    Texture decal_grunge3 = Texture("textures/decals/grunge3.png");
    Texture decal_patch1 = Texture("textures/decals/patch1.png");
    Texture decal_sand = Texture("textures/decals/sand.png");

    Texture sky_cubemap = Texture({
        "textures/sky/sky_ft.png",
        "textures/sky/sky_bk.png",
        "textures/sky/sky_lf.png",
        "textures/sky/sky_rt.png",
        "textures/sky/sky_up.png",
        "textures/sky/sky_dn.png"
    });
};

struct Sounds
{
    Sound evironment = Sound("sounds/environment.ogg");
    Sound blaster = Sound("sounds/blaster.wav");
    Sound blaster_hit = Sound("sounds/blaster_hit.wav");
    Sound bouncer_bounce = Sound("sounds/bouncer_bounce.wav");
    Sound bouncer_fire = Sound("sounds/bouncer_fire.wav");
    Sound clapping = Sound("sounds/clapping.wav");
    Sound click = Sound("sounds/click.wav");
    Sound close = Sound("sounds/close.wav");
    Sound countdown_a = Sound("sounds/countdown_a.wav");
    Sound countdown_b = Sound("sounds/countdown_b.wav");
    //Sound engine = Sound("sounds/engine.wav");
    Sound engine2 = Sound("sounds/engine2.wav");
    //Sound engine_overlay = Sound("sounds/engine_overlay.wav");
    Sound explosion1 = Sound("sounds/explosion1.wav");
    Sound explosion2 = Sound("sounds/explosion2.wav");
    Sound explosion3 = Sound("sounds/explosion3.wav");
    Sound explosion4 = Sound("sounds/explosion4.wav");
    Sound explosion5 = Sound("sounds/explosion5.wav");
    Sound explosion6 = Sound("sounds/explosion6.wav");
    Sound explosion7 = Sound("sounds/explosion7.wav");
    Sound impact = Sound("sounds/impact.wav");
    Sound lap = Sound("sounds/lap.wav");
    //Sound mg = Sound("sounds/mg.wav");
    Sound mg2 = Sound("sounds/mg2.wav");
    Sound missile = Sound("sounds/missile.wav");
    Sound nono = Sound("sounds/nono.wav");
    Sound oil = Sound("sounds/oil.wav");
    Sound glue = Sound("sounds/glue.wav");
    Sound sticky = Sound("sounds/sticky.wav");
    Sound rocketboost = Sound("sounds/rocketboost.wav");
    Sound select = Sound("sounds/select.wav");
    Sound thunk = Sound("sounds/thunk.wav");
    Sound tires = Sound("sounds/tires.wav");
    Sound money = Sound("sounds/money.wav");
    Sound fixup = Sound("sounds/fixup.wav");
    Sound airdrill = Sound("sounds/airdrill.wav");

    Sound bullet_impact1 = Sound("sounds/impacts/bullet_impact1.wav");
    Sound bullet_impact2 = Sound("sounds/impacts/bullet_impact2.wav");
    Sound bullet_impact3 = Sound("sounds/impacts/bullet_impact3.wav");
    Sound richochet1 = Sound("sounds/impacts/richochet1.wav");
    Sound richochet2 = Sound("sounds/impacts/richochet2.wav");
    Sound richochet3 = Sound("sounds/impacts/richochet3.wav");
    Sound richochet4 = Sound("sounds/impacts/richochet4.wav");
};

const char* dataFiles[] = {
    "models/cactus.dat",
    "models/mine.dat",
    "models/plants.dat",
    "models/railing.dat",
    "models/railing2.dat",
    "models/rock.dat",
    "models/tree.dat",
    "models/world.dat",
    "models/barrel.dat",
    "models/ctvpole.dat",
    "models/billboard.dat",
    "models/money.dat",
    "models/wrench.dat",

    "models/vehicles/mini.dat",
    "models/vehicles/coolcar.dat",
    "models/vehicles/muscle.dat",
    "models/vehicles/racecar.dat",
    "models/vehicles/sportscar.dat",
    "models/vehicles/stationwagon.dat",
    "models/vehicles/truck.dat",

    "models/weapons/minigun.dat",
    "models/weapons/blaster.dat",
    "models/weapons/missile.dat",
    "models/weapons/bouncer.dat",
};

class Resources
{
private:
    std::map<std::string, Mesh> meshes;
    std::map<std::string, DataFile::Value::Dict> scenes;
    std::map<const char*, std::map<u32, Font>> fonts;

public:
    std::unique_ptr<Textures> textures;
    std::unique_ptr<Sounds> sounds;

    void load();

    Mesh* getMesh(const char* name)
    {
        auto iter = meshes.find(name);
        if (iter == meshes.end())
        {
            FATAL_ERROR("Mesh not found: ", name);
        }
        return &iter->second;
    }

    DataFile::Value::Dict& getScene(const char* name)
    {
        auto iter = scenes.find(name);
        if (iter == scenes.end())
        {
            FATAL_ERROR("Scene not found: ", name);
        }
        return iter->second;
    }

    Font& getFont(const char* name, u32 height)
    {
        auto iter = fonts.find(name);
        if (iter == fonts.end())
        {
            return fonts[name][height] = Font(std::string(name) + ".ttf", (f32)height);
        }

        auto iter2 = iter->second.find(height);
        if (iter2 == iter->second.end())
        {
            return fonts[name][height] = Font(std::string(name) + ".ttf", (f32)height);
        }

        return iter2->second;
    }
} g_res;

