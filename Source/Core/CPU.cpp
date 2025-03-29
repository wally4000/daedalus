/*
Copyright (C) 2001-2007 StrmnNrmn

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


// Stuff to handle Processor

#include "Base/Types.h"
#include "Core/CPU.h"

#include <algorithm>
#include <string>
#include <vector>
#include <cstring>


#include "Interface/ConfigOptions.h"
#include "Interface/Cheats.h"
#include "Core/Dynamo.h"
#include "Core/Interpret.h"
#include "Core/Interrupt.h"
#include "Core/Memory.h"
#include "Core/R4300.h"
#include "Debug/Registers.h"					// For REG_?? defines
#include "Core/ROM.h"
#include "RomFile/ROMBuffer.h"
#include "Core/RSP_HLE.h"
#include "Core/Save.h"
#include "Interface/SaveState.h"
#include "Debug/DBGConsole.h"
#include "Debug/DebugLog.h"
#include "Ultra/ultra_R4300.h"
#include "System/SystemInit.h"
#include "System/AtomicPrimitives.h"
#include "Utility/FramerateLimiter.h"
#include "Utility/Hash.h"
#include "Base/Macros.h"
#include "Debug/PrintOpCode.h"
#include "Debug/Synchroniser.h"
#include "System/Thread.h"
#include "System/SpinLock.h"


extern void R4300_Init();


// Take advantage of the cooperative multitasking
// of the PSP to make locking/unlocking as fast as possible.



//
//	New dynarec engine
//
#ifdef DAEDALUS_PROFILE_EXECUTION
u64					gTotalInstructionsExecuted = 0;
u64					gTotalInstructionsEmulated = 0;
#endif

#ifdef DAEDALUS_BREAKPOINTS_ENABLED
std::vector< DBG_BreakPoint > g_BreakPoints;
#endif

static bool			gCPURunning   = false;			// CPU is actively running
u8 *				gLastAddress     = nullptr;


static bool			gCPUStopOnSimpleState = false;			// When stopping, try to stop in a 'simple' state (i.e. no RSP running and not in a branch delay slot)


enum ESaveStateOperation
{
	SSO_NONE,
	SSO_SAVE,
	SSO_LOAD,
};

static ESaveStateOperation		gSaveStateOperation(SSO_NONE);

const  u32			kInitialVIInterruptCycles = 62500;
static u32			gVerticalInterrupts = 0;
static u32			VI_INTR_CYCLES(kInitialVIInterruptCycles);




static bool	CPU_IsStateSimple();
void (* g_pCPUCore)();

using VblCallbackFn = void (*)(void * arg);

struct VblCallback
{
	VblCallbackFn		Fn;
	void *				Arg;
};

static std::vector<VblCallback>		gVblCallbacks;


void CPU_RegisterVblCallback(VblCallbackFn fn, void * arg)
{
	
	VblCallback callback = { fn, arg };
	gVblCallbacks.push_back(callback);
}

void CPU_UnregisterVblCallback(VblCallbackFn fn, void * arg)
{
	for (auto it = gVblCallbacks.begin(); it != gVblCallbacks.end(); ++it)
	{
		if (it->Fn == fn && it->Arg == arg)
		{
			gVblCallbacks.erase(it);
			break;
		}
	}
}


void CPU_SkipToNextEvent()
{
	LOCK_EVENT_QUEUE();

#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( ctx.cpuState.NumEvents > 0, "There are no events" );
	#endif
    ctx.cpuState.CPUControl[C0_COUNT]._u32 += (ctx.cpuState.Events[ 0 ].mCount - 1);
    ctx.cpuState.Events[ 0 ].mCount = 1;
}

static void CPU_ResetEventList()
{
	ctx.cpuState.Events[ 0 ].mCount     = kInitialVIInterruptCycles;
	ctx.cpuState.Events[ 0 ].mEventType = CPU_EVENT_VBL;
	ctx.cpuState.NumEvents = 1;

	RESET_EVENT_QUEUE_LOCK();
}

void CPU_AddEvent( s32 count, ECPUEventType event_type )
{
	LOCK_EVENT_QUEUE();

#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( count > 0, "Count is invalid" );
	DAEDALUS_ASSERT( ctx.cpuState.NumEvents < MAX_CPU_EVENTS, "Too many events" );
#endif

	u32 event_idx = 0;
	for( ; event_idx < ctx.cpuState.NumEvents; ++event_idx )
	{
		CPUEvent & event = ctx.cpuState.Events[ event_idx ];

		if( count <= event.mCount )
		{
			//
			//	This event belongs before the subsequent one so insert a space for it here and break out
			//	Don't forget to decrement the counter for the subsequent event
			//
			event.mCount -= count;


			if (event_idx < ctx.cpuState.NumEvents )
			{
				std::memmove( &ctx.cpuState.Events[ event_idx+1 ], &ctx.cpuState.Events[ event_idx ], (ctx.cpuState.NumEvents - event_idx) * sizeof( CPUEvent ) );
			}
			break;
		}

		//	Decrease counter by that for this event
		count -= event.mCount;
	}
#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( event_idx <= ctx.cpuState.NumEvents, "Invalid idx" );
	#endif
	ctx.cpuState.Events[ event_idx ].mCount = count;
	ctx.cpuState.Events[ event_idx ].mEventType = event_type;
	ctx.cpuState.NumEvents++;
}

static void CPU_SetCompareEvent( s32 count )
{
	{
		LOCK_EVENT_QUEUE();

#ifdef DAEDALUS_ENABLE_ASSERTS
		DAEDALUS_ASSERT( count > 0, "Count is invalid" );
#endif
		//
		//	Remove any existing compare events. Need to adjust any subsequent timer's count.
		//
		for( u32 i = 0; i < ctx.cpuState.NumEvents; ++i )
		{
			if( ctx.cpuState.Events[ i ].mEventType == CPU_EVENT_COMPARE )
			{
				//
				//	Check for a following event, and remove
				//
				if( i+1 < ctx.cpuState.NumEvents )
				{
					ctx.cpuState.Events[ i+1 ].mCount += ctx.cpuState.Events[ i ].mCount;
					u32 num_to_copy = ctx.cpuState.NumEvents - (i+1);
					memmove( &ctx.cpuState.Events[ i ], &ctx.cpuState.Events[ i+1 ], num_to_copy * sizeof( CPUEvent ) );
				}
				ctx.cpuState.NumEvents--;
				break;
			}
		}
	}

	CPU_AddEvent( count, CPU_EVENT_COMPARE );
}

static ECPUEventType CPU_PopEvent()
{
	LOCK_EVENT_QUEUE();

#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( ctx.cpuState.NumEvents > 0, "Event queue empty" );
	DAEDALUS_ASSERT( ctx.cpuState.Events[ 0 ].mCount <= 0, "Popping event when cycles remain" );
	//DAEDALUS_ASSERT( ctx.cpuState.Events[ 0 ].mCount == 0, "Popping event with a bit of underflow" );
#endif

	ECPUEventType event_type = ctx.cpuState.Events[ 0 ].mEventType;

	if( ctx.cpuState.NumEvents > 1 )
	{
		std::memcpy( &ctx.cpuState.Events[ 0 ], &ctx.cpuState.Events[ 1 ], (ctx.cpuState.NumEvents -1) * sizeof( CPUEvent ) );
	}
	ctx.cpuState.NumEvents--;

	return event_type;
}

// XXXX This is for savestate. Looks very suspicious to me
u32 CPU_GetVideoInterruptEventCount()
{
	for( u32 i = 0; i < ctx.cpuState.NumEvents; ++i )
	{
		if(ctx.cpuState.Events[ i ].mEventType == CPU_EVENT_VBL)
		{
			return ctx.cpuState.Events[ i ].mCount;
		}
	}

	return 0;
}

// XXXX This is for savestate. Looks very suspicious to me
void CPU_SetVideoInterruptEventCount( u32 count )
{
	for( u32 i = 0; i < ctx.cpuState.NumEvents; ++i )
	{
		if(ctx.cpuState.Events[ i ].mEventType == CPU_EVENT_VBL)
		{
			ctx.cpuState.Events[ i ].mCount = count;
			return;
		}
	}
}

void SCPUState::ClearStuffToDo()
{
	StuffToDo = 0;
	Dynarec_ClearedCPUStuffToDo();
}

void SCPUState::AddJob( u32 job )
{
	u32 stuff =  AtomicBitSet( &StuffToDo, 0xffffffff, job );
	if( stuff != 0 )
	{
		Dynarec_SetCPUStuffToDo();
	}
}

void SCPUState::ClearJob( u32 job )
{
	u32 stuff( AtomicBitSet( &StuffToDo, ~job, 0x00000000 ) );
	if( stuff == 0 )
	{
		Dynarec_ClearedCPUStuffToDo();
	}
}

#ifdef DAEDALUS_ENABLE_SYNCHRONISATION
static const char * const kRegisterNames[] =
{
	"zr", "at", "v0", "v1", "a0", "a1", "a2", "a3",
	"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
	"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
	"t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};
DAEDALUS_STATIC_ASSERT(std::size(kRegisterNames) == 32);

void SCPUState::Dump()
{

	DBGConsole_Msg(0, "Emulation CPU State:");
	{
		for(int i = 0; i < 32; i += 4)
		{
			DBGConsole_Msg(0, "%s:%08X %s:%08X %s:%08X %s:%08X",
			kRegisterNames[i+0], ctx.cpuState.CPU[i+0]._u32_0,
			kRegisterNames[i+1], ctx.cpuState.CPU[i+1]._u32_0,
			kRegisterNames[i+2], ctx.cpuState.CPU[i+2]._u32_0,
			kRegisterNames[i+3], ctx.cpuState.CPU[i+3]._u32_0);
		}

		DBGConsole_Msg(0, "TargetPC: %08x", ctx.cpuState.TargetPC);
		DBGConsole_Msg(0, "CurrentPC: %08x", ctx.cpuState.CurrentPC);
		DBGConsole_Msg(0, "Delay: %08x", ctx.cpuState.Delay);
	}
}
#endif

bool CPU_RomOpen()
{
	#ifdef DAEDALUS_DEBUG_CONSOLE
	DBGConsole_Msg(0, "Resetting CPU");
#endif

	gLastAddress = nullptr;
	gCPURunning = false;
	gCPUStopOnSimpleState = false;
	RESET_EVENT_QUEUE_LOCK();

	memset(&ctx.cpuState, 0, sizeof(ctx.cpuState));

	CPU_SetPC( 0xbfc00000 );
	ctx.cpuState.MultHi._u64 = 0;
	ctx.cpuState.MultLo._u64 = 0;

	for(u32 i = 0; i < 32; i++)
	{
		ctx.cpuState.CPU[i]._u64        = 0;
		ctx.cpuState.CPUControl[i]._u32 = 0;
		ctx.cpuState.FPU[i]._u32        = 0;
		ctx.cpuState.FPUControl[i]._u32 = 0;
	}

	// Init TLBs:
	for (u32 i = 0; i < 32; i++)
	{
		g_TLBs[i].Reset();
	}

	// From R4300 manual
	ctx.cpuState.CPUControl[C0_RAND]._u32   = 32-1;			// TLBENTRIES-1
	//ctx.cpuState.CPUControl[C0_SR]._u32   = 0x70400004;	//*SR_FR |*/ SR_ERL | SR_CU2|SR_CU1|SR_CU0;
	R4300_SetSR(0x70400004);
	ctx.cpuState.CPUControl[C0_PRID]._u32   = 0x00000b10;	// Was 0xb00 - test rom reports 0xb10!!
	ctx.cpuState.CPUControl[C0_CONFIG]._u32 = 0x0006E463;	// 0x00066463;
	ctx.cpuState.CPUControl[C0_WIRED]._u32  = 0x0;

	ctx.cpuState.FPUControl[0]._u32 = 0x00000511;

	Memory_MI_SetRegister(MI_VERSION_REG, 0x02020102);

	((u32 *)g_pMemoryBuffers[MEM_RI_REG])[3] = 1;					// RI_CONFIG_REG Skips most of init

	R4300_Init();

	ctx.cpuState.Delay = NO_DELAY;
	ctx.cpuState.ClearStuffToDo();
	gVerticalInterrupts = 0;

	// Clear event list:
	CPU_ResetEventList();

