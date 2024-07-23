#pragma once

#include "RomFile/RomFileSettings.h"

namespace UI
{
	void LoadRomPreferences(RomID mRomID);
	bool DrawOptionsPage(RomID mRomID);
	void DrawInGameMenu();
}