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


#include "Base/Types.h"


#include <stdio.h>

#include <string>


#include "Core/CPU.h"
#include "Core/ROM.h"
#include "RomFile/RomSettings.h"
#include "Graphics/ColourValue.h"
#include "Graphics/GraphicsContext.h"
#include "Utility/MathUtil.h"
#include "DrawTextUtilities.h"

#include "AboutComponent.h"
#include "GlobalSettingsComponent.h"
#include "PauseOptionsComponent.h"
#include "PauseScreen.h"

#include "Utility/Translate.h"
#include "Menu.h"

extern void battery_info();

	const char * const	gMenuOptionNames[ NUM_PAUSE_OPTIONS ] =
	{
		"Global Settings",
		"Paused",
		"About",
	};


CPauseScreen::CPauseScreen( CUIContext * p_context )
:	CUIScreen( p_context )
,	mIsFinished( false )
,	mCurrentOption( PO_PAUSE_OPTIONS )
{
	for( auto i = 0; i < NUM_MENU_OPTIONS; ++i )
	{
		mOptionComponents[ i ] = NULL;
	}

	mOptionComponents[ PO_GLOBAL_SETTINGS ]	= std::make_unique<CGlobalSettingsComponent>( mpContext );
	mOptionComponents[ PO_PAUSE_OPTIONS ]	= std::make_unique<CPauseOptionsComponent>( mpContext, std::bind(&CPauseScreen::OnResume, this), std::bind(&CPauseScreen::OnReset, this ) );
	mOptionComponents[ PO_ABOUT ]			= std::make_unique<CAboutComponent>( mpContext );

#ifdef DAEDALUS_ENABLE_ASSERTS
	for( u32 i = 0; i < NUM_MENU_OPTIONS; ++i )
	{
		DAEDALUS_ASSERT( mOptionComponents[ i ] != NULL, "Unhandled screen" );
	}
	#endif
}

CPauseScreen::~CPauseScreen()
{}


EPauseOptions		CPauseScreen::GetPreviousValidOption() const
{
	bool			looped = false;
	EPauseOptions	 current_option = mCurrentOption;

	do
	{
		current_option = GetPreviousOption( current_option );
		looped = current_option == mCurrentOption;
	}
	while( !IsOptionValid( current_option ) && !looped );

	return current_option;
}


EPauseOptions		CPauseScreen::GetNextValidOption() const
{
	bool			looped = false;
	EPauseOptions	current_option = mCurrentOption;

	do
	{
		current_option = GetNextOption( current_option );
		looped = current_option == mCurrentOption;
	}
	while( !IsOptionValid( current_option ) && !looped );

	return current_option;
}


bool	CPauseScreen::IsOptionValid( EPauseOptions option [[maybe_unused]] ) const
{
	return true;
}


void	CPauseScreen::Update( float elapsed_time, const glm::vec2 & stick, u32 old_buttons, u32 new_buttons )
{
	static bool button_released = false;

	if(!(new_buttons & PSP_CTRL_SELECT) && button_released)
	{
		button_released = false;
		mIsFinished = true;
	}

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
		if(new_buttons & PSP_CTRL_SELECT)
		{
			button_released = true;
			new_buttons &= ~PSP_CTRL_SELECT;
		}
	}

	mOptionComponents[ mCurrentOption ]->Update( elapsed_time, stick, old_buttons, new_buttons );
}


void	CPauseScreen::Render()
{
	mpContext->ClearBackground();

	auto y = MENU_TOP;

	c32		valid_colour( mpContext->GetDefaultTextColour() );
	c32		invalid_colour( 200, 200, 200 );

	EPauseOptions		previous = GetPreviousOption();
	EPauseOptions		current = GetCurrentOption();
	EPauseOptions		next = GetNextOption();

	// Meh should be big enough regarding if translated..
	char					info[120];

#if DAEDALUS_PSP
	s32 bat = scePowerGetBatteryLifePercent();
	s32 batteryLifeTime = scePowerGetBatteryLifeTime();
	if(!scePowerIsBatteryCharging())
	{
			snprintf(info, sizeof(info), " [%s %d%% %s %2dh %2dm]",
			Translate_String("Battery / "), bat,
			Translate_String("Time"), batteryLifeTime / 60, batteryLifeTime % 60);

	}
	else
	{
			snprintf(info, sizeof(info), "[%s]" ,
			Translate_String("Battery is Charging"));
	}
#endif

	// Battery Info
	mpContext->SetFontStyle( CUIContext::FS_REGULAR );
	mpContext->DrawTextAlign( 0, SCREEN_WIDTH - LIST_TEXT_LEFT, AT_RIGHT, CATEGORY_TEXT_TOP, info, DrawTextUtilities::TextWhiteDisabled, DrawTextUtilities::TextBlueDisabled );
	
	auto p_option_text = gMenuOptionNames[ previous ];
	mpContext->DrawTextAlign( LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_LEFT, y + mpContext->GetFontHeight(), p_option_text, IsOptionValid( previous ) ? valid_colour : invalid_colour );

	mpContext->SetFontStyle( CUIContext::FS_HEADING );
	p_option_text = gMenuOptionNames[ current ];
	mpContext->DrawTextAlign( LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_CENTRE, y + mpContext->GetFontHeight(), p_option_text, IsOptionValid( current ) ? valid_colour : invalid_colour );
	mpContext->SetFontStyle( CUIContext::FS_REGULAR );

	p_option_text = gMenuOptionNames[ next ];
	mpContext->DrawTextAlign( LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_RIGHT, y + mpContext->GetFontHeight(), p_option_text, IsOptionValid( next ) ? valid_colour : invalid_colour );

	mOptionComponents[ mCurrentOption ]->Render();

}

void	CPauseScreen::Run()
{
	mIsFinished = false;
	CUIScreen::Run();

#ifdef DAEDALUS_PSP
	ctx.graphicsContext->SwitchToChosenDisplay();
#endif
	ctx.graphicsContext->ClearAllSurfaces();
}

void CPauseScreen::OnResume()
{
	mIsFinished = true;
}

void CPauseScreen::OnReset()
{
	CPU_Halt("Resetting");
	mIsFinished = true;
}
