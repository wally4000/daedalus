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


#ifndef UI_PAUSEOPTIONSCOMPONENT_H_
#define UI_PAUSEOPTIONSCOMPONENT_H_

#include "UIContext.h"
#include "UIScreen.h"
#include "UISetting.h"
#include "UICommand.h"
#include "UISpacer.h"
#include "UIComponent.h"

#include <functional>
#include <memory>


class CPauseOptionsComponent : public CUIComponent
{
	public:

		CPauseOptionsComponent( CUIContext * p_context,  std::function<void()> on_resume, std::function<void()> on_reset );
		~CPauseOptionsComponent();

		// CUIComponent
		virtual void				Update( float elapsed_time, const glm::vec2 & stick, u32 old_buttons, u32 new_buttons );		
		static std::unique_ptr<CPauseOptionsComponent>	Create( CUIContext * p_context, std::function<void()> on_resume, std::function<void()> on_reset);	
		virtual void Render();

	private:
				void				OnResume();
				void				OnReset();
				void				EditPreferences();
				void				AdvancedOptions();
				void				CheatOptions();
				void				SaveState();
				void				LoadState();
				void				TakeScreenshot();
#ifdef DAEDALUS_DIALOGS
				void				ExitConfirmation();
#endif
				void				OnSaveStateSlotSelected( const std::filesystem::path& filename );
				void				OnLoadStateSlotSelected( const std::filesystem::path& filename );

#ifdef DAEDALUS_DEBUG_DISPLAYLIST
				void				DebugDisplayList();
#endif
#ifdef DAEDALUS_KERNEL_MODE
				void				ProfileNextFrame();
#endif

	private:
		std::function<void()> 					mOnResume;
		std::function<void()>					mOnReset;

		CUIElementBag				mElements;
};


#endif // UI_PAUSEOPTIONSCOMPONENT_H_
