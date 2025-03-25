/*
Copyright (C) 2009 Howard Su (howard0su@gmail.com)

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


#include "Base/Types.h"
#include "System/SystemInit.h"

#include "Core/Memory.h"
#include "Core/CPU.h"
#include "Core/Save.h"
#include "Core/PIF.h"
#include "RomFile/ROMBuffer.h"
#include "RomFile/RomSettings.h"
#include "Input/SDL/InputManagerSDL.h" // Really needs to be one big header file for all platforms
#include "Interface/RomDB.h"

#ifdef DAEDALUS_PSP
#include "SysPSP/Graphics/VideoMemoryManager.h"
#endif

#include "Graphics/GraphicsContext.h"

#if defined(DAEDALUS_POSIX) || defined(DAEDALUS_W32)
#include "SysPosix/Debug/WebDebug.h"
#include "SysPosix/HLEGraphics/TextureCacheWebDebug.h"
#include "HLEGraphics/DisplayListDebugger.h"
#endif


#include "Utility/FramerateLimiter.h"
#include "Debug/Synchroniser.h"
#include "Base/Macros.h"
#include "Utility/Profiler.h"
#include "Interface/Preferences.h"
#include "Utility/Translate.h"

#include "Input/InputManager.h"		// CInputManager::Create/Destroy

#include "Debug/DBGConsole.h"
#include "Debug/DebugLog.h"

#include "HLEGraphics/GraphicsPlugin.h"
#include "HLEAudio/AudioPlugin.h"
#include "Interface/Preferences.h"
#include "Interface/RomDB.h"
#include "RomFile/RomFileMemory.h"

#include <array>
#include <memory>
#include <functional>


SystemContext ctx; 
SystemContext* g_SystemContext = &ctx;

std::unique_ptr<CRomDB> gRomDB;

 bool Init_LegacyAudio(SystemContext& ctx)
{
	std::unique_ptr<CAudioPlugin> audio_plugin = CreateAudioPlugin();
	if( audio_plugin != NULL )
	{
		gAudioPlugin = std::move(audio_plugin);
	}
	gAudioPlugin->StartEmulation();
	return true;
}

 void Dispose_LegacyAudio(SystemContext& ctx)
{
	if ( gAudioPlugin != NULL )
	{
		gAudioPlugin->StopEmulation();
		gAudioPlugin.release();
		gAudioPlugin = NULL;
	}
}

bool Init_LegacyInputManager(SystemContext& ctx)
{
	ctx.gInputManager = std::make_unique<IInputManager>();
	return ctx.gInputManager->Initialise();
}

 void Destroy_LegacyInputManager(SystemContext& ctx)
{
	ctx.gInputManager->Finalise();
}


bool Init_GraphicsPlugin(SystemContext& ctx)
{
	DAEDALUS_ASSERT(ctx.graphicsPlugin == NULL, "Graphics Plugin should not be initialised at this point");

	auto plugin = CreateGraphicsPlugin();
	if (plugin)
	{
		ctx.graphicsPlugin = std::move(plugin);
		return true;
	}
	return false;
}
void Cleanup_GraphicsPlugin(SystemContext& ctx)
{
	if (ctx.graphicsPlugin)
	{
		ctx.graphicsPlugin->RomClosed();
		ctx.graphicsPlugin.reset();
	}
}


static bool Init_LegacyRomPreferences(SystemContext& ctx)
{
	gPreferences = std::make_unique<CPreferences>();
	return true;
}

static void Destroy_LegacyRomPreferences(SystemContext& ctx)
{
	gPreferences.reset();
}

static bool Init_LegacyRomDB(SystemContext& ctx)
{
	gRomDB = std::make_unique<CRomDB>();
	return gRomDB->OpenDB(setBasePath("rom.db"));
}

static void Destroy_LegacyRomDB(SystemContext& ctx)
{
	gRomDB.reset();
}
struct SysEntityEntry
{
	const char *name;
	std::function<bool(SystemContext&)> init;
	std::function<void(SystemContext&)> final;	
};


static bool Init_LegacyRomSettingsDB(SystemContext& ctx)
{
	gRomSettingsDB = std::make_unique<CRomSettingsDB>();
	return gRomSettingsDB->OpenSettingsFile(setBasePath("roms.ini"));
}
static void Destroy_LegacyRomSettingsDB(SystemContext& ctx)
{
	gRomSettingsDB.reset();
}


#ifdef DAEDALUS_ENABLE_PROFILING
static void ProfilerVblCallback(void * arg)
{
	gProfiler->Update();
	gProfiler->Display();
}

static bool Profiler_Init()
{
	gProfiler = std::make_unique<CPreferences>();

	CPU_RegisterVblCallback(&ProfilerVblCallback, NULL);

	return true;
}

static void Profiler_Fini()
{
	CPU_UnregisterVblCallback(&ProfilerVblCallback, NULL);
	gProfiler.reset();
}
#endif
static bool Init_Translate(SystemContext& ctx) {
	return Translate_Init();
}

static bool Init_Memory(SystemContext& ctx) {
	return Memory_Init();
}

static void Destroy_Memory(SystemContext& ctx) {
	 Memory_Fini();
}

bool Init_LegacyController(SystemContext& ctx)
{
	gController = std::make_unique<CController>();
	return true;
}

void Destroy_LegacyController(SystemContext& ctx)
{
	if (gController)
	{
		gController->OnRomClose();
		gController.reset();
	}
}

bool Legacy_RomBuffer_Init(SystemContext& ctx)
{
	// Create memory heap used for either ROM Cache or ROM buffer
	// We do this to avoid memory fragmentation
	return RomBuffer::Create();
}

static void Legacy_RomBuffer_Destroy(SystemContext& ctx) {
	return RomBuffer::Destroy();
}

#ifdef DAEDALUS_DEBUG_CONSOLE
bool Legacy_Debug_Init(SystemContext& ctx)
{

	return Init_DebugConsole();
}

bool Legacy_DebugLog_Init(SystemContext& ctx)
{

	return Debug_InitLogging();
}

static void Legacy_Debug_Destroy(SystemContext& ctx) {
	Destroy_DebugConsole();
}


// Debug Console
static bool Init_OldDebug(SystemContext&)              { return Legacy_Debug_Init(ctx); }
static void Dispose_OldDebug(SystemContext&)           { Legacy_Debug_Destroy(ctx); }

// Debug Logger
static bool Init_OldDebugLog(SystemContext&)           { return Legacy_DebugLog_Init(ctx); }
#endif
// Audio
static bool Init_OldAudio(SystemContext&)              { return Init_LegacyAudio(ctx); }
static void Dispose_OldAudio(SystemContext&)           { Dispose_LegacyAudio(ctx); }

// Input Manager
static bool Init_OldInputManager(SystemContext&)       { return Init_LegacyInputManager(ctx); }
static void Destroy_OldInputManager(SystemContext&)    { Destroy_LegacyInputManager(ctx); }

// ROM DB
static bool Init_OldRomDB(SystemContext&)              { return Init_LegacyRomDB(ctx); }
static void Destroy_OldRomDB(SystemContext&)           { Destroy_LegacyRomDB(ctx); }

// ROM Settings DB
static bool Init_OldRomSettingsDB(SystemContext&)      { return Init_LegacyRomSettingsDB(ctx); }
static void Destroy_OldRomSettingsDB(SystemContext&)   { Destroy_LegacyRomSettingsDB(ctx); }

// Preferences
static bool Init_OldRomPreferences(SystemContext&)     { return Init_LegacyRomPreferences(ctx); }
static void Destroy_OldRomPreferences(SystemContext&)  { Destroy_LegacyRomPreferences(ctx); }

// Graphics Plugin
static bool Init_OldGraphics(SystemContext&)           { return Init_GraphicsPlugin(ctx); }
static void Destroy_OldGraphics(SystemContext&)        { Cleanup_GraphicsPlugin(ctx); }

// Controller
static bool Init_OldController(SystemContext&)         { return Init_LegacyController(ctx); }
static void Destroy_OldController(SystemContext&)      { Destroy_LegacyController(ctx); }

// RomBuffer Init
static bool Init_OldRomBuffer(SystemContext&)          { return Legacy_RomBuffer_Init(ctx); }
static void Destroy_OldRomBuffer(SystemContext&)       { Legacy_RomBuffer_Destroy(ctx); }


// FramerateLimiter (System table doesn't use it, but included here if needed)
static bool Init_OldFramerateLimiter(SystemContext&)   { return FramerateLimiter_Reset(); }

static const std::vector<SysEntityEntry> gSysInitTable =
{{
#ifdef DAEDALUS_DEBUG_CONSOLE
	{"DebugConsole",         Init_OldDebug,              Dispose_OldDebug},
#endif
#ifdef DAEDALUS_LOG
	{"Logger",               Init_OldDebugLog,           nullptr},
#endif
#ifdef DAEDALUS_ENABLE_PROFILING
	{"Profiler",             Profiler_Init,              Profiler_Fini},
#endif
	{"ROM Database",         Init_OldRomDB,              Destroy_OldRomDB},
	{"ROM Settings",         Init_OldRomSettingsDB,      Destroy_OldRomSettingsDB},
	{"InputManager",         Init_OldInputManager,       Destroy_OldInputManager},
#ifndef DAEDALUS_CTR
	{"Language",             Init_Translate,             nullptr},
#endif
#ifdef DAEDALUS_PSP
	{"VideoMemory",          Init_PSP_VideoMemoryManager, Destroy_PSP_VideoMemoryManager},
#endif
	{"GraphicsContext",
		[](SystemContext&) { return mGraphicsContext->Create(); },
		[](SystemContext&) { mGraphicsContext->Destroy(); }
	},
	{"Preference",           Init_OldRomPreferences,     Destroy_OldRomPreferences},
	{"Memory",               Init_Memory,                Destroy_Memory},
	{"Controller",           Init_OldController,         Destroy_OldController},
	{"RomBuffer",            Init_OldRomBuffer,          Destroy_OldRomBuffer},

#if defined(DAEDALUS_POSIX) || defined(DAEDALUS_W32)
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	{"WebDebug",             WebDebug_Init,              WebDebug_Fini},
	{"TextureCacheWebDebug",TextureCache_RegisterWebDebug, nullptr},
	{"DLDebuggerWebDebug",   DLDebugger_RegisterWebDebug, nullptr},
#endif
#endif
}};

struct RomEntityEntry
{
	const char *name;
	std::function<bool(SystemContext&)> open;
	std::function<void(SystemContext&)> close;
};

bool Legacy_RomBuffer_Open(SystemContext& ctx)
{
	// Create memory heap used for either ROM Cache or ROM buffer
	// We do this to avoid memory fragmentation
	return RomBuffer::Open();
}

void Legacy_RomBuffer_Close(SystemContext& ctx)
{
	return RomBuffer::Close();
}

static bool Legacy_RomFile_Open(SystemContext& ctx) {
	return ROM_LoadFile();
}

static void Legacy_RomFile_Close(SystemContext& ctx) {
	return ROM_UnloadFile();
}

static bool Legacy_Memory_Reset(SystemContext& ctx) {
	return Memory_Reset();
}

static void Legacy_Memory_Cleanup(SystemContext& ctx) {
	return Memory_Cleanup();
}

static bool Legacy_FramerateLimiter_Reset(SystemContext& ctx) {
	return FramerateLimiter_Reset();
}

static bool Legacy_CPU_RomOpen(SystemContext& ctx) {
	return CPU_RomOpen();
}

static void Legacy_CPU_RomClose(SystemContext& ctx) {
	return CPU_RomClose();
}

static bool Legacy_Controller_Reset(SystemContext& ctx) {
	return CController::Reset();
}

static void Legacy_Controller_RomClose(SystemContext& ctx) {
	CController::RomClose();
}

static bool Legacy_Save_Reset(SystemContext& ctx) {
	return Save_Reset();
}

static void Legacy_Save_Fini(SystemContext& ctx) {
	return Save_Fini();
}

static bool Legacy_CPU_RomReboot(SystemContext& ctx) {
	return ROM_ReBoot();
}

static void Legacy_CPU_RomUnload(SystemContext& ctx) {
	return ROM_Unload();
}
// RomBuffer
static bool Init_OldRomBufferOpen(SystemContext&)         { return Legacy_RomBuffer_Open(ctx); }
static void Destroy_OldRomBufferOpen(SystemContext&)      { Legacy_RomBuffer_Close(ctx); }

// Settings (ROM file)
static bool Init_OldRomFile(SystemContext&)               { return Legacy_RomFile_Open(ctx); }
static void Destroy_OldRomFile(SystemContext&)            { Legacy_RomFile_Close(ctx); }

// Memory Reset/Cleanup
static bool Init_OldMemoryReset(SystemContext&)           { return Legacy_Memory_Reset(ctx); }
static void Destroy_OldMemoryReset(SystemContext&)        { Legacy_Memory_Cleanup(ctx); }

// CPU
static bool Init_OldCPU(SystemContext&)                   { return Legacy_CPU_RomOpen(ctx); }
static void Destroy_OldCPU(SystemContext&)                { Legacy_CPU_RomClose(ctx); }

// ROM Reboot
static bool Init_OldROM(SystemContext&)                   { return Legacy_CPU_RomReboot(ctx); }
static void Destroy_OldROM(SystemContext&)                { Legacy_CPU_RomUnload(ctx); }

// Controller
static bool Init_OldControllerReset(SystemContext&)       { return Legacy_Controller_Reset(ctx); }
static void Destroy_OldControllerReset(SystemContext&)    { Legacy_Controller_RomClose(ctx); }

// Save
static bool Init_OldSave(SystemContext&)                  { return Legacy_Save_Reset(ctx); }
static void Destroy_OldSave(SystemContext&)               { Legacy_Save_Fini(ctx); }
static const std::vector<RomEntityEntry> gRomInitTable =
{{
	{"RomBuffer",           Init_OldRomBufferOpen,      Destroy_OldRomBufferOpen},
	{"Settings",            Init_OldRomFile,            Destroy_OldRomFile},
	{"InputManager",        Init_OldInputManager,       Destroy_OldInputManager},
	{"Memory",              Init_OldMemoryReset,        Destroy_OldMemoryReset},
	{"Audio",               Init_OldAudio,              Dispose_OldAudio},
	{"Graphics",            Init_OldGraphics,           Destroy_OldGraphics},
	{"FramerateLimiter",    Init_OldFramerateLimiter,   nullptr},
	{"CPU",                 Init_OldCPU,                Destroy_OldCPU},
	{"ROM",                 Init_OldROM,                Destroy_OldROM},
	{"Controller",          Init_OldControllerReset,    Destroy_OldControllerReset},
	{"Save",                Init_OldSave,               Destroy_OldSave}
// #ifdef DAEDALUS_ENABLE_SYNCHRONISATION
// 	{"CSynchroniser",       CSynchroniser::InitialiseSynchroniser, CSynchroniser::Destroy},
// #endif
}};

bool System_Init()
{
	for (const auto& entry : gSysInitTable)
	{
		if (entry.init && !entry.init(ctx))
		{
			DBGConsole_Msg(0, "===>Init %s failed", entry.name);
			return false;
		}
		DBGConsole_Msg(0, "==> Init %s succeeded", entry.name);
	}
	return true;
}
// bool System_Init()
// {
// 	for(u32 i = 0; i < gSysInitTable.size(); i++)
// 	{
// 		const SysEntityEntry & entry = gSysInitTable[i];

// 		if (entry.init == NULL)
// 			continue;

// 		if (entry.init())
// 		{
// 			#ifdef DAEDALUS_DEBUG_CONSOLE
// 			DBGConsole_Msg(0, "==>Initialized %s", entry.name);
// 			#endif
// 		}
// 		else
// 		{
// 				#ifdef DAEDALUS_DEBUG_CONSOLE
// 			DBGConsole_Msg(0, "==>Initialize %s Failed", entry.name);
// 			#endif
// 			return false;
// 		}
// 	}

// 	return true;
// }

bool System_Open(const std::filesystem::path &filename)
{
	g_ROM.mFileName = filename;

	for (size_t i = 0; i < gRomInitTable.size(); ++i)
	{
		const auto& entry = gRomInitTable[i];

		if (!entry.name)
		{
			DBGConsole_Msg(0, "==>Open entry[%zu] has no name (null)", i);
			continue;
		}

		if (entry.open && !entry.open(ctx)) 
		{
			DBGConsole_Msg(0, "==>Open %s [FAILED]", entry.name);
			return false;
		}

		DBGConsole_Msg(0, "==>Open %s succeeded", entry.name);
	}
	return true;
}

void System_Close()
{
	for (auto it = gRomInitTable.rbegin(); it != gRomInitTable.rend(); ++it)
	{
		if (it->close)
		{
			DBGConsole_Msg(0, "==>Close %s", it->name);
			it->close(ctx);  
		}
	}
}

// void System_Finalize()
// {
// 	for(s32 i = gSysInitTable.size() - 1; i >= 0; i--)
// 	{
// 		const SysEntityEntry & entry = gSysInitTable[i];

// 		if (entry.final == NULL)
// 			continue;
// 	#ifdef DAEDALUS_DEBUG_CONSOLE
// 		DBGConsole_Msg(0, "==>Finalize %s", entry.name);
// 		#endif
// 		entry.final();
// 	}
// }

void System_Finalize(SystemContext& ctx)
{
	for (auto it = gSysInitTable.rbegin(); it != gSysInitTable.rend(); ++it)
	{
		if ( it-> final )
		{
			DBGConsole_Msg(0, "===>Finalizing %s", it->name);
			it->final(ctx);
		}
	}
}