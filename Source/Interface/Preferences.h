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

#pragma once

#ifndef UTILITY_PREFERENCES_H_
#define UTILITY_PREFERENCES_H_

#include "Base/Singleton.h"
#include "Interface/GlobalPreferences.h"
#include "Interface/RomPreferences.h"
#include <fstream>
#include <string>
#include <set>
#include <map>
#include <filesystem>
#include "Utility/IniFile.h"
#include "Utility/FramerateLimiter.h"

#include "Interface/ConfigOptions.h"
#include "Core/ROM.h"
#include "Input/InputManager.h"
#include "Interface/RomDB.h"
#include "Utility/Paths.h"


#include "Utility/Translate.h"


class RomID;

class CPreferences 
{
   public:
	 CPreferences();
	 ~CPreferences();

	 bool OpenPreferencesFile(const std::filesystem::path &filename);
	 void Commit();

	 bool GetRomPreferences(const RomID& id, SRomPreferences* preferences) const;
	 void SetRomPreferences(const RomID& id, const SRomPreferences& preferences);

	 private:
	 void					OutputSectionDetails( const RomID & id, const SRomPreferences & preferences, std::ofstream& fh );

	private:
	using PreferencesMap = std::map<RomID, SRomPreferences>;

	 PreferencesMap			mPreferences;

	 bool					mDirty;				// (STRMNNRMN - Changed since read from disk?)
	 std::filesystem::path	mFilename;
};

extern std::unique_ptr<CPreferences> gPreferences;

const char* Preferences_GetTextureHashFrequencyDescription(ETextureHashFrequency thf);
const char* Preferences_GetFrameskipDescription(EFrameskipValue value);

#endif  // UTILITY_PREFERENCES_H_
