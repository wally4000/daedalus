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
#include "Interface/RomDB.h"
#include "HLEGraphics/TextureCache.h"

#ifdef DAEDALUS_PSP
#include "SysPSP/Graphics/VideoMemoryManager.h"

#endif

// Graphics Context
#include "Graphics/GraphicsContext.h"
#ifdef DAEDALUS_PSP
#include "Input/PSP/InputManagerPSP.h"
#include "SysPSP/Graphics/GraphicsContextPSP.h"
#include "SysPSP/HLEGraphics/RendererPSP.h"
#include "HLEAudio/Plugin/PSP/AudioPluginPSP.h"
#elif defined(DAEDALUS_CTR) 
#include "Input/CTR/InputManagerCTR.h"
#include "SysCTR/Graphics/GraphicsContextCTR.h"
#include "SysCTR/HLEGraphics/RendererCTR.h"
#include "HLEAudio/Plugin/CTR/AudioPluginCTR.h"
#elif defined(DAEDALUS_GL)
#include "Input/SDL/InputManagerSDL.h"
#include "SysGL/Graphics/GraphicsContextGL.h"
#include "SysGL/HLEGraphics/RendererGL.h"
#include "HLEAudio/Plugin/SDL/AudioPluginSDL.h"
#elif defined (DAEDALUS_GLES)
#include "Input/SDL/InputManagerSDL.h"
#include "SysGL/Graphics/GraphicsContextGLES.h"
#include "SysGLES/HLEGraphics/RendererGL.h"
#include "HLEAudio/Plugin/SDL/AudioPluginSDL.h"
#endif


#if defined(DAEDALUS_POSIX) || defined(DAEDALUS_W32)
#include "SysPosix/Debug/WebDebug.h"
#include "SysPosix/HLEGraphics/TextureCacheWebDebug.h"
#include "HLEGraphics/DisplayListDebugger.h"
#endif

#include "HLEGraphics/DLParser.h"
#include "Utility/FramerateLimiter.h"
#include "Debug/Synchroniser.h"
#include "Base/Macros.h"
#include "Utility/Profiler.h"
#include "Interface/Preferences.h"
#include "Utility/Translate.h"

#include "Input/InputManager.h"

#include "Debug/DBGConsole.h"
#include "Debug/DebugLog.h"

#include "HLEGraphics/GraphicsPlugin.h"
#include "HLEGraphics/BaseRenderer.h"
#include "HLEAudio/AudioPlugin.h"
#include "Interface/Preferences.h"
#include "Interface/RomDB.h"
#include "RomFile/RomFileMemory.h"

#include <array>
#include <memory>
#include <functional>

SystemContext ctx; 
SystemContext::SystemContext(): eventQueueLocked(0)
{
	romInfo = std::make_unique<RomInfo>();
}

struct SysEntityEntry
{
	const char *name;
	std::function<bool(SystemContext&)> init;
	std::function<void(SystemContext&)> final;	
};

struct RomEntityEntry
{
	const char *name;
	std::function<bool(SystemContext&)> open;
	std::function<void(SystemContext&)> close;
};

// Completed
bool Init_Audio(SystemContext& ctx)
{
    ctx.audioPlugin = std::make_unique<AudioPlugin>();

    EAudioPluginMode mode = APM_DISABLED; // Default mode if preferences are unavailable

    if (ctx.preferences && ctx.romInfo)
    {
        SRomPreferences rom_prefs;
        if (ctx.preferences->GetRomPreferences(ctx.romInfo->mRomID, &rom_prefs))
        {
            mode = rom_prefs.AudioEnabled; // Load mode from preferences if available
        }
    }

    ctx.audioPlugin->SetMode(mode);
    ctx.audioPlugin->StartEmulation();

    return true;
}

 void Dispose_Audio(SystemContext& ctx)
{
	if (ctx.audioPlugin)
	{
		ctx.audioPlugin->StopEmulation();
		ctx.audioPlugin.reset();
	}
}

