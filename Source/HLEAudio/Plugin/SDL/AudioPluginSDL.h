#pragma once 

class AudioPlugin : public CAudioPlugin
{
public:
    AudioPlugin();
    virtual ~AudioPlugin();

    virtual void            StopEmulation();

    virtual void            DacrateChanged(int system_type);
    virtual void            LenChanged();
    virtual u32             ReadLength()            { return 0; }
    virtual EProcessResult  ProcessAList();

    void                    AddBuffer(void * ptr, u32 length);   // Uploads a new buffer and returns status
    void                    StopAudio();                        // Stops the Audio PlayBack (as if paused)
    void                    StartAudio();                       // Starts the Audio PlayBack (as if unpaused)

    static void             AudioSyncFunction(void * arg);
    static int              AudioThread(void * arg);

    virtual void SetMode(EAudioPluginMode mode) override;
    EAudioPluginMode GetMode() const override { return audioPluginmode; }

private:
    u32                     mFrequency;
    SDL_Thread*             mAudioThread;
    SDL_AudioDeviceID       mAudioDevice;
    EAudioPluginMode audioPluginmode = APM_DISABLED;
    
};
