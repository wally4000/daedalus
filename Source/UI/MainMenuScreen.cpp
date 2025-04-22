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


#include "Base/Types.h"


#include <string>
#include <filesystem>

#include "Core/CPU.h"
#include "Core/ROM.h"
#include "RomFile/RomSettings.h"
#include "Interface/SaveState.h"
#include "Graphics/ColourValue.h"
#include "Graphics/GraphicsContext.h"
#include "Utility/MathUtil.h"
#include "DrawTextUtilities.h"
#include "AboutComponent.h"
#include "GlobalSettingsComponent.h"
#include "MainMenuScreen.h"
#include "Menu.h"



#include "System/SystemInit.h"
#include "Interface/Preferences.h"



const char * const	gMenuOptionNames[ NUM_MENU_OPTIONS ] =
{
	"Global Settings",
	"Roms",
	"Selected Rom",
	"Savestates",
	"About",
};



CMainMenuScreen::CMainMenuScreen(CUIContext * p_context)
: CUIScreen(p_context)
, mIsFinished(false)
, mCurrentOption(MO_ROMS)
, mCurrentDisplayOption(mCurrentOption)
{
	// Create components
	mOptionComponents[MO_SELECTED_ROM] = std::make_unique<CSelectedRomComponent>(mpContext, [this]() {this->OnStartEmulation(); });
	
	mSelectedRomComponent = static_cast<CSelectedRomComponent*>(mOptionComponents[MO_SELECTED_ROM].get());
	mOptionComponents[MO_GLOBAL_SETTINGS] = std::make_unique<CGlobalSettingsComponent>(mpContext);

	mOptionComponents[MO_ROMS] = std::make_unique<CRomSelectorComponent>(mpContext, [this](const std::filesystem::path& rom) { this->OnRomSelected(rom); });
	
	mOptionComponents[MO_SAVESTATES] = std::make_unique<CSavestateSelectorComponent>(mpContext,
		CSavestateSelectorComponent::AT_LOADING, [this](const char* savestate) { this->OnSavestateSelected(savestate); }, std::filesystem::path()	);

	mOptionComponents[MO_ABOUT] = std::make_unique<CAboutComponent>(mpContext);
}

CMainMenuScreen::~CMainMenuScreen() {}

EMenuOption	CMainMenuScreen::AsMenuOption( s32 option )
{
	s32 m = option % static_cast<s32>(NUM_MENU_OPTIONS);
	if( m < 0 )
		m += NUM_MENU_OPTIONS;

#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( m >= 0 && m < (s32)NUM_MENU_OPTIONS, "Whoops" );
#endif
	return EMenuOption( m );
}


s32	CMainMenuScreen::GetPreviousValidOption() const
{
	bool		looped =  false;
	s32			current_option =  mCurrentOption;
	EMenuOption	initial_option = AsMenuOption( current_option );

	do
	{
		current_option--;
		looped = AsMenuOption( current_option ) == initial_option;
	}
	while( !IsOptionValid( AsMenuOption( current_option ) ) && !looped );

	return current_option;
}


s32	CMainMenuScreen::GetNextValidOption() const
{
	bool			looped =  false;
	s32			current_option = mCurrentOption;
	EMenuOption	initial_option = AsMenuOption( current_option );

	do
	{
		current_option++;
		looped = AsMenuOption( current_option ) == initial_option;
	}
	while( !IsOptionValid( AsMenuOption( current_option ) ) && !looped );

	return current_option;
}


//

bool CMainMenuScreen::IsOptionValid(EMenuOption option) const
{
	if (option == MO_SELECTED_ROM)
	{
		return ctx.romInfo && !ctx.romInfo->mFileName.empty();
	}
	return true;
}


//

void	CMainMenuScreen::Update( float elapsed_time, const glm::vec2 & stick, u32 old_buttons, u32 new_buttons )
{
	if(old_buttons != new_buttons)
	{
		if(new_buttons & PSP_CTRL_LTRIGGER)
		{
			mCurrentOption = GetPreviousValidOption();
			new_buttons &= ~PSP_CTRL_LTRIGGER;
		}
		if(new_buttons & PSP_CTRL_RTRIGGER)
		{
			mCurrentOption = GetNextValidOption();
			new_buttons &= ~PSP_CTRL_RTRIGGER;
		}
	}

	// Approach towards target
	mCurrentDisplayOption = mCurrentDisplayOption + (mCurrentOption - mCurrentDisplayOption) * 0.1f;

	mOptionComponents[ GetCurrentOption() ]->Update( elapsed_time, stick, old_buttons, new_buttons );
}


