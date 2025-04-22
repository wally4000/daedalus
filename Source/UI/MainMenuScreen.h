/*
Copyright (C) 2006 StrmnNrmn

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


#ifndef UI_MAINMENUSCREEN_H_
#define UI_MAINMENUSCREEN_H_

#include "UIContext.h"
#include "UIScreen.h"
#include "RomSelectorComponent.h"
#include "SelectedRomComponent.h"
#include "SavestateSelectorComponent.h"
#include "SplashScreen.h"

class CUIContext;

	enum EMenuOption
	{
		MO_GLOBAL_SETTINGS = 0,
		MO_ROMS,
		MO_SELECTED_ROM,
		MO_SAVESTATES,
		MO_ABOUT,
	};
	const s16 NUM_MENU_OPTIONS {MO_ABOUT+1};

	const EMenuOption	MO_FIRST_OPTION [[maybe_unused]] = MO_GLOBAL_SETTINGS;
	const EMenuOption	MO_LAST_OPTION [[maybe_unused]] = MO_ABOUT;


class CMainMenuScreen : public CUIScreen
{
	public:

		CMainMenuScreen( CUIContext * p_context );
		~CMainMenuScreen();

		// CMainMenuScreen
		virtual void				Run();

		// CUIScreen
		virtual void				Update( float elapsed_time, const glm::vec2 & stick, u32 old_buttons, u32 new_buttons );
		virtual void				Render();
		virtual bool				IsFinished() const									{ return mIsFinished; }


	private:
		static	EMenuOption			AsMenuOption( s32 option );

				EMenuOption			GetPreviousOption() const							{ return AsMenuOption( mCurrentOption - 1 ); }
				EMenuOption			GetCurrentOption() const							{ return AsMenuOption( mCurrentOption ); }
				EMenuOption			GetNextOption() const								{ return AsMenuOption( mCurrentOption + 1 ); }

				s32					GetPreviousValidOption() const;
				s32					GetNextValidOption() const;

				bool				IsOptionValid( EMenuOption option ) const;

				void				OnRomSelected( const std::filesystem::path& filename);
				void				OnSavestateSelected( const char * savestate_filename );
				void				OnStartEmulation();

	private:
		bool						mIsFinished;

		s32							mCurrentOption;
		f32							mCurrentDisplayOption;

		std::unique_ptr<CUIComponent>				mOptionComponents[ NUM_MENU_OPTIONS ];
		CSelectedRomComponent* mSelectedRomComponent = nullptr; 

		RomID						mRomID;
};
void DisplayRomsAndChoose(bool show_splash);

#endif // UI_MAINMENUSCREEN_H_
