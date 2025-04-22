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


#ifndef UI_SELECTEDROMCOMPONENT_H_
#define UI_SELECTEDROMCOMPONENT_H_

#include "UIComponent.h"
#include "UIContext.h"
#include "UIScreen.h"
#include "UISetting.h"
#include "UICommand.h"
#include "UISpacer.h"

#include <functional>


class CSelectedRomComponent :  public CUIComponent
{
	public:

		CSelectedRomComponent( CUIContext * p_context, std::function<void()> on_start_emulation );
		~CSelectedRomComponent();

		// CUIComponent
		virtual void				Update( float elapsed_time, const glm::vec2 & stick, u32 old_buttons, u32 new_buttons );
		virtual void				Render();

		virtual void				SetRomID( const RomID & rom_id )			{ mRomID = rom_id; }

	private:
		void						EditPreferences();
		void						AdvancedOptions();
		void						CheatOptions();
		void						StartEmulation();

	private:
		std::function<void()> OnStartEmulation;

		CUIElementBag				mElements;

		RomID						mRomID;
};


#endif // UI_SELECTEDROMCOMPONENT_H_