#ifdef DAEDALUS_BREAKPOINTS_ENABLED
	g_BreakPoints.clear();
#endif

	Dynamo_Reset();

	CPU_SelectCore();
	return true;
}

void CPU_RomClose()
{
#ifdef DAEDALUS_ENABLE_DYNAREC
	#ifdef DAEDALUS_DEBUG_CONSOLE_DYNAREC
		//This will dump the fragment cache on exit to ROMs menu
		//CPU_DumpFragmentCache();
	#endif
#endif
}

static bool	CPU_IsStateSimple()
{
	bool rsp_halted = !RSP_IsRunning();

	return rsp_halted && (ctx.cpuState.Delay == NO_DELAY);
}

void CPU_SelectCore()
{
#ifdef DAEDALUS_ENABLE_DYNAREC
	if (gDynarecEnabled)
		Dynamo_SelectCore();
	else
#endif
		Inter_SelectCore();

	if( gCPUStopOnSimpleState && CPU_IsStateSimple() )
	{
		ctx.cpuState.AddJob( CPU_STOP_RUNNING );
	}
	else
	{
		ctx.cpuState.AddJob( CPU_CHANGE_CORE );
	}
}

bool CPU_RequestSaveState( const std::filesystem::path &filename )
{
	// Call SaveState_SaveToFile directly if the CPU is not running.
	DAEDALUS_ASSERT(gCPURunning, "Expecting the CPU to be running at this point");
	LOCK_EVENT_QUEUE();

	// Abort if already in the process of loading/saving
	if( gSaveStateOperation != SSO_NONE )
	{
		return false;
	}

	gSaveStateOperation = SSO_SAVE;
	ctx.saveStateFilename = filename.string();
	ctx.cpuState.AddJob(CPU_CHANGE_CORE);

	return true;
}

