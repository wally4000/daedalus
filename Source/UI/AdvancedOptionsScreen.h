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

#ifndef UI_ADVANCEDOPTIONSSCREEN_H_
#define UI_ADVANCEDOPTIONSSCREEN_H_

#include "UIScreen.h"
#include "UIContext.h"
#include "UISetting.h"
#include "UISpacer.h"
#include "UICommand.h"
#include "Menu.h"
#include <memory>

#include "Interface/RomID.h"


class CAdvancedOptionsScreen : public CUIScreen
{
	public:

		CAdvancedOptionsScreen( CUIContext * p_context, const RomID & rom_id );
		~CAdvancedOptionsScreen();
		static std::unique_ptr<CAdvancedOptionsScreen> Create( CUIContext * p_context, const RomID & rom_id );
		// CAdvancedOptionsScreen
		virtual void				Run();

		// CUIScreen
		virtual void				Update( float elapsed_time, const glm::vec2 & stick, u32 old_buttons, u32 new_buttons );
		virtual void				Render();
		virtual bool				IsFinished() const									{ return mIsFinished; }

	private:
				void				OnConfirm();
				void				OnCancel();

	private:
		RomID						mRomID;
		std::string					mRomName;
		SRomPreferences				mRomPreferences;

		bool						mIsFinished;

		CUIElementBag				mElements;
};

#endif // UI_ADVANCEDOPTIONSSCREEN_H_
