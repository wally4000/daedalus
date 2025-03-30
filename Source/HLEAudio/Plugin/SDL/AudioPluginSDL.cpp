#include <SDL2/SDL.h>

#include "Base/Types.h"
#include "Interface/ConfigOptions.h"
#include "Utility/FramerateLimiter.h"
#include "Core/Memory.h"
#include "Debug/DBGConsole.h"
#include "HLEAudio/AudioPlugin.h"
#include "HLEAudio/HLEAudioInternal.h"
#include "System/Timing.h"
#include "HLEAudio/Plugin/SDL/AudioPluginSDL.h"

// EAudioPluginMode gAudioPluginEnabled = APM_ENABLED_ASYNC;

SDL_Thread* Asyncthread = nullptr;
int Asyncthreadreturn;

void* Audio_UcodeEntry(void* arg) {
    Audio_Ucode();
    return nullptr;
}

AudioPlugin::AudioPlugin()
    : mFrequency(44100), mAudioThread(nullptr), mAudioDevice(0)
{}

AudioPlugin::~AudioPlugin()
{
    StopAudio();
}

void AudioPlugin::SetMode(EAudioPluginMode mode)
{
    audioPluginmode = mode;
}



void AudioPlugin::StopEmulation()
{
    Audio_Reset();
    StopAudio();
}

void AudioPlugin::DacrateChanged(int system_type)
{
    u32 clock      = (system_type == ST_NTSC) ? VI_NTSC_CLOCK : VI_PAL_CLOCK;
    u32 dacrate    = Memory_AI_GetRegister(AI_DACRATE_REG);
    u32 frequency  = clock / (dacrate + 1);

    mFrequency = frequency;
}

void AudioPlugin::LenChanged()
{
    if (GetMode() > APM_DISABLED)
    {
        u32 address = Memory_AI_GetRegister(AI_DRAM_ADDR_REG) & 0xFFFFFF;
        u32 length  = Memory_AI_GetRegister(AI_LEN_REG);

        AddBuffer(g_pu8RamBase + address, length);
    }
}

EProcessResult AudioPlugin::ProcessAList()
{
    Memory_SP_SetRegisterBits(SP_STATUS_REG, SP_STATUS_HALT);

    EProcessResult result = PR_NOT_STARTED;

    switch (GetMode())
    {
        case APM_DISABLED:
            result = PR_COMPLETED;
            break;
            
        case APM_ENABLED_ASYNC:
            if (!Asyncthread) 
            {
                Asyncthread = SDL_CreateThread((SDL_ThreadFunction)Audio_UcodeEntry, "Audio_UcodeThread", nullptr);
                if (Asyncthread == nullptr) {
                    DBGConsole_Msg(0, "Failed to create Async thread: %s", SDL_GetError());
                } else {
                    SDL_DetachThread(Asyncthread);
                }
            }
            result = PR_COMPLETED;
            break;
            
        case APM_ENABLED_SYNC:
            Audio_Ucode();
            result = PR_COMPLETED;
            break;
    }

    return result;
}


void AudioPlugin::AddBuffer(void * ptr, u32 length)
{
    if (length == 0)
        return;

    if (mAudioThread == nullptr)
        StartAudio();

    u32 num_samples = length / sizeof(Sample);

    // Queue the samples into SDL's audio buffer
    if (SDL_QueueAudio(mAudioDevice, ptr, num_samples * sizeof(Sample)) != 0) {
        DBGConsole_Msg(0, "SDL_QueueAudio error: %s", SDL_GetError());
        return;
    }
}

int AudioPlugin::AudioThread(void * arg)
{
    AudioPlugin * plugin = static_cast<AudioPlugin *>(arg);

    SDL_AudioSpec audio_spec;
    SDL_zero(audio_spec);
    audio_spec.freq = plugin->mFrequency;
    audio_spec.format = AUDIO_S16SYS;
    audio_spec.channels = 2;
    audio_spec.samples = 4096;
    audio_spec.callback = NULL;
    audio_spec.userdata = plugin;

    plugin->mAudioDevice = SDL_OpenAudioDevice(NULL, 0, &audio_spec, NULL, 0);
    if (plugin->mAudioDevice == 0) {
        DBGConsole_Msg(0, "Failed to open audio: %s", SDL_GetError());
        return -1;
    }

    SDL_PauseAudioDevice(plugin->mAudioDevice, 0);

    return 0;
}

void AudioPlugin::StartAudio()
{
    mAudioThread = SDL_CreateThread(&AudioThread, "Audio", this);

    if (mAudioThread == nullptr)
    {
        DBGConsole_Msg(0, "Failed to start the audio thread!");
        FramerateLimiter_SetAuxillarySyncFunction(nullptr, nullptr);
    }
}

void AudioPlugin::StopAudio()
{
    if (mAudioThread == nullptr)
        return;

    if (mAudioThread != nullptr)
    {
        int threadReturnValue;
        SDL_WaitThread(mAudioThread, &threadReturnValue);
        mAudioThread = nullptr;
    }

    // Clear the remaining audio data in SDL's audio buffer
    if (mAudioDevice != 0) {
        SDL_ClearQueuedAudio(mAudioDevice);
        SDL_CloseAudioDevice(mAudioDevice);
        mAudioDevice = 0;
    }
}
