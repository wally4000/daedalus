/*
Copyright (C) 2003 Azimer
Copyright (C) 2001,2006 StrmnNrmn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

//
//	N.B. This source code is derived from Azimer's Audio plugin (v0.55?)
//	and modified by StrmnNrmn to work with Daedalus PSP. Thanks Azimer!
//	Drop me a line if you get chance :)
//



#include <3ds.h>
#include <cstring>

#include "AudioPluginCTR.h"
#include "AudioOutput.h"
#include "HLEAudio/AudioPlugin.h"
#include "HLEAudio/HLEAudioInternal.h"

#include "Interface/ConfigOptions.h"
#include "Core/CPU.h"
#include "Core/Interrupt.h"
#include "Core/Memory.h"
#include "Core/ROM.h"
#include "Core/RSP_HLE.h"
#include "System/SystemInit.h"

#define RSP_AUDIO_INTR_CYCLES     1

#define DEFAULT_FREQUENCY 44100	// Taken from Mupen64 : )

extern bool isN3DS;
// FIXME: Hack!

static bool _runThread = false;

static Thread asyncThread;
static Handle audioRequest;
extern u32 gSoundSync;

static const u32 DESIRED_OUTPUT_FREQUENCY = 44100;

static const u32 CTR_BUFFER_SIZE  = 1024 * 2;
static const u32 CTR_BUFFER_COUNT = 6;
static const u32 CTR_NUM_SAMPLES  = 512;


static ndspWaveBuf waveBuf[CTR_BUFFER_COUNT];
static unsigned int waveBuf_id;

bool audioOpen = false;


static void asyncProcess(void *arg)
{
	while(_runThread)
	{
		svcWaitSynchronization(audioRequest, U64_MAX);
		if(_runThread) Audio_Ucode();
	}
}


//bool gAdaptFrequency( false );


AudioPlugin::AudioPlugin()	
:	mAudioPlaying( false )
,	mFrequency( 44100 )
{
	mAudioBuffer = new CAudioBuffer( CTR_BUFFER_SIZE );
	if(isN3DS)
	{
		_runThread = true;

		svcCreateEvent(&audioRequest, RESET_ONESHOT);
		asyncThread = threadCreate(asyncProcess, 0, (8 * 1024), 0x18, 2, true);
	}
}


AudioPlugin::~AudioPlugin()
{
	if(isN3DS)
	{
		_runThread = false;
		
		svcSignalEvent(audioRequest);
		threadJoin(asyncThread, U64_MAX);
	}
	StopAudio();
	delete mAudioBuffer;
}

//*****************************************************************************
/*
void	AudioPlugin::SetAdaptFrequecy( bool adapt )
{
	etAdaptFrequency( adapt );
}
*/


void AudioPlugin::StopEmulation()
{
	Audio_Reset();
	StopAudio();
}

void AudioPlugin::SetMode(EAudioPluginMode mode)
{
	audioPluginmode = mode;
}

EAudioPluginMode AudioPlugin::GetMode() const
{
	return audioPluginmode;
}


void	AudioPlugin::DacrateChanged( int SystemType )
{
//	printf( "DacrateChanged( %s )\n", (SystemType == ST_NTSC) ? "NTSC" : "PAL" );
	u32 type = (u32)((SystemType == ST_NTSC) ? VI_NTSC_CLOCK : VI_PAL_CLOCK);
	u32 dacrate = Memory_AI_GetRegister(AI_DACRATE_REG);
	u32	frequency = type / (dacrate + 1);

	SetFrequency( frequency );
}



void	AudioPlugin::LenChanged()
{
	if( GetMode() > APM_DISABLED )
	{
		//SetAdaptFrequency( gAdaptFrequency );

		u32		address( Memory_AI_GetRegister(AI_DRAM_ADDR_REG) & 0xFFFFFF );
		u32		length(Memory_AI_GetRegister(AI_LEN_REG));

		AddBuffer( g_pu8RamBase + address, length );
	}
	else
	{
		StopAudio();
	}
}


u32		AudioPlugin::ReadLength()
{
	return 0;
}


