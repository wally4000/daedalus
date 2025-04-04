#include "AudioPlugin.h"
#include "Debug/DBGConsole.h"
#include "HLEAudio/HLEAudioInternal.h"
#include "Utility/FramerateLimiter.h"
#include <thread> 

#ifdef DAEDALUS_SDL
#include "HLEAudio/Plugin/SDL/AudioPluginSDL.h"
#elif DAEDALUS_PSP
#endif
std::thread Asyncthread;

CAudioPlugin::CAudioPlugin() : mFrequency(44100) {} //: mAudioRunning(false)

CAudioPlugin::~CAudioPlugin()
{
    // StopEmulation();
}

// void CAudioPlugin::StopEmulation()
// {
//     // Lock Audio using CSingleton
//     mAudioRunning = false;

// }


void CAudioPlugin::DacrateChanged(int system_type)
{
    u32 clock      = (system_type == ST_NTSC) ? VI_NTSC_CLOCK : VI_PAL_CLOCK;
    u32 dacrate    = Memory_AI_GetRegister(AI_DACRATE_REG);
    u32 frequency  = clock / (dacrate + 1);

    mFrequency = frequency;
}

void CAudioPlugin::AddBuffer(void* ptr, u32 length)
{
    if (length == 0)
        return;

    if (!mAudioThread || !mAudioThread->joinable())
        StartAudio();

    // Push data to your ring buffer instead of directly to SDL_QueueAudio()
    if (!mRingBuffer.Push(ptr, length))
    {
        DBGConsole_Msg(0, "Ring buffer overflow, dropping audio data.");
    }
}


void CAudioPlugin::LenChanged()
{
    if (GetMode() > APM_DISABLED)
    {
        u32 address = Memory_AI_GetRegister(AI_DRAM_ADDR_REG) & 0xFFFFFF;
        u32 length  = Memory_AI_GetRegister(AI_LEN_REG);

        AddBuffer(g_pu8RamBase + address, length);
    }
}

EAudioPluginMode CAudioPlugin::GetMode() const
{
	return audioPluginmode;
}


EProcessResult CAudioPlugin::ProcessAList()
{
    Memory_SP_SetRegisterBits(SP_STATUS_REG, SP_STATUS_HALT);

    EProcessResult result = PR_NOT_STARTED;

    switch (GetMode())
    {
        case APM_DISABLED:
            result = PR_COMPLETED;
            break;
        case APM_ENABLED_ASYNC:
            Asyncthread = std::thread(Audio_Ucode);
            if (!Asyncthread.joinable()) {
                DBGConsole_Msg(0, "Failed to create Async thread");
            } else {
                Asyncthread.detach();
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
void CAudioPlugin::StartAudio()
{
    if(mAudioThread && mAudioThread->joinable())
        return;

        mAudioThread = std::make_unique<std::thread>([this]() { this->AudioThread(); });


    

    if (!mAudioThread->joinable())
    {
        DBGConsole_Msg(0, "Failed to start the audio thread!");
        FramerateLimiter_SetAuxillarySyncFunction(nullptr, nullptr);
    }
}

void CAudioPlugin::SetMode(EAudioPluginMode mode)
{
    audioPluginmode = mode;
}

void CAudioPlugin::StopAudio()
{
    if (mAudioThread && mAudioThread->joinable())
    {
        mAudioThread->join();
        mAudioThread.reset();

        StopAudioDevice();
    }
}
void CAudioPlugin::StopEmulation()
{
    Audio_Reset();
    StopAudio();
}