bool Init_InputManager(SystemContext& ctx)
{
	ctx.inputManager = std::make_unique<IInputManager>();
	return ctx.inputManager->Initialise();
}

 void Destroy_InputManager(SystemContext& ctx)
{
	if (ctx.inputManager)
	{
		ctx.inputManager->Finalise();
		ctx.inputManager.reset();
	}
}

bool Init_GraphicsPlugin(SystemContext& ctx)
{
	DAEDALUS_ASSERT(ctx.graphicsPlugin == NULL, "Graphics Plugin should not be initialised at this point");

	ctx.graphicsPlugin = std::make_unique<CGraphicsPlugin>();
	ctx.textureCache = std::make_unique<CTextureCache>();

	if (!DLParser_Initialise())
	{
		DBGConsole_Msg(0, "ERROR: Failed to initialise DLParser.");
		ctx.graphicsPlugin.reset();
		ctx.textureCache.reset();
	}
	return DLParser_Initialise();
	
}
void Destroy_GraphicsPlugin(SystemContext& ctx)
{
	if (ctx.graphicsPlugin)
	{
		DLParser_Finalise(); // This does nothing anyway
		ctx.graphicsPlugin.reset();
		ctx.textureCache.reset();
	}
}

static bool Init_RomPreferences(SystemContext& ctx)
{
	ctx.preferences = std::make_unique<CPreferences>();
	return true;
}

static void Destroy_RomPreferences(SystemContext& ctx)
{
	ctx.preferences.reset();
}

static bool Init_RomDB(SystemContext& ctx)
{
	ctx.romDB = std::make_unique<CRomDB>();
	return ctx.romDB->OpenDB(setBasePath("rom.db"));
}

static void Destroy_RomDB(SystemContext& ctx)
{
	ctx.romDB.reset();
}

// WIP

static bool Init_RomSettingsDB(SystemContext& ctx)
{
	ctx.romSettingsDB = std::make_unique<CRomSettingsDB>();
	return ctx.romSettingsDB->OpenSettingsFile(setBasePath("roms.ini"));
}
static void Destroy_RomSettingsDB(SystemContext& ctx)
{
	ctx.romSettingsDB.reset();
}


#ifdef DAEDALUS_ENABLE_PROFILING
static void ProfilerVblCallback(void * arg)
{
	gProfiler->Update();
	gProfiler->Display();
}

static bool Profiler_Init(SystemContext& ctx)
{
	gProfiler = std::make_unique<CPreferences>();

	CPU_RegisterVblCallback(&ProfilerVblCallback, NULL);

	return true;
}

static void Profiler_Fini(SystemContext& ctx)
{
	CPU_UnregisterVblCallback(&ProfilerVblCallback, NULL);
	gProfiler.reset();
}
#endif

static bool Init_Translate(SystemContext& ctx) { // should be a class and unique_ptr
	 Translate_Load("Languages/" );
	 return true;
}

static bool Init_Memory(SystemContext& ctx) { // should be a class and unique_ptr
	return Memory_Init();
}

static void Destroy_Memory(SystemContext& ctx) { // should be a class and unique_ptr
	 Memory_Fini();
}

bool Init_pifController(SystemContext& ctx)
{
	ctx.pifController = std::make_unique<CController>();
	return true;
}

void Destroy_pifController(SystemContext& ctx)
{
	if (ctx.pifController)
	{
		ctx.pifController->OnRomClose(); // Does nothing 
		ctx.pifController.reset();
	}
}


 bool Init_GraphicsContext(SystemContext& ctx) {
	ctx.graphicsContext = std::make_unique<IGraphicsContext>();
	return ctx.graphicsContext->Initialise();
}

 void Destroy_GraphicsContext(SystemContext& ctx) {
    ctx.graphicsContext.reset();
}

bool Init_ROMBuffer(SystemContext& ctx)
{
	// Create memory heap used for either ROM Cache or ROM buffer
	// We do this to avoid memory fragmentation
	ctx.ROMFileMemory = std::make_unique<CROMFileMemory>();

	return true;
}

