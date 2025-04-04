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


#include "Base/Types.h"


#include <pspkernel.h>
#include <pspaudiolib.h>
#include <pspaudio.h>

#include "Interface/ConfigOptions.h"
#include "Core/CPU.h"
#include "Core/Interrupt.h"
#include "Core/Memory.h"
#include "Core/ROM.h"
#include "Core/RSP_HLE.h"
#include "Debug/DBGConsole.h"
#include "HLEAudio/HLEAudioInternal.h"
#include "HLEAudio/AudioBuffer.h"
#include "HLEAudio/AudioPlugin.h"
#include "SysPSP/Utility/CacheUtil.h"
#include "Utility/FramerateLimiter.h"
#include "System/Thread.h"
#include "HLEAudio/Plugin/PSP/AudioPluginPSP.h"



int audio_channel = -1;
#ifdef DAEDALUS_PSP_USE_ME

#include "SysPSP/PRX/MediaEngine/me.h"
#include "SysPSP/Utility/ModulePSP.h"

std::unique_ptr<volatile struct me_struct> mei;

bool InitialiseMediaEngine()
{
	static bool mePRXloaded = false;

	if ( mePRXloaded == false)
	{
		if( CModule::Load("Plugins/mediaengine.prx") < 0 )	return false;

		mei = std::make_unique<volatile struct me_struct>(); 
		// mei = (volatile struct me_struct *)(make_uncached_ptr(mei));
		// sceKernelDcacheWritebackInvalidateAll();

		if (InitME(mei.get()) == 0)
		{
			mePRXloaded = true;
			return true;
		}
		else
		{
			#ifdef DAEDALUS_DEBUG_CONSOLE
			printf(" Couldn't initialize MediaEngine Instance\n");
			#endif
			return false;
		}
	}
	else
	{
		printf("Media Engine already Initialised\n");
		return true;
	}
}

#endif

#define RSP_AUDIO_INTR_CYCLES     1
extern u32 gSoundSync;

static const u32	kOutputFrequency = 44100;
static const u32	MAX_OUTPUT_FREQUENCY = kOutputFrequency * 4;


// Large kAudioBufferSize creates huge delay on sound //Corn
// static const u32	kAudioBufferSize = 1024 * 2; // OSX uses a circular buffer length, 1024 * 1024



void AudioPlugin::SetMode(EAudioPluginMode mode)
{
	audioPluginmode = mode;
}

EAudioPluginMode AudioPlugin::GetMode() const
{
	return audioPluginmode;
}

void AudioPlugin::FillBuffer(Sample * buffer, u32 num_samples)
{
	// sceKernelWaitSema( mSemaphore, 1, nullptr );

	mAudioBufferUncached->Drain( buffer, num_samples );

	// sceKernelSignalSema( mSemaphore, 1 );
}


AudioPlugin::AudioPlugin()
:mKeepRunning (false)
//: mAudioBuffer( kAudioBufferSize )
, mFrequency( 44100 )
// ,	mSemaphore( sceKernelCreateSema( "AudioPlugin", 0, 1, 1, nullptr ) )
//, mAudioThread ( kInvalidThreadHandle )
//, mKeepRunning( false )
//, mBufferLenMs ( 0 )
{
	// Allocate audio buffer with malloc_64 to avoid cached/uncached aliasing
	// alignas(64)	void * mem = new sizeof( CAudioBuffer );
	// mAudioBuffer = new( mem ) CAudioBuffer( kAudioBufferSize );
//   mAudioBufferUncached = (CAudioBuffer*)make_uncached_ptr(mem);
	// Ideally we could just invalidate this range?
	// dcache_wbinv_range_unaligned( mAudioBuffer, mAudioBuffer+sizeof( CAudioBuffer ) );

	#ifdef DAEDALUS_PSP_USE_ME
	InitialiseMediaEngine();
	#endif
}

AudioPlugin::~AudioPlugin( )
{
// 	mAudioBuffer->~CAudioBuffer();
//   free(mAudioBuffer);
//   sceKernelDeleteSema(mSemaphore);
//   pspAudioEnd();
}



