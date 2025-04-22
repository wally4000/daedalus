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

#include "CheatOptionsScreen.h"
#include "Menu.h"

#include "Interface/ConfigOptions.h"
#include "Core/ROM.h"

#include "RomFile/RomSettings.h"
#include "Graphics/ColourValue.h"
#include "Input/InputManager.h"
#include "DrawTextUtilities.h"

#include "Utility/Stream.h"


CCheatOptionsScreen::CCheatOptionsScreen( CUIContext * p_context, const RomID & rom_id )
:	CUIScreen( p_context )
,	mRomID( rom_id )
,	mRomName( "?" )
,	mIsFinished( false )
{
	ctx.preferences->GetRomPreferences( mRomID, &mRomPreferences );

	RomSettings			settings;
	if ( ctx.romSettingsDB->GetSettings( rom_id, &settings ) )
	{
 		mRomName = settings.GameName;
	}

	// HACK: if cheatcode feature is forced in roms.ini, force preference too
	// This is done mainly to reflect that cheat option is being forced
	// We should handle this in preferences.cpp, since is kind of confusing for the user that forced options in roms.ini don't reflect it in preferences menu
	if(settings.CheatsEnabled)
		mRomPreferences.CheatsEnabled = true;

	// Read hack code for this rom
	// We always parse the cheat file when the cheat menu is accessed, to always have cheats ready to be used by the user without hassle
	// Also we do this to make sure we clear any non-associated cheats, we only parse once per ROM access too :)
	//
	CheatCodes_Read( mRomName.c_str(), "Daedalus.cht", mRomID.CountryID );

	mElements.Add(std::make_unique<CBoolSetting>( &mRomPreferences.CheatsEnabled, "Enable Cheat Codes", "Whether to use cheat codes for this ROM", "Yes", "No" ) );

	// ToDo: add a dialog if cheatcodes were truncated, aka MAX_CHEATCODE_PER_GROUP is reached
	for(u32 i = 0; i < MAX_CHEATCODE_PER_LOAD; i++)
	{
		// Only display the cheat list when the cheatfile been loaded correctly and there were cheats found
		if(codegroupcount > 0 && codegroupcount > i)
		{
			// Generate list of available cheatcodes
			mElements.Add(std::make_unique<CCheatType>( i, codegrouplist[i].name, &mRomPreferences.CheatsEnabled, codegrouplist[i].note ) );
		}
		else
		{
			//mElements.Add( new CCheatNotFound("No cheat codes found for this entry", "Make sure codes are formatted correctly for this entry. Daedalus supports a max of eight cheats per game." ) );
			mElements.Add(std::make_unique<CCheatNotFound>("No cheat codes found for this entry" ) );
		}
	}


	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&CCheatOptionsScreen::OnConfirm, this ), "Save & Return", "Confirm changes to settings and return." ) );
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&CCheatOptionsScreen::OnCancel, this ), "Cancel", "Cancel changes to settings and return." ) );

}


//

CCheatOptionsScreen::~CCheatOptionsScreen()
{
}


//

void	CCheatOptionsScreen::Update( float elapsed_time [[maybe_unused]], const glm::vec2 & stick [[maybe_unused]], u32 old_buttons, u32 new_buttons )
{
	if(old_buttons != new_buttons)
	{
		if( new_buttons & PSP_CTRL_UP )
		{
			mElements.SelectPrevious();
		}
		if( new_buttons & PSP_CTRL_DOWN )
		{
			mElements.SelectNext();
		}

		auto	element = mElements.GetSelectedElement();
		if( element != NULL )
		{
			if( new_buttons & PSP_CTRL_LEFT )
			{
				element->OnPrevious();
			}
			if( new_buttons & PSP_CTRL_RIGHT )
			{
				element->OnNext();
			}
			if( new_buttons & (PSP_CTRL_CROSS|PSP_CTRL_START) )
			{
				element->OnSelected();
			}
		}
	}
}


void	CCheatOptionsScreen::Render()
{
	mpContext->ClearBackground();


 s16		y;

	const auto title_text = "Cheat Options";
	mpContext->SetFontStyle( CUIContext::FS_HEADING );
	u32		heading_height = mpContext->GetFontHeight();
	y = MENU_TOP + heading_height;
	mpContext->DrawTextAlign( LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_CENTRE, y, title_text, mpContext->GetDefaultTextColour() ); y += heading_height;
	mpContext->SetFontStyle( CUIContext::FS_REGULAR );

	y += 6;

	// Very basic scroller for cheats, note ROM title is disabled since it overlaps when scrolling - FIX ME
	if( mElements.GetSelectedIndex() > 1 )
		mElements.Draw( mpContext, LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_CENTRE, BELOW_MENU_MIN - mElements.GetSelectedIndex()*11 );
	else
		mElements.Draw( mpContext, LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_CENTRE, BELOW_MENU_MIN - mElements.GetSelectedIndex());

	//mElements.Draw( mpContext, TEXT_AREA_LEFT, TEXT_AREA_RIGHT, AT_CENTRE, y );

	auto	element = mElements.GetSelectedElement();
	if( element != NULL )
	{
		const auto	p_description = element->GetDescription();

		mpContext->DrawTextArea( DESCRIPTION_AREA_LEFT,
								 DESCRIPTION_AREA_TOP,
								 DESCRIPTION_AREA_RIGHT - DESCRIPTION_AREA_LEFT,
								 DESCRIPTION_AREA_BOTTOM - DESCRIPTION_AREA_TOP,
								 p_description,
								 DrawTextUtilities::TextWhite,
								 VA_BOTTOM );
	}
}


//

void	CCheatOptionsScreen::Run()
{
	CUIScreen::Run();
}


void	CCheatOptionsScreen::OnConfirm()
{
	ctx.preferences->SetRomPreferences( mRomID, mRomPreferences );

	ctx.preferences->Commit();

	mRomPreferences.Apply(ctx);

	mIsFinished = true;
}

void	CCheatOptionsScreen::OnCancel()
{
	mIsFinished = true;
}
