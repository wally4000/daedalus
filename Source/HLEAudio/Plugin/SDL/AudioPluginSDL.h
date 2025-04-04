#pragma once 

#include "HLEAudio/Plugin/SDL/RingBuffer.h"
#include <SDL2/SDL.h>

class AudioPlugin : public CAudioPlugin
{
public:
    AudioPlugin();
    virtual ~AudioPlugin();


    virtual u32             ReadLength()            { return 0; }
   // Uploads a new buffer and returns status

    static void         AudioSyncFunction(void * arg);
     void              AudioThread();
     void StopAudioDevice() override;

private:

    SDL_AudioDeviceID       mAudioDevice;

    
};