bool CPU_RequestLoadState( const std::filesystem::path &filename )
{
	// Call SaveState_SaveToFile directly if the CPU is not running.
	DAEDALUS_ASSERT(gCPURunning, "Expecting the CPU to be running at this point");
	LOCK_EVENT_QUEUE();

	// Abort if already in the process of loading/saving
	if( gSaveStateOperation != SSO_NONE )
	{
		return false;
	}

	gSaveStateOperation = SSO_LOAD;
	ctx.saveStateFilename = filename.string();
	ctx.cpuState.AddJob(CPU_CHANGE_CORE);

	return true;	// XXXX could fail
}

static void HandleSaveStateOperationOnVerticalBlank()
{
	DAEDALUS_ASSERT(gCPURunning, "Expecting the CPU to be running at this point");
	if( gSaveStateOperation == SSO_NONE )
		return;
		LOCK_EVENT_QUEUE();

	//
	// Handle the save state
	//
	switch( gSaveStateOperation )
	{
	case SSO_NONE:
		DAEDALUS_ERROR( "Unreachable" );
		break;
	case SSO_SAVE:
		DBGConsole_Msg(0, "Saving '%s'\n", ctx.saveStateFilename.c_str());
		SaveState_SaveToFile( ctx.saveStateFilename );
		gSaveStateOperation = SSO_NONE;
		break;
	case SSO_LOAD:
		DBGConsole_Msg(0, "Loading '%s'\n", ctx.saveStateFilename.c_str());
		// Try to load the savestate immediately. If this fails, it
		// usually means that we're running the correct rom (we should have a
		// separate return code to check this case). In that case we
		// stop the cpu and handle the load in
		// HandleSaveStateOperationOnCPUStopRunning.
		if (SaveState_LoadFromFile( ctx.saveStateFilename ))
		{
			CPU_ResetFragmentCache();
			gSaveStateOperation = SSO_NONE;
		}
		else
		{
			// Halt the CPU so that we can swap the rom safely and load the savesate.
			CPU_Halt("Load SaveSate");
			// NB: return without clearing gSaveStateOperation
		}
		break;
	}
}

