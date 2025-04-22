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


#ifndef UI_PAUSESCREEN_H_
#define UI_PAUSESCREEN_H_

#include "UIContext.h"
#include "UIScreen.h"
#include  "MainMenuScreen.h"
class CUIContext;

enum EPauseOptions
{
	PO_GLOBAL_SETTINGS = 0,
	PO_PAUSE_OPTIONS,
	PO_ABOUT,
	NUM_PAUSE_OPTIONS,
};


class CPauseScreen : public CUIScreen
{
	public:

		CPauseScreen( CUIContext * p_context );
		~CPauseScreen();

		// CPauseScreen
		virtual void				Run();

		// CUIScreen
		virtual void				Update( float elapsed_time, const glm::vec2 & stick, u32 old_buttons, u32 new_buttons );
		virtual void				Render();
		virtual bool				IsFinished() const									{ return mIsFinished; }
		static std::unique_ptr<CPauseScreen>	Create( CUIContext * p_context );

	private:
		static	EPauseOptions			GetNextOption( EPauseOptions option )					{ return EPauseOptions( (option + 1) % NUM_MENU_OPTIONS ); }
		static	EPauseOptions			GetPreviousOption( EPauseOptions option )				{ return EPauseOptions( (option + NUM_MENU_OPTIONS -1) % NUM_MENU_OPTIONS ); }

				EPauseOptions			GetPreviousOption() const							{ return GetPreviousOption( mCurrentOption ); }
				EPauseOptions			GetCurrentOption() const							{ return mCurrentOption; }
				EPauseOptions			GetNextOption() const								{ return GetNextOption( mCurrentOption ); }

				EPauseOptions			GetPreviousValidOption() const;
				EPauseOptions			GetNextValidOption() const;

				bool				IsOptionValid( EPauseOptions option ) const;

				void				OnResume();
				void				OnReset();

	private:
		bool						mIsFinished;

		EPauseOptions					mCurrentOption;

		std::unique_ptr<CUIComponent>				mOptionComponents[ NUM_MENU_OPTIONS ];
};
#endif // UI_PAUSESCREEN_H_