bool Init_Renderer(SystemContext& ctx)
{
	DAEDALUS_ASSERT_Q(ctx.renderer == NULL);
	ctx.renderer = std::make_unique<Renderer>(); 
	return true;
}
void Destroy_Renderer(SystemContext& ctx)
{
	ctx.renderer.reset();
}

static void Destroy_ROMBuffer(SystemContext& ctx) {
	ctx.ROMFileMemory.reset();
}
#ifdef DAEDALUS_PSP
bool Init_PSPVideoMemoryManager(SystemContext& ctx)
{
	ctx.videoMemoryManager = std::make_unique<CVideoMemoryManager>();
	return true;
}

void Destroy_PSPVideoMemoryManager(SystemContext& ctx)
{
	if (ctx.videoMemoryManager)
	{
		ctx.videoMemoryManager.reset();
	}
}
#endif

#ifdef DAEDALUS_DEBUG_CONSOLE
bool Legacy_Debug_Init(SystemContext& ctx) {	return Init_DebugConsole(); }

bool Legacy_DebugLog_Init(SystemContext& ctx) { return Debug_InitLogging(); }

static void Legacy_Debug_Destroy(SystemContext& ctx) {Destroy_DebugConsole(); }


// Debug Console
static bool Init_OldDebug(SystemContext& ctx)              { return Legacy_Debug_Init(ctx); }
static void Dispose_OldDebug(SystemContext& ctx)           { Legacy_Debug_Destroy(ctx); }

// Debug Logger
static bool Init_OldDebugLog(SystemContext& ctx)           { return Legacy_DebugLog_Init(ctx); }
#endif



// FramerateLimiter (System table doesn't use it, but included here if needed)
static bool Init_OldFramerateLimiter(SystemContext& ctx)   { return FramerateLimiter_Reset(); }

const std::vector<SysEntityEntry> gSysInitTable =
{
#ifdef DAEDALUS_DEBUG_CONSOLE
	{"DebugConsole",         Init_OldDebug,              Dispose_OldDebug},
#endif
#ifdef DAEDALUS_LOG
	{"Logger",               Init_OldDebugLog,           nullptr},
#endif
#ifdef DAEDALUS_ENABLE_PROFILING
	{"Profiler",             Profiler_Init,              Profiler_Fini},
#endif
	{"Renderer", 			 Init_Renderer, 			 Destroy_Renderer},
	{"ROM Database",         Init_RomDB,              	 Destroy_RomDB},
	{"ROM Settings",         Init_RomSettingsDB,     	 Destroy_RomSettingsDB},
	{"InputManager",         Init_InputManager,       	 Destroy_InputManager},
#ifndef DAEDALUS_CTR
	{"Language",             Init_Translate,             nullptr},
#endif
#ifdef DAEDALUS_PSP
	{"VideoMemory",          Init_PSPVideoMemoryManager, Destroy_PSPVideoMemoryManager},
#endif
	{"GraphicsContext",		 Init_GraphicsContext,  	 Destroy_GraphicsContext},
	{"Preference",           Init_RomPreferences,     	 Destroy_RomPreferences},
	{"Memory",               Init_Memory,                Destroy_Memory},
	{"Controller",           Init_pifController,         Destroy_pifController},
	{"RomBuffer",            Init_ROMBuffer,          	Destroy_ROMBuffer},

#if defined(DAEDALUS_POSIX) || defined(DAEDALUS_W32)
#ifdef DAEDALUS_DEBUG_DISPLAYLIST
	{"WebDebug",             WebDebug_Init,              WebDebug_Fini},
	{"TextureCacheWebDebug",TextureCache_RegisterWebDebug, nullptr},
	{"DLDebuggerWebDebug",   DLDebugger_RegisterWebDebug, nullptr},
#endif
#endif
};


