#pragma once

#include "misc.h"
#include "smallvec.h"
#include "math.h"
#include <mutex>
#include <vector>

struct Sound
{
    std::unique_ptr<i16[]> audioData;
    u32 numSamples;
    u32 numChannels;
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
        bool isLooping = false;
        bool isPaused = false;
        bool is3D = false;
        SoundHandle handle = 0;
        glm::vec3 position = {};
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
            MASTER_VOLUME,
            MASTER_PITCH,
        } type;

        SoundHandle handle;
        union
        {
            f32 volume;
            f32 pitch;
            f32 pan;
            glm::vec3 position;
            PlayingSound newSound;
            bool paused;
        };

        PlaybackModification() {}
    };

    SoundHandle nextSoundHandle = 0;

    f32 masterVolume = 1.f;
    f32 masterPitch = 1.f;

    std::vector<PlayingSound> playingSounds; // modified only by audio thread
    std::vector<PlaybackModification> playbackModifications;
    SmallVec<glm::vec3> listenerPositions;

    RandomSeries randomSeries;

    std::mutex audioMutex;

public:
    void init();
    void close();

    void setMasterVolume(f32 volume);
    void setMasterPitch(f32 pitch);
    SoundHandle playSound(Sound* sound, bool loop = false, f32 pitch = 1.f, f32 volume = 1.f, f32 pan = 0.f);
    SoundHandle playSound3D(Sound* sound, glm::vec3 const& position, bool loop = false, f32 pitch = 1.f, f32 volume = 1.f, f32 pan = 0.f);
    void stopSound(SoundHandle handle);
    void setSoundPitch(SoundHandle handle, f32 pitch);
    void setSoundVolume(SoundHandle handle, f32 volume);
    void setSoundPan(SoundHandle handle, f32 pan);
    void setSoundPosition(SoundHandle handle, glm::vec3 const& position);
    void setSoundPaused(SoundHandle handle, bool paused);
    void setListeners(SmallVec<glm::vec3>const& listeners);
};

void SDLAudioCallback(void* userdata, u8* buf, i32 len);
