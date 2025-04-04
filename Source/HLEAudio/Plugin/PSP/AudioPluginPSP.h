class AudioPlugin : public CAudioPlugin
{
public:

 	AudioPlugin();
	virtual ~AudioPlugin();

	virtual void			StopEmulation();

	virtual void			DacrateChanged( int system_type );
	virtual void			LenChanged();
	virtual u32				ReadLength() {return 0;}
	virtual EProcessResult	ProcessAList();

	//virtual void SetFrequency(u32 frequency);
	virtual void AddBuffer( u8 * start, u32 length);
	virtual void FillBuffer( Sample * buffer, u32 num_samples);

	virtual void StopAudio();
	virtual void StartAudio();
	virtual void SetMode(EAudioPluginMode mode) override;
	virtual EAudioPluginMode GetMode() const override;
public:
  CAudioBuffer *		mAudioBufferUncached;

private:
	CAudioBuffer * mAudioBuffer;
	bool mKeepRunning;
	bool mExitAudioThread;
	u32 mFrequency;
	s32 mAudioThread;
	s32 mSemaphore;
	int audio_channel;
//	u32 mBufferLenMs;
	bool audio_open = false;
	EAudioPluginMode audioPluginmode = APM_DISABLED;
};
