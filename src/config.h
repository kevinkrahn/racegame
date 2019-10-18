#pragma once

#include "math.h"
#include "datafile.h"

struct Config
{
    struct Graphics
    {
        u32 resolutionX = 1280;
        u32 resolutionY = 720;
        bool fullscreen = false;
        u32 maxFPS = 200;
        bool vsync = true;
        u32 shadowMapResolution = 2048;
        bool shadowsEnabled = true;
        bool ssaoEnabled = true;
        bool ssaoHighQuality = false; // TODO: Implement
        bool bloomEnabled = true;
        u32 msaaLevel = 0;
    } graphics;

    struct Audio
    {
        f32 masterVolume = 0.f;
        f32 sfxVolume = 1.f;
        f32 vehicleVolume = 1.f;
        f32 musicVolume = 1.f;
    } audio;

    struct Gameplay
    {
        f32 hudTrackScale = 1.f;
        f32 hudTextScale = 1.f;
    } gameplay;

    DataFile::Value serialize()
    {
        DataFile::Value data;
        data["resolutionX"] = DataFile::makeInteger(graphics.resolutionX);
        data["resolutionY"] = DataFile::makeInteger(graphics.resolutionY);
        data["fullscreen"] = DataFile::makeBool(graphics.fullscreen);
        data["maxFPS"] = DataFile::makeInteger(graphics.maxFPS);
        data["vsync"] = DataFile::makeBool(graphics.vsync);
        data["shadowMapResolution"] = DataFile::makeInteger(graphics.shadowMapResolution);
        data["shadowsEnabled"] = DataFile::makeBool(graphics.shadowsEnabled);
        data["ssaoEnabled"] = DataFile::makeBool(graphics.ssaoEnabled);
        data["ssaoHighQuality"] = DataFile::makeBool(graphics.ssaoHighQuality);
        data["bloomEnabled"] = DataFile::makeBool(graphics.bloomEnabled);
        data["msaaLevel"] = DataFile::makeInteger(graphics.msaaLevel);

        data["masterVolume"] = DataFile::makeReal(audio.masterVolume);
        data["sfxVolume"] = DataFile::makeReal(audio.sfxVolume);
        data["vehicleVolume"] = DataFile::makeReal(audio.vehicleVolume);
        data["musicVolume"] = DataFile::makeReal(audio.musicVolume);

        data["hudTrackScale"] = DataFile::makeReal(gameplay.hudTrackScale);
        data["hudTextScale"] = DataFile::makeReal(gameplay.hudTextScale);

        return data;
    }

    void deserialize(DataFile::Value& data)
    {
        Config d;
        graphics.resolutionX = (u32)data["resolutionX"].integer(d.graphics.resolutionX);
        graphics.resolutionY = (u32)data["resolutionY"].integer(d.graphics.resolutionY);
        graphics.fullscreen = data["fullscreen"].boolean(d.graphics.fullscreen);
        graphics.maxFPS = (u32)data["maxFPS"].integer(d.graphics.maxFPS);
        graphics.vsync = data["vsync"].boolean(d.graphics.vsync);
        graphics.shadowMapResolution = (u32)data["shadowMapResolution"].integer(d.graphics.shadowMapResolution);
        graphics.shadowsEnabled = data["shadowsEnabled"].boolean(d.graphics.shadowsEnabled);
        graphics.ssaoEnabled = data["ssaoEnabled"].boolean(d.graphics.ssaoEnabled);
        graphics.ssaoHighQuality = data["ssaoHighQuality"].boolean(d.graphics.ssaoHighQuality);
        graphics.bloomEnabled = data["bloomEnabled"].boolean(d.graphics.bloomEnabled);
        graphics.msaaLevel = (u32)data["msaaLevel"].integer(d.graphics.msaaLevel);

        audio.masterVolume = data["masterVolume"].real(d.audio.masterVolume);
        audio.sfxVolume = data["sfxVolume"].real(d.audio.sfxVolume);
        audio.vehicleVolume = data["vehicleVolume"].real(d.audio.vehicleVolume);
        audio.musicVolume = data["musicVolume"].real(d.audio.musicVolume);

        gameplay.hudTrackScale = data["hudTrackScale"].real(d.gameplay.hudTrackScale);
        gameplay.hudTextScale = data["hudTextScale"].real(d.gameplay.hudTextScale);
    }
};

