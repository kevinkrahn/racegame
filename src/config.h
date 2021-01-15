#pragma once

#include "math.h"
#include "datafile.h"

const char* CONFIG_FILE_PATH = "config.txt";

enum struct ConfigLevel
{
    LOW = 0,
    MEDIUM = 1,
    HIGH = 2,
};

struct Config
{
    struct Graphics
    {
        u32 resolutionX = 1280;
        u32 resolutionY = 720;
        u32 shadowMapResolution = 2048;
        ConfigLevel shadowFilteringQuality = ConfigLevel::MEDIUM;
        u32 maxFPS = 200;
        u32 msaaLevel = 0;
        bool fullscreen = false;
        bool vsync = true;
        bool shadowsEnabled = true;
        ConfigLevel ssaoQuality = ConfigLevel::HIGH;
        bool bloomEnabled = true;
        bool sharpenEnabled = false;
        bool motionBlurEnabled = true;
        bool pointLightsEnabled = true;
        bool fogEnabled = true;
        bool highQualityTerrainEnabled = true;
        bool highQualityTrackEnabled = true;
        u32 anisotropicFilteringLevel = 4;
        bool cloudShadowsEnabled = true;

        void serialize(Serializer& s)
        {
            s.field(resolutionX);
            s.field(resolutionY);
            s.field(shadowMapResolution);
            s.field(shadowFilteringQuality);
            s.field(maxFPS);
            s.field(msaaLevel);
            s.field(fullscreen);
            s.field(vsync);
            s.field(shadowsEnabled);
            s.field(ssaoQuality);
            s.field(bloomEnabled);
            s.field(sharpenEnabled);
            s.field(motionBlurEnabled);
            s.field(pointLightsEnabled);
            s.field(fogEnabled);
            s.field(highQualityTerrainEnabled);
            s.field(highQualityTrackEnabled);
            s.field(anisotropicFilteringLevel);
            s.field(cloudShadowsEnabled);
        }
    } graphics;

    struct Audio
    {
        f32 masterVolume = 1.f;
        f32 sfxVolume = 1.f;
        f32 vehicleVolume = 1.f;
        f32 musicVolume = 1.f;

        void serialize(Serializer& s)
        {
            s.field(masterVolume);
            s.field(sfxVolume);
            s.field(vehicleVolume);
            s.field(musicVolume);
        }
    } audio;

    struct Gameplay
    {
        f32 hudTrackScale = 1.f;
        f32 hudTextScale = 1.f;
        bool forceFeedbackEnabled = true;

        void serialize(Serializer& s)
        {
            s.field(hudTrackScale);
            s.field(hudTextScale);
            s.field(forceFeedbackEnabled);
        }
    } gameplay;

    void serialize(Serializer& s)
    {
        s.field(graphics);
        s.field(audio);
        s.field(gameplay);
    }

    void save() { Serializer::toFile(*this, CONFIG_FILE_PATH); }
    void load() { Serializer::fromFile(*this, CONFIG_FILE_PATH); }
};

