#pragma once

#include "misc.h"
#include "math.h"
#include "resource.h"
#include <mutex>

enum struct AudioFormat
{
    RAW,
    VORBIS
};

struct Sound : public Resource
{
    // serialized
    std::string sourceFilePath;
    Array<u8> rawAudioData;
    u32 numSamples = 0;
    u32 numChannels = 0;
    AudioFormat format = AudioFormat::RAW;
    f32 volume = 1.f;
    f32 falloffDistance = 90.f;

    void serialize(Serializer& s) override
    {
        Resource::serialize(s);

        s.field(sourceFilePath);
        s.field(rawAudioData);
        s.field(numSamples);
        s.field(numChannels);
        s.field(format);
        s.field(volume);
        s.field(falloffDistance);

        if (s.deserialize)
        {
            if (format == AudioFormat::VORBIS)
            {
                decodeVorbisData();
            }
            else
            {
                audioData = (i16*)rawAudioData.data();
            }
        }
    }

    i16* audioData;
    Array<i16> decodedAudioData;

    Sound() {}
    Sound(const char* filename);

    void loadFromFile(const char* filename);
    void decodeVorbisData();
};

int decodeVorbis(u8* oggData, int len, int* channels, int* sampleRate, short** data);

enum struct SoundType
{
    VEHICLE,
    GAME_SFX,
    MENU_SFX,
    MUSIC
};

using SoundHandle = u32;
class Audio
{
    SDL_AudioDeviceID audioDevice;

    void audioCallback(u8* buf, i32 len);
    friend void SDLAudioCallback(void* userdata, u8* buf, i32 len);

    struct PlayingSound
    {
        Sound* sound = nullptr;
        f32 pitch = 1.f;
        f32 volume = 1.f;
        f32 pan = 1.f;
        f32 targetPitch = 1.f;
        f32 playPosition = 0.f;
        // TODO: these could be combined into a single bitfield
        bool isLooping = false;
        bool isPaused = false;
        bool is3D = false;
        SoundHandle handle = 0;
        Vec3 position = {};
        SoundType soundType;
    };

    struct PlaybackModification
    {
        enum
        {
            VOLUME,
            PITCH,
            PAN,
            POSITION,
            PLAY,
            STOP,
            SET_PAUSED,
            STOP_GAMEPLAY_SOUNDS
        } type;

        SoundHandle handle;
        union
        {
            f32 volume;
            f32 pitch;
            f32 pan;
            Vec3 position;
            PlayingSound newSound;
            bool paused;
        };

        PlaybackModification() {}
    };

    SoundHandle nextSoundHandle = 0;
    bool pauseGameplaySounds = false;

    Array<PlayingSound> playingSounds; // modified only by audio thread
    Array<PlaybackModification> playbackModifications;
    SmallArray<Vec3> listenerPositions;

    RandomSeries randomSeries;

    std::mutex audioMutex;

public:
    void init();
    void close();

    SoundHandle playSound(Sound* sound, SoundType soundType,
            bool loop = false, f32 pitch = 1.f, f32 volume = 1.f, f32 pan = 0.f);
    SoundHandle playSound3D(Sound* sound, SoundType soundType,
            Vec3 const& position, bool loop = false, f32 pitch = 1.f,
            f32 volume = 1.f, f32 pan = 0.f);
    void stopSound(SoundHandle handle);
    void setSoundPitch(SoundHandle handle, f32 pitch);
    void setSoundVolume(SoundHandle handle, f32 volume);
    void setSoundPan(SoundHandle handle, f32 pan);
    void setSoundPosition(SoundHandle handle, Vec3 const& position);
    void setSoundPaused(SoundHandle handle, bool paused);
    void setListeners(SmallArray<Vec3>const& listeners);
    void setPaused(bool paused);
    void stopAllGameplaySounds();
} g_audio;

void SDLAudioCallback(void* userdata, u8* buf, i32 len);