void	AudioPlugin::StopEmulation()
{
    Audio_Reset();
  	StopAudio();
    // sceKernelDeleteSema(mSemaphore);
    // pspAudioEndPre();
    // sceKernelDelayThread(1000);
    // pspAudioEnd();


}

void	AudioPlugin::DacrateChanged( int system_type )
{
u32 clock = (system_type == ST_NTSC) ? VI_NTSC_CLOCK : VI_PAL_CLOCK;
u32 dacrate = Memory_AI_GetRegister(AI_DACRATE_REG);
u32 frequency = clock / (dacrate + 1);

#ifdef DAEDALUS_DEBUG_CONSOLE
DBGConsole_Msg(0, "Audio frequency: %d", frequency);
#endif
mFrequency = frequency;
}


void	AudioPlugin::LenChanged()
{
	if( GetMode() > APM_DISABLED )
	{
		u32 address = Memory_AI_GetRegister(AI_DRAM_ADDR_REG) & 0xFFFFFF;
		u32	length = Memory_AI_GetRegister(AI_LEN_REG);

		AddBuffer( g_pu8RamBase + address, length );
	}
	else
	{
		StopAudio();
	}
}


EProcessResult	AudioPlugin::ProcessAList()
{
	Memory_SP_SetRegisterBits(SP_STATUS_REG, SP_STATUS_HALT);

	EProcessResult	result = PR_NOT_STARTED;

	switch( GetMode() )
	{
		case APM_DISABLED:
			result = PR_COMPLETED;
			break;
		case APM_ENABLED_ASYNC:
			{
#ifdef DAEDALUS_PSP_USE_ME
				// sceKernelDcacheWritebackInvalidateAll();
				if(BeginME( mei.get(), (int)&Audio_Ucode, (int)NULL, -1, NULL, -1, NULL) < 0){
						Audio_Ucode();
						result = PR_COMPLETED;
						break;
				}
#else
				DAEDALUS_ERROR("Async audio is unimplemented");
				Audio_Ucode();
				result = PR_COMPLETED;
				break;
#endif
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


// void audioCallback( void * buf, unsigned int length, void * userdata )
// {
// 	AudioPlugin * ac( reinterpret_cast< AudioPlugin * >( userdata ) );

// 	ac->FillBuffer( reinterpret_cast< Sample * >( buf ), length );
// }


void AudioPlugin::StartAudio()
{
	if (mKeepRunning)
		return;

	mKeepRunning = true;


    audio_channel = sceAudioChReserve(-1, 512, PSP_AUDIO_FORMAT_STEREO);
    if (audio_channel < 0)
    {
        printf("Failed to reserve audio channel.\n");
        return;
    }

	// Everything OK
	audio_open = true;
}

void AudioPlugin::AddBuffer( u8 *start, u32 length )
{
	if (length == 0)
		return;

	if (!mKeepRunning)
		StartAudio();

	u32 num_samples = length / sizeof( Sample );

	switch( GetMode() )
	{
	case APM_DISABLED:
		break;

	case APM_ENABLED_ASYNC:
	case APM_ENABLED_SYNC:
		{
      // Output buffer to audio channel
	  int volume = 0x8000; // Max volume
	  int result = sceAudioOutput(audio_channel, volume, start);
			// mAudioBufferUncached->AddSamples( reinterpret_cast< const Sample * >( start ), num_samples, mFrequency, kOutputFrequency );
		}
		break;
	}

	/*
	u32 remaining_samples = mAudioBuffer.GetNumBufferedSamples();
	mBufferLenMs = (1000 * remaining_samples) / kOutputFrequency);
	float ms = (float) num_samples * 1000.f / (float)mFrequency;
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DPF_AUDIO("Queuing %d samples @%dHz - %.2fms - bufferlen now %d\n", num_samples, mFrequency, ms, mBufferLenMs);
	#endif
	*/
}

void AudioPlugin::StopAudio()
{
	if (!mKeepRunning)
		return;

		if (audio_channel >= 0)
		{
			sceAudioChRelease(audio_channel);
			audio_channel = -1;
		}
		#ifdef DAEDALUS_PSP_USE_ME
		mei.reset(); 
		#endif
	mKeepRunning = false;

	audio_open = false;
}
