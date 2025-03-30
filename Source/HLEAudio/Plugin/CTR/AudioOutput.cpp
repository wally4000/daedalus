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


#include <stdio.h>
#include <new>

#include <3ds.h>
#include <cstring>

#include "Utility/FramerateLimiter.h"
#include "Interface/ConfigOptions.h"
#include "Debug/DBGConsole.h"
#include "HLEAudio/AudioBuffer.h"

#include "System/Thread.h"

extern u32 gSoundSync;

static const u32 DESIRED_OUTPUT_FREQUENCY = 44100;

static const u32 CTR_BUFFER_SIZE  = 1024 * 2;
static const u32 CTR_BUFFER_COUNT = 6;
static const u32 CTR_NUM_SAMPLES  = 512;


static ndspWaveBuf waveBuf[CTR_BUFFER_COUNT];
static unsigned int waveBuf_id;

bool audioOpen = false;