bool Legacy_RomBuffer_Open(SystemContext& ctx)
{
	// Create memory heap used for either ROM Cache or ROM buffer
	// We do this to avoid memory fragmentation
	return RomBuffer::Open(); // Not a class
}

void Legacy_RomBuffer_Close(SystemContext& ctx)
{
	return RomBuffer::Close(); // Not a class
}

static bool Legacy_RomFile_Open(SystemContext& ctx) {
	return ROM_LoadFile(); // Not a class
}

static void Legacy_RomFile_Close(SystemContext& ctx) {
	return ROM_UnloadFile(); // not a class
}

static bool Legacy_Memory_Reset(SystemContext& ctx) {
	return Memory_Reset(); // Not a class
}

static void Legacy_Memory_Cleanup(SystemContext& ctx) {
	return Memory_Cleanup(); // Not a class
}

static bool Legacy_FramerateLimiter_Reset(SystemContext& ctx) {
	return FramerateLimiter_Reset(); // not a class
}

static bool Legacy_CPU_RomOpen(SystemContext& ctx) {
	return CPU_RomOpen(); // not a class
} 

static void Legacy_CPU_RomClose(SystemContext& ctx) {
	// Does nothing, supposedly dumps the fragment cache.
}

static bool Controller_Reset(SystemContext& ctx) {
	if (!ctx.pifController)
	{
		ctx.pifController = std::make_unique<CController>();
	}
	return ctx.pifController->OnRomOpen(); 

}

static void Controller_RomClose(SystemContext& ctx) {
	ctx.pifController.reset(); // Does nothing othewise
}

static bool Legacy_Save_Reset(SystemContext& ctx) {
	return Save_Reset(); // Not a class
}

static void Legacy_Save_Fini(SystemContext& ctx) {
	return Save_Fini(); // Not a class
}

static bool Legacy_CPU_RomReboot(SystemContext& ctx) {
	return ROM_ReBoot(); // Not a class
}

static void Legacy_CPU_RomUnload(SystemContext& ctx) {
	return ROM_Unload(); // Not a class
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

// Save
static bool Init_OldSave(SystemContext&)                  { return Legacy_Save_Reset(ctx); }
static void Destroy_OldSave(SystemContext&)               { Legacy_Save_Fini(ctx); }
static const std::vector<RomEntityEntry> gRomInitTable =
{
	{"RomBuffer",           Init_OldRomBufferOpen,      Destroy_OldRomBufferOpen},
	{"Settings",            Init_OldRomFile,            Destroy_OldRomFile},
	{"Memory",              Init_OldMemoryReset,        Destroy_OldMemoryReset},
	{"Audio",               Init_Audio,                 Dispose_Audio},
	{"Graphics",            Init_GraphicsPlugin,        Destroy_GraphicsPlugin},
	{"FramerateLimiter",    Init_OldFramerateLimiter,   nullptr},
	{"CPU",                 Init_OldCPU,                Destroy_OldCPU},
	{"ROM",                 Init_OldROM,                Destroy_OldROM},
	{"Controller",          Controller_Reset,   		Controller_RomClose},
	{"Save",                Init_OldSave,               Destroy_OldSave}
// #ifdef DAEDALUS_ENABLE_SYNCHRONISATION
// 	{"CSynchroniser",       CSynchroniser::InitialiseSynchroniser, CSynchroniser::Destroy},
// #endif
};

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

bool System_Open(const std::filesystem::path &filename)
{
	
	// // Close any previously loaded ROM
	// System_Close();



	// #ifdef DAEDALUS_PSP

	// if (ctx.videoMemoryManager)
	// {
	// 	ctx.videoMemoryManager->Reset();
	// 	DBGConsole_Msg(0, "==> VideoMemoryManager: Reset VRAM/ERAM heaps");
	// }
	// #endif
	
	if (!ctx.romInfo)
		ctx.romInfo = std::make_unique<RomInfo>();
	ctx.romInfo->mFileName = filename;

	for (const auto& entry : gRomInitTable)
	{
		if (!entry.name) continue;

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