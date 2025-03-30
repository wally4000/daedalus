/*
Copyright (C) 2007 StrmnNrmn

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

#ifndef _AUDIOPLUGINCTR_H_
#define _AUDIOPLUGINCTR_H_

#include "HLEAudio/AudioPlugin.h"

class AudioOutput;

class AudioPlugin : public CAudioPlugin
{
private:

public:
AudioPlugin();

	virtual ~AudioPlugin();
	virtual bool			StartEmulation();
	virtual void			StopEmulation();

	virtual void			DacrateChanged( int SystemType );
	virtual void			LenChanged();
	virtual u32				ReadLength();
	virtual EProcessResult	ProcessAList();
	virtual void SetMode(EAudioPluginMode mode) override;
	virtual EAudioPluginMode GetMode() const override;



public:
	void				FillBuffer( Sample * buffer, u32 num_samples );
	void				SetFrequency( u32 frequency );			// Sets the Nintendo64 Game Audio Frequency
	void				AddBuffer( u8 * start, u32 length );	// Uploads a new buffer

	// Management functions
	void				StopAudio( );						// Stops the Audio PlayBack (as if paused)
	void				StartAudio( );						// Starts the Audio PlayBack (as if unpaused)
	CAudioBuffer * mAudioBuffer;
private:
	bool				mAudioPlaying;
	bool				mExitAudioThread;
	u32					mFrequency;
//			void			SetAdaptFrequecy( bool adapt );

private:


	EAudioPluginMode audioPluginmode = APM_DISABLED;
};


#endif // _AUDIOPLUGINCTR_H_

