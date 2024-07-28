#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <iostream> 
#include <fstream> 

#include <3ds.h>
#include <GL/picaGL.h>


#include "Config/ConfigOptions.h"
#include "Interface/Cheats.h"
#include "Core/CPU.h"
#include "Core/CPU.h"
#include "Core/Memory.h"
#include "Core/PIF.h"
#include "RomFile/RomFileSettings.h"
#include "Core/Save.h"
#include "Debug/DBGConsole.h"
#include "Debug/DebugLog.h"
#include "Graphics/GraphicsContext.h"
#include "HLEGraphics/TextureCache.h"
#include "Input/InputManager.h"
#include "Interface/RomDB.h"
#include "System/SystemInit.h"
#include "Utility/BatchTest.h"

#include "UI/UserInterface.h"
#include "UI/RomSelector.h"

#include "System/IO.h"
#include "Interface/Preferences.h"
#include "Utility/Profiler.h"
#include "System/Thread.h"


#include "Utility/Timer.h"
#include "RomFile/RomFile.h"
#include "Utility/MemoryCTR.h"

bool isN3DS = false;
bool shouldQuit = false;

EAudioPluginMode enable_audio = APM_ENABLED_ASYNC;

void log2file(const std::filesystem::path& baseDir, const char* format, ...) {
    va_list arg;
    va_start(arg, format);

    char msg[512];
    vsnprintf(msg, sizeof(msg), format, arg);

    va_end(arg);

    // Safely append a newline character
    size_t msg_len = strlen(msg);
    if (msg_len < sizeof(msg) - 1) {
        msg[msg_len] = '\n';
        msg[msg_len + 1] = '\0';
    } else {
        msg[sizeof(msg) - 2] = '\n';
        msg[sizeof(msg) - 1] = '\0';
    }

    std::filesystem::path logFilePath = baseDir / "DaedalusX64.log";
    std::ofstream log(logFilePath, std::ios::out | std::ios::app); // Open in append mode
    if (log.is_open()) {
        log.write(msg, strlen(msg));
        log.close();
    } else {
        // Handle the error case where the file could not be opened
        std::cerr << "Failed to open log file: " << logFilePath << std::endl;
    }
}

static void CheckDSPFirmware()
{	
	std::filesystem::path firmwarePath = "sdmc:/3ds/dspfirm.cdc";
	std::ifstream firmware(firmwarePath, std::ios::binary |  std::ios::in);

	if(firmware.is_open())
	{
		firmware.close();
		return;
	}
	else
	{
	gfxInitDefault();
	consoleInit(GFX_BOTTOM, NULL);

	std::cout << "DSP Firmware not found " << std::endl;
	std::cout << "Press START to exit" << std::endl;
	while(aptMainLoop())
	{
		hidScanInput();

		if(hidKeysDown() == KEY_START)
			exit(1);
	}
	}

}	

static void Initialize()
{
	CheckDSPFirmware();
	
	_InitializeSvcHack();

	romfsInit();
	
	APT_CheckNew3DS(&isN3DS);
	osSetSpeedupEnable(true);
	
	gfxInit(GSP_BGR8_OES, GSP_BGR8_OES, true);

	if(isN3DS)
		gfxSetWide(true);
	
	pglInitEx(0x080000, 0x040000);

	UI::Initialize();

	System_Init();
}


void HandleEndOfFrame()
{
	shouldQuit = !aptMainLoop();
	
	if (shouldQuit)
	{
		CPU_Halt("Exiting");
	}
}

extern u32 __ctru_heap_size;

int main(int argc, char* argv[])
{
	char fullpath[512];

	Initialize();
	
	while(shouldQuit == false)
	{
	
	// Set the default path

	std::string rom = UI::DrawRomSelector();
	std::filesystem::path RomPath = baseDir / "Roms" / std::string(rom);

	System_Open(RomPath.string());
	CPU_Run();
	System_Close();
	}
	
	System_Finalize();
	pglExit();

	return 0;
}