EProcessResult	AudioPlugin::ProcessAList()
{
	Memory_SP_SetRegisterBits(SP_STATUS_REG, SP_STATUS_HALT);

	EProcessResult	result( PR_NOT_STARTED );

	switch( GetMode() )
	{
		case APM_DISABLED:
			result = PR_COMPLETED;
			break;
		case APM_ENABLED_ASYNC:
			if(isN3DS) {
				svcSignalEvent(audioRequest);

				CPU_AddEvent(RSP_AUDIO_INTR_CYCLES, CPU_EVENT_AUDIO);
				result = PR_STARTED;
			}
			else
			{
				Audio_Ucode();
				result = PR_COMPLETED;
			}
			break;
		case APM_ENABLED_SYNC:
			Audio_Ucode();
			result = PR_COMPLETED;
			break;
	}

	return result;
}




static void audioCallback(void *arg)
{
	(void)arg;
	AudioPlugin* plugin = static_cast<AudioPlugin*>(arg);
	u32 samples_written = 0;

	if(waveBuf[waveBuf_id].status == NDSP_WBUF_DONE)
	{
		samples_written = plugin->mAudioBuffer->Drain( reinterpret_cast< Sample * >( waveBuf[waveBuf_id].data_pcm16 ), CTR_NUM_SAMPLES );

		if(samples_written != 0)
			waveBuf[waveBuf_id].nsamples = samples_written;

		DSP_FlushDataCache(waveBuf[waveBuf_id].data_pcm16, CTR_NUM_SAMPLES << 2);
		ndspChnWaveBufAdd( 0, &waveBuf[waveBuf_id] );

		waveBuf_id = (waveBuf_id + 1) % CTR_BUFFER_COUNT;
	}
}

static void AudioInit(AudioPlugin *plugin)
{
	if (ndspInit() != 0)
		return;

	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
	ndspChnSetRate(0, DESIRED_OUTPUT_FREQUENCY);

	for(u32 i = 0; i < CTR_BUFFER_COUNT; i++)
	{
		waveBuf[i].data_vaddr = linearAlloc(CTR_NUM_SAMPLES * 4);
		waveBuf[i].nsamples = CTR_NUM_SAMPLES;
		waveBuf[i].status = 0;

		memset(waveBuf[i].data_pcm16, 0, CTR_NUM_SAMPLES * 4);

		ndspChnWaveBufAdd(0, &waveBuf[i]);
	}

	waveBuf_id = 0;

	ndspSetCallback(&audioCallback, plugin);

	// Everything OK
	audioOpen = true;
}

static void AudioExit()
{
	// Stop stream
	ndspChnWaveBufClear(0);
	ndspExit();

	for(u32 i = 0; i < CTR_BUFFER_COUNT; i++)
	{
		linearFree((void*)waveBuf[i].data_vaddr);
	}

	audioOpen = false;
}


void AudioPlugin::SetFrequency( u32 frequency )
{
	mFrequency = frequency;
}

void AudioPlugin::AddBuffer( u8 *start, u32 length )
{
	if (length == 0)
		return;

	if (!mAudioPlaying)
		StartAudio();

	u32 num_samples = length / sizeof( Sample );

	u32 output_freq = DESIRED_OUTPUT_FREQUENCY;

	/*if (gAudioRateMatch)
	{
		if (gSoundSync > DESIRED_OUTPUT_FREQUENCY * 2)	
			output_freq = DESIRED_OUTPUT_FREQUENCY * 2;	//limit upper rate
		else if (gSoundSync < DESIRED_OUTPUT_FREQUENCY)
			output_freq = DESIRED_OUTPUT_FREQUENCY;	//limit lower rate
		else
			output_freq = gSoundSync;
	}*/

	mAudioBuffer->AddSamples( reinterpret_cast< const Sample * >( start ), num_samples, mFrequency, output_freq );
}

void AudioPlugin::StartAudio()
{
	if (mAudioPlaying)
		return;

	mAudioPlaying = true;

	AudioInit(this);

	audioOpen = true;
}

void AudioPlugin::StopAudio()
{
	if (!mAudioPlaying)
		return;

	mAudioPlaying = false;

	AudioExit();
}