//

void	CMainMenuScreen::Render()
{
	mpContext->ClearBackground();


	c32		valid_colour( mpContext->GetDefaultTextColour() );
	c32		invalid_colour( 200, 200, 200 );

	f32		min_scale =  0.60f;
	f32		max_scale =  1.0f;

	s32		SCREEN_LEFT = 20;
	s32		SCREEN_RIGHT = SCREEN_WIDTH - 20;

	mpContext->SetFontStyle( CUIContext::FS_HEADING );

	s32		y = MENU_TOP + mpContext->GetFontHeight();

	for( s32 i = -2; i <= 2; ++i )
	{
		EMenuOption		option =  AsMenuOption( mCurrentOption + i );
		c32				text_col( IsOptionValid( option ) ? valid_colour : invalid_colour );
		const char *	option_text =  gMenuOptionNames[ option ];
		u32				text_width =  mpContext->GetTextWidth( option_text );

		f32				diff =  static_cast<f32>( mCurrentOption + i ) - mCurrentDisplayOption;
		f32				dist = fabsf( diff );

		s32				centre = ( SCREEN_WIDTH - text_width ) / 2;
		s32				extreme = diff < 0 ? SCREEN_LEFT : static_cast<s32>( SCREEN_RIGHT - (text_width * min_scale) );

		// Interpolate between central and extreme position and centre
		f32				scale =  max_scale + (min_scale - max_scale) * dist;
		s32				x = s32( centre + (extreme - centre) * dist );

		mpContext->DrawTextScale( x, y, scale, option_text, text_col );
	}

	mpContext->SetFontStyle( CUIContext::FS_REGULAR );

	mOptionComponents[ GetCurrentOption() ]->Render();
}


void	CMainMenuScreen::Run()
{
	mIsFinished = false;
	CUIScreen::Run();

	// switch back to the emulator display
	ctx.graphicsContext->SwitchToChosenDisplay();

	// Clear everything to black - looks a bit tidier
	ctx.graphicsContext->ClearAllSurfaces();
}

void CMainMenuScreen::OnRomSelected(const std::filesystem::path& rom_filename)
{
	u32			rom_size;
	ECicType	boot_type;

	std::filesystem::path abs_path = std::filesystem::absolute(rom_filename);

	if (ROM_GetRomDetailsByFilename(abs_path, &mRomID, &rom_size, &boot_type))
	{
		if (!ctx.romInfo)
			ctx.romInfo = std::make_unique<SRomInfo>(abs_path);
		else
			ctx.romInfo->mFileName = abs_path;
		mSelectedRomComponent->SetRomID(mRomID);
		mCurrentOption = MO_SELECTED_ROM;
		mCurrentDisplayOption = float(mCurrentOption);
	}
	else
	{
		std::cerr << "[ERROR] Failed to get ROM details for: " << abs_path << std::endl;
		if (ctx.romInfo)
			ctx.romInfo->mFileName.clear();
	}
}
// This feature is not really stable

void	CMainMenuScreen::OnSavestateSelected( const char * savestate_filename )
{
	// If the CPU is running we need to queue a request to load the state
	// (and possibly switch roms). Otherwise we just load the rom directly
	if( CPU_IsRunning() )
	{
		if( CPU_RequestLoadState( savestate_filename ) )
		{
			mIsFinished = true;
		}
		else
		{
			// Should report some kind of error
		}
	}
	else
	{
		const std::filesystem::path rom_filename = SaveState_GetRom(savestate_filename);

		System_Open(rom_filename);

		if( !SaveState_LoadFromFile( savestate_filename ) )
		{
			// Should report some kind of error
		}

		mIsFinished = true;
	}
}


void	CMainMenuScreen::OnStartEmulation()
{	
	System_Open(ctx.romInfo->mFileName );
	mIsFinished = true;
}

void DisplayRomsAndChoose(bool show_splash)
{
	// switch back to the LCD display
	ctx.graphicsContext->SwitchToLcdDisplay();

	auto p_context =  CUIContext::Create();

	if(p_context)
	{
		p_context->ClearBackground(c32::Red);

		if( show_splash )
		{
			auto p_splash = std::make_unique<CSplashScreen>( p_context );
			p_splash->Run();
		}

		auto p_main_menu = std::make_unique<CMainMenuScreen>( p_context );
		p_main_menu->Run();
	}

	delete p_context;
}