// Returns true if we handled a load request and should keep running.
static bool HandleSaveStateOperationOnCPUStopRunning()
{
	if (gSaveStateOperation != SSO_LOAD)
		return false;

		LOCK_EVENT_QUEUE();

	gSaveStateOperation = SSO_NONE;

	std::filesystem::path rom_filename = SaveState_GetRom(ctx.saveStateFilename);
	if (!rom_filename.empty())
	{
		System_Close();
		System_Open(rom_filename);
		SaveState_LoadFromFile(ctx.saveStateFilename);
	}
	else
	{
		DBGConsole_Msg(0, "Couldn't find matching rom for %s\n", ctx.saveStateFilename.c_str());
		// Keep running with the current rom.
	}

	return true;
}

bool CPU_Run()
{
	if (!RomBuffer::IsRomLoaded())
		return false;

	while (1)
	{
		gCPURunning = true;
		gCPUStopOnSimpleState = false;
		DAEDALUS_ASSERT(gSaveStateOperation == SSO_NONE, "Shouldn't have a save state operation queued.");
		RESET_EVENT_QUEUE_LOCK();

		while (gCPURunning)
		{
			g_pCPUCore();
		}

		if (!HandleSaveStateOperationOnCPUStopRunning())
			break;
	}
	DAEDALUS_ASSERT(!gCPURunning, "gCPURunning should be false by now.");
	return true;
}

void CPU_Halt( const char * reason )
{
	DBGConsole_Msg( 0, "CPU Halting: %s", reason );
	gCPUStopOnSimpleState = true;
	ctx.cpuState.AddJob( CPU_STOP_RUNNING );
}

#ifdef DAEDALUS_BREAKPOINTS_ENABLED
void CPU_AddBreakPoint( u32 address )
{
	OpCode * pdwOp;

	// Force 4 byte alignment
	address &= 0xFFFFFFFC;

	if (!Memory_GetInternalReadAddress(address, (void**)&pdwOp))
	{
		DBGConsole_Msg(0, "Invalid Address for BreakPoint: 0x%08x", address);
	}
	else
	{
		DBG_BreakPoint bpt;
		DBGConsole_Msg(0, "[YInserting BreakPoint at 0x%08x]", address);

		bpt.mOriginalOp       = *pdwOp;
		bpt.mEnabled          = true;
		bpt.mTemporaryDisable = false;
		g_BreakPoints.push_back(bpt);

		pdwOp->op       = OP_DBG_BKPT;
		pdwOp->bp_index = (g_BreakPoints.size() - 1);
	}
}
#endif

#ifdef DAEDALUS_BREAKPOINTS_ENABLED
void CPU_EnableBreakPoint( u32 address, bool enable )
{
	OpCode * pdwOp;

	// Force 4 byte alignment
	address &= 0xFFFFFFFC;

	if (!Memory_GetInternalReadAddress(address, (void**)&pdwOp))
	{
		DBGConsole_Msg(0, "Invalid Address for BreakPoint: 0x%08x", address);
	}
	else
	{
		OpCode op_code = *pdwOp;

		if (op_code.op != OP_DBG_BKPT)
		{
			DBGConsole_Msg(0, "[YNo breakpoint is set at 0x%08x]", address);
			return;
		}

		// Entry is in lower 26 bits...
		u32 breakpoint_idx = op_code.bp_index;

		if (breakpoint_idx < g_BreakPoints.size())
		{
			g_BreakPoints[breakpoint_idx].mEnabled = enable;
			// Alwyas disable
			g_BreakPoints[breakpoint_idx].mTemporaryDisable = false;
		}
	}
}
#endif

extern u32 gVISyncRate ;
extern "C"
{
void CPU_HANDLE_COUNT_INTERRUPT()
{
	#ifdef DAEDALUS_ENABLE_ASSERTS
		DAEDALUS_ASSERT( ctx.cpuState.NumEvents > 0, "Should always have at least one event queued up" );
	#endif
	switch (CPU_PopEvent())
	{
	case CPU_EVENT_VBL:
		{
			//Todo: Work on VI_INTR_CYCLES should be 62500 * (60/Real game FPS)
			u32 vertical_sync_reg = Memory_VI_GetRegister( VI_V_SYNC_REG );
			if (vertical_sync_reg == 0)
			{
				VI_INTR_CYCLES = 62500;
			}
			else
			{
				VI_INTR_CYCLES = (vertical_sync_reg+1) * ( gVideoRateMatch ? gVISyncRate : 1500 );
			}

			// Apply cheatcodes, if enabled
			if( gCheatsEnabled )
			{
				CheatCodes_Activate( IN_GAME );
			}

			// Add another Interrupt at the next time:
			CPU_AddEvent(VI_INTR_CYCLES, CPU_EVENT_VBL);

			gVerticalInterrupts++;

			FramerateLimiter_Limit();

			Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_VI);
			R4300_Interrupt_UpdateCause3();

			// ToDo: Has to be a better way than this???
			// Maybe After each X frames instead of each 60 VI?
			// (strmnnrmn): I don't see what's so bad about checking these on a vbl,
			//   because it means we can remain responsive even if the game is not rendering frames
			//   (e.g. if it's slow starting up)
			//   Alternatively, we could add a special-purpose CPU even that triggers every
			//   N cycles, but that would have a small impact on framerate (it would
			//   interrupt the dynamo tracer for instance)
			// TODO(strmnnrmn): should register this with CPU_RegisterVblCallback.
			 if ((gVerticalInterrupts & 0x3F) == 0) { // once every 60 VBLs
				Save_Flush();
				for (size_t i = 0; i < gVblCallbacks.size(); ++i)
				{
					VblCallback & callback = gVblCallbacks[i];
					callback.Fn(callback.Arg);
				}

			}
			HandleSaveStateOperationOnVerticalBlank();
		}
		break;
	case CPU_EVENT_COMPARE:
		{
			ctx.cpuState.CPUControl[C0_CAUSE]._u32 |= CAUSE_IP8;
			ctx.cpuState.AddJob( CPU_CHECK_INTERRUPTS );
		}
		break;
	case CPU_EVENT_AUDIO:
		{
			u32 status = Memory_SP_SetRegisterBits(SP_STATUS_REG, SP_STATUS_TASKDONE|SP_STATUS_YIELDED|SP_STATUS_BROKE|SP_STATUS_HALT);
			if( status & SP_STATUS_INTR_BREAK )
				CPU_AddEvent(4000, CPU_EVENT_SPINT);
		}
		break;
	case CPU_EVENT_SPINT:
		Memory_MI_SetRegisterBits(MI_INTR_REG, MI_INTR_SP);
		R4300_Interrupt_UpdateCause3();
		break;
	}

	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( ctx.cpuState.NumEvents > 0, "Should always have at least one event queued up" );
	#endif
}
}

void CPU_SetCompare(u32 value)
{
	ctx.cpuState.CPUControl[C0_CAUSE]._u32 &= ~CAUSE_IP8;

	#ifdef DAEDALUS_PROFILE
	DPF( DEBUG_REGS, "COMPARE set to 0x%08x.", value );
	//DBGConsole_Msg(0, "COMPARE set to 0x%08x Count is 0x%08x.", value, ctx.cpuState.CPUControl[C0_COUNT]._u32);
	#endif
	// Add an event for this compare:
	if (value == ctx.cpuState.CPUControl[C0_COMPARE]._u32)
	{
		//DBGConsole_Msg(0, "Clear");
	}
	else
	{
		if (value != 0)
		{
			// NB, value can be less than COUNT here, which indicates that the counter is close to wrapping.
			// Don't do anything special to handle this - just treat delta as an unsigned value.
			u32 delta = value - ctx.cpuState.CPUControl[C0_COUNT]._u32;

			// This fires a lot for Zelda OoT. It's benign.
			// If seems to keep setting a delta of 140624981 when the counter is close to wrapping.
			// if (value < ctx.cpuState.CPUControl[C0_COUNT]._u32)
			// {
			// 	DBGConsole_Msg(0, "SetCompare wrapping: %d -> %d = %d", ctx.cpuState.CPUControl[C0_COUNT]._u32, value, delta);
			// }
			CPU_SetCompareEvent( delta );
		}
		#ifdef DAEDALUS_DEBUG_CONSOLE
		else
		{
			//DBGConsole_Msg(0, "[RIgnoring SetCompare 0] - is this right?");
		}
		#endif
		ctx.cpuState.CPUControl[C0_COMPARE]._u32 = value;
	}
}

#ifdef DAEDALUS_ENABLE_SYNCHRONISATION
u32	CPU_ProduceRegisterHash()
{
	u32	hash = 0;

	if ( DAED_SYNC_MASK & DAED_SYNC_REG_GPR )
	{
		hash = murmur2_hash( (u8*)&(ctx.cpuState.CPU[0]), sizeof( ctx.cpuState.CPU ), hash );
	}

	if ( DAED_SYNC_MASK & DAED_SYNC_REG_CCR0 )
	{
		hash = murmur2_hash( (u8*)&(ctx.cpuState.CPUControl[0]), sizeof( ctx.cpuState.CPUControl ), hash );
	}

	if ( DAED_SYNC_MASK & DAED_SYNC_REG_CPU1 )
	{
		hash = murmur2_hash( (u8*)&(ctx.cpuState.FPU[0]), sizeof( ctx.cpuState.FPU ), hash );
	}
	if ( DAED_SYNC_MASK & DAED_SYNC_REG_CCR1 )
	{
		hash = murmur2_hash( (u8*)&(ctx.cpuState.FPUControl[0]), sizeof( ctx.cpuState.FPUControl ), hash );
	}

	return hash;
}
#endif

