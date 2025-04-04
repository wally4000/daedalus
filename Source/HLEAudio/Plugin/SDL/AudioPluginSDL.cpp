#include <SDL2/SDL.h>
#include <thread>

#include "Base/Types.h"
#include "Interface/ConfigOptions.h"
#include "Utility/FramerateLimiter.h"
#include "Core/Memory.h"
#include "Debug/DBGConsole.h"
#include "HLEAudio/Plugin/AudioPlugin.h"
#include "HLEAudio/HLEAudioInternal.h"
#include "System/Timing.h"
#include "HLEAudio/Plugin/SDL/AudioPluginSDL.h"
#include "HLEAudio/Plugin/SDL/RingBuffer.h"
// EAudioPluginMode gAudioPluginEnabled = APM_ENABLED_ASYNC;



AudioPlugin::AudioPlugin()
    :  mAudioDevice(0)
{}

AudioPlugin::~AudioPlugin()
{
    StopAudioDevice();
}


void AudioPlugin::AudioThread()
{
    SDL_AudioSpec audio_spec{};
    audio_spec.freq = mFrequency;
    audio_spec.format = AUDIO_S16SYS;
    audio_spec.channels = 2;
    audio_spec.samples = 4096;

    mAudioDevice = SDL_OpenAudioDevice(NULL, 0, &audio_spec, NULL, 0);
    if (mAudioDevice == 0)
    {
        DBGConsole_Msg(0, "Failed to open Audio: %s", SDL_GetError());
        return;
    }

    SDL_PauseAudioDevice(mAudioDevice, 0); // Start playback

    std::vector<u8> buffer(4096);

    while (mAudioThread->joinable())
    {
        size_t bytesRead = mRingBuffer.Pop(buffer.data(), buffer.size());

        if (bytesRead > 0)
        {
            if (SDL_QueueAudio(mAudioDevice, buffer.data(), bytesRead) != 0)
            {
                DBGConsole_Msg(0, "SDL_QueueAudio error: %s", SDL_GetError());
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Prevent 100% CPU usage
    }
}


void AudioPlugin::StopAudioDevice()
{
    // Clear the remaining audio data in SDL's audio buffer
    if (mAudioDevice != 0) {
        SDL_ClearQueuedAudio(mAudioDevice);
        SDL_CloseAudioDevice(mAudioDevice);
        mAudioDevice = 0;
    }

}


