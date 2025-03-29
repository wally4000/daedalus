/*
Copyright (C) 2001 StrmnNrmn

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

#ifndef CORE_CPU_H_
#define CORE_CPU_H_

#include <stdlib.h>
#include <stdalign.h>
#include "Core/Memory.h"
#include "Core/R4300Instruction.h"
#include "Core/R4300OpCode.h"
#include "Core/TLB.h"
#include "System/SpinLock.h"
#include "Utility/Paths.h"
#include "System/SystemInit.h"
#include "Core/CPUState.h"


#define LOCK_EVENT_QUEUE() CSpinLock _lock( &ctx.eventQueueLocked )
#define RESET_EVENT_QUEUE_LOCK() ctx.eventQueueLocked = 0;

enum EDelayType
{
	NO_DELAY = 0,
	EXEC_DELAY,
	DO_DELAY
};

#ifdef DAEDALUS_BREAKPOINTS_ENABLED
struct DBG_BreakPoint
{
	OpCode mOriginalOp;
	bool  mEnabled;
	bool  mTemporaryDisable;		// Set to true when BP is activated - this lets us type
									// go immediately after a bp and execution will resume.
									// otherwise it would keep activating the bp
									// The patch bp r4300 instruction clears this
									// when it is next executed
};
#endif
//
// CPU Jobs
//
#define CPU_CHECK_EXCEPTIONS				0x00000001
#define CPU_CHECK_INTERRUPTS				0x00000002
#define CPU_STOP_RUNNING					0x00000008
#define CPU_CHANGE_CORE						0x00000010

//*****************************************************************************
// External declarations
//*****************************************************************************
#ifdef DAEDALUS_BREAKPOINTS_ENABLED
#include <vector>
extern std::vector< DBG_BreakPoint > g_BreakPoints;
#endif

#define gGPR (ctx.cpuState.CPU)
//*****************************************************************************
//
//*****************************************************************************
bool	CPU_RomOpen();
void	CPU_RomClose();
void	CPU_Step();
void	CPU_Skip();
bool	CPU_Run();
bool	CPU_RequestSaveState( const std::filesystem::path &filename );
bool	CPU_RequestLoadState( const std::filesystem::path &filename );
void	CPU_Halt( const char * reason );
void	CPU_SelectCore();
u32		CPU_GetVideoInterruptEventCount();
void	CPU_SetVideoInterruptEventCount( u32 count );
void	CPU_DynarecEnable();
void	 CPU_InvalidateICacheRange( u32 address, u32 length );
void	 CPU_InvalidateICache();
void	CPU_SetCompare(u32 value);
#ifdef DAEDALUS_BREAKPOINTS_ENABLED
void	CPU_AddBreakPoint( u32 address );						// Add a break point at address dwAddress
void	CPU_EnableBreakPoint( u32 address, bool enable );		// Enable/Disable the breakpoint as the specified address
#endif
bool	CPU_IsRunning();
void	CPU_AddEvent( s32 count, ECPUEventType event_type );
void	CPU_SkipToNextEvent();
bool	CPU_CheckStuffToDo();

using VblCallbackFn = void(*) (void * arg);

void CPU_RegisterVblCallback(VblCallbackFn fn, void * arg);
void CPU_UnregisterVblCallback(VblCallbackFn fn, void * arg);

// For PSP, we just keep running forever. For other platforms we need to bail when the user quits.
#ifdef DAEDALUS_PSP
#define CPU_KeepRunning() (1)
#else
#define CPU_KeepRunning() (CPU_IsRunning())
#endif

inline void CPU_SetPC( u32 pc )		{ ctx.cpuState.CurrentPC = pc; }
inline void INCREMENT_PC()			{ ctx.cpuState.CurrentPC += 4; }
inline void DECREMENT_PC()			{ ctx.cpuState.CurrentPC -= 4; }

inline void CPU_TakeBranch( u32 new_pc ) { ctx.cpuState.TargetPC = new_pc; ctx.cpuState.Delay = DO_DELAY; }


#define COUNTER_INCREMENT_PER_OP			1

extern u8 *		gLastAddress;


#ifdef FRAGMENT_SIMULATE_EXECUTION
void	CPU_ExecuteOpRaw( u32 count, u32 address, OpCode op_code, CPU_Instruction p_instruction, bool * p_branch_taken );
#endif
// Needs to be callable from assembly
extern "C"
{
	void	 CPU_UpdateCounter( u32 ops_executed );
#ifdef FRAGMENT_SIMULATE_EXECUTION
	void	CPU_UpdateCounterNoInterrupt( u32 ops_exexuted );
#endif
	void	CPU_HANDLE_COUNT_INTERRUPT();
}

extern	void (* g_pCPUCore)();

//***********************************************
// This function gets called *alot* //Corn
//***********************************************
//
// From ReadAddress
//
#define CPU_FETCH_INSTRUCTION(ptr, pc)								\
	const MemFuncRead & m( g_MemoryLookupTableRead[ pc >> 18 ] );	\
	if((m.pRead != nullptr) )					\
	{																\
/* Access through pointer with no function calls at all (Fast) */	\
		ptr = ( m.pRead + pc );										\
	}																\
	else															\
	{																\
/* ROM or TLB and possible trigger for an exception (Slow)*/		\
		ptr = (u8*)m.ReadFunc( pc );								\
		if( ctx.cpuState.GetStuffToDo() )								\
			return;													\
	}

//***********************************************


#ifdef DAEDALUS_PROFILE_EXECUTION
extern u64									gTotalInstructionsExecuted;
extern u64									gTotalInstructionsEmulated;
extern u32									g_HardwareInterrupt;
#endif

#ifdef DAEDALUS_ENABLE_SYNCHRONISATION
extern u32 CPU_ProduceRegisterHash();
#endif

//This function gets called *alot* //Corn
//CPU_ProcessEventCycles
//***********************************************
inline bool CPU_ProcessEventCycles( u32 cycles )
{
	LOCK_EVENT_QUEUE();
#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( ctx.cpuState.NumEvents > 0, "There are no events" );
	#endif
	ctx.cpuState.Events[ 0 ].mCount -= cycles;
	return ctx.cpuState.Events[ 0 ].mCount <= 0;
}


#endif // CORE_CPU_H_