#ifdef FRAGMENT_SIMULATE_EXECUTION
// Execute the specified opcode
void CPU_ExecuteOpRaw( u32 count, u32 address, OpCode op_code, CPU_Instruction p_instruction, bool * p_branch_taken )
{
	ctx.cpuState.CurrentPC = address;

	SYNCH_POINT( DAED_SYNC_REG_PC, ctx.cpuState.CurrentPC, "Program Counter doesn't match" );
	SYNCH_POINT( DAED_SYNC_REG_PC, count, "Count doesn't match" );

	p_instruction( op_code._u32 );

	SYNCH_POINT( DAED_SYNC_REGS, CPU_ProduceRegisterHash(), "Registers don't match" );

	*p_branch_taken = ctx.cpuState.Delay == DO_DELAY;
}
#endif

extern "C"
{
void  CPU_UpdateCounter( u32 ops_executed )
{
	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( ops_executed > 0, "Expecting at least one op" );
	#endif
	//SYNCH_POINT( DAED_SYNC_FRAGMENT_PC, ops_executed, "Number of executed ops doesn't match" );

#ifdef DAEDALUS_PROFILE_EXECUTION
	gTotalInstructionsExecuted += ops_executed;
#endif

	const u32 cycles = ops_executed * COUNTER_INCREMENT_PER_OP;

	// Increment count register
	ctx.cpuState.CPUControl[C0_COUNT]._u32 += cycles;

	if( CPU_ProcessEventCycles( cycles ) )
	{
		CPU_HANDLE_COUNT_INTERRUPT();
	}
}

#ifdef UPDATE_COUNTER_ON_EXCEPTION
// As above, but no interrupts are fired
void CPU_UpdateCounterNoInterrupt( u32 ops_executed )
{
	//SYNCH_POINT( DAED_SYNC_FRAGMENT_PC, ops_executed, "Number of executed ops doesn't match" );

	if( ops_executed > 0 )
	{
		const u32 cycles  =ops_executed * COUNTER_INCREMENT_PER_OP};

#ifdef DAEDALUS_PROFILE_EXECUTION
		gTotalInstructionsExecuted += ops_executed;
#endif

		// Increment count register
		ctx.cpuState.CPUControl[C0_COUNT]._u32 += cycles;

#ifdef DAEDALUS_ENABLE_ASSERTS
		bool	ready = CPU_ProcessEventCycles( cycles );
		use( ready );
		DAEDALUS_ASSERT(!ready, "Ignoring Count interrupt");	// Just a test - remove eventually (needs to handle this)
#endif
	}
}
#endif
}

// Return true if change the core
bool CPU_CheckStuffToDo()
{
    // Get jobs to do
    u32 stuff_to_do = ctx.cpuState.GetStuffToDo();

    // Early return if nothing to do
    if (stuff_to_do == 0) {
        return false;
    }

    // Process Interrupts/Exceptions on a priority basis using a switch statement
		if( ctx.cpuState.GetStuffToDo() & CPU_CHECK_INTERRUPTS )
		{
			R4300_Handle_Interrupt();
			ctx.cpuState.ClearJob( CPU_CHECK_INTERRUPTS );
		}
		else if( ctx.cpuState.GetStuffToDo() & CPU_CHECK_EXCEPTIONS )
		{
			R4300_Handle_Exception();
			ctx.cpuState.ClearJob( CPU_CHECK_EXCEPTIONS );
		}
		else if( ctx.cpuState.GetStuffToDo() & CPU_CHANGE_CORE )
		{
			ctx.cpuState.ClearJob( CPU_CHANGE_CORE );
			return true;
		}
		else if( ctx.cpuState.GetStuffToDo() & CPU_STOP_RUNNING )
		{
			ctx.cpuState.ClearJob( CPU_STOP_RUNNING );
			gCPURunning = false;
			return true;
		}
		// Clear stuff_to_do?

	return false;
}

// FIX ME: This gets called alot
// Return true if the CPU is running
bool CPU_IsRunning()	
{	
	return gCPURunning;	
}

