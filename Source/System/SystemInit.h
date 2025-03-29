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

#pragma once
#include <filesystem>
#include <memory>
 #include "RomFile/RomFileCache.h"
 #include "Core/ROM.h" 
 #include "Core/CPUState.h"

 
#ifdef DAEDALUS_PSP
class CVideoMemoryManager;
#endif
class CGraphicsContext;

class CPreferences;
class CRomDB;
class CInputManager;
class CRomSettingsDB;
class CAudioPlugin;
class CGraphicsPlugin;
class CROMFileMemory;
class CROMBuffer;
class CDebugConsole;
class CController;
class CTextureCache;
class BaseRenderer;
struct SCPUState;

struct SystemContext {
    SystemContext(); 
    std::unique_ptr<CGraphicsPlugin> graphicsPlugin;
    std::unique_ptr<CAudioPlugin> audioPlugin;
    std::unique_ptr<CPreferences> preferences;
    std::unique_ptr<CRomDB> romDB;
    std::unique_ptr<CRomSettingsDB> romSettingsDB;
    std::unique_ptr<CInputManager> inputManager;
    std::unique_ptr<CROMFileMemory> ROMFileMemory;
    std::unique_ptr<CController> pifController;
    std::unique_ptr<RomInfo> romInfo;
    std::unique_ptr<CTextureCache> textureCache;
    std::unique_ptr<CGraphicsContext> graphicsContext;
    std::unique_ptr<BaseRenderer> renderer;
    SCPUState cpuState;
    volatile u32 eventQueueLocked;

    #ifdef DAEDALUS_PSP
    std::unique_ptr<CVideoMemoryManager> videoMemoryManager;
    #endif

    #ifdef DAEDALUS_DEBUG_CONSOLE
    std::unique_ptr<CDebugConsole> debugConsole;
    #endif
};

extern SystemContext ctx;

// Initialize the whole system
bool System_Init();

// Open a ROM. After this call, you can call CPU_Run safely.
bool System_Open(const std::filesystem::path &filename);

// Close the ROM and cleanup the resources
void System_Close();

// Finalize the whole system
void System_Finalize(SystemContext &ctx);

