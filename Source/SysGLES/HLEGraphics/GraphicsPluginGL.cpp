
#include "Base/Types.h"

#include <stdio.h>

#include "Core/Memory.h"

#include "Debug/DBGConsole.h"

#include "Graphics/GraphicsContext.h"

#include "HLEGraphics/BaseRenderer.h"
#include "HLEGraphics/TextureCache.h"
#include "HLEGraphics/DLParser.h"
#include "HLEGraphics/DisplayListDebugger.h"

#include "HLEGraphics/GraphicsPlugin.h"

#include "System/Timing.h"

#include "SysGLES/GL.h"

extern SDL_Window * gWindow;

EFrameskipValue     gFrameskipValue = FV_DISABLED;
u32                 gVISyncRate     = 1500;
bool                gTakeScreenshot = false;
bool				gTakeScreenshotSS = false;

namespace
{
	//u32					gVblCount = 0;
	u32					gFlipCount = 0;
	//float				gCurrentVblrate = 0.0f;
	float				gCurrentFramerate = 0.0f;
	u64					gLastFramerateCalcTime = 0;
	u64					gTicksPerSecond = 0;

#ifdef DAEDALUS_FRAMERATE_ANALYSIS
	u32					gTotalFrames = 0;
	u64					gFirstFrameTime = 0;
	FILE *				gFramerateFile = NULL;
#endif

static void	UpdateFramerate()
{
#ifdef DAEDALUS_FRAMERATE_ANALYSIS
	gTotalFrames++;
#endif
	gFlipCount++;

	u64			now;
	NTiming::GetPreciseTime( &now );

	if(gLastFramerateCalcTime == 0)
	{
		u64		freq;
		gLastFramerateCalcTime = now;

		NTiming::GetPreciseFrequency( &freq );
		gTicksPerSecond = freq;
	}

#ifdef DAEDALUS_FRAMERATE_ANALYSIS
	if( gFramerateFile == NULL )
	{
		gFirstFrameTime = now;
		gFramerateFile = fopen( "framerate.csv", "w" );
	}
	fprintf( gFramerateFile, "%d,%f\n", gTotalFrames, f32(now - gFirstFrameTime) / f32(gTicksPerSecond) );
#endif

	// If 1 second has elapsed since last recalculation, do it now
	u64		ticks_since_recalc( now - gLastFramerateCalcTime );
	if(ticks_since_recalc > gTicksPerSecond)
	{
		//gCurrentVblrate = float( gVblCount * gTicksPerSecond ) / float( ticks_since_recalc );
		gCurrentFramerate = float( gFlipCount * gTicksPerSecond ) / float( ticks_since_recalc );

		//gVblCount = 0;
		gFlipCount = 0;
		gLastFramerateCalcTime = now;

#ifdef DAEDALUS_FRAMERATE_ANALYSIS
		if( gFramerateFile != NULL )
		{
			fflush( gFramerateFile );
		}
#endif
	}

}
}



CGraphicsPlugin::CGraphicsPlugin()
:	LastOrigin( 0 )
{
}

CGraphicsPlugin::~CGraphicsPlugin() {}


bool ShowFPSonmenu = false;


void CGraphicsPlugin::UpdateScreen()
{
	u32 current_origin = Memory_VI_GetRegister(VI_ORIGIN_REG);

	if (current_origin != LastOrigin)
	{
		UpdateFramerate();

		if (gTakeScreenshot)
		{
			ctx.graphicsContext->DumpNextScreen();
			gTakeScreenshot = false;
		}

		ctx.graphicsContext->UpdateFrame( false );

		LastOrigin = current_origin;
	}
}

