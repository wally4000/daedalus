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

#include <stdio.h>

#include "RomPreferencesScreen.h"

#include "Interface/ConfigOptions.h"
#include "Core/ROM.h"
#include "RomFile/RomSettings.h"
#include "Graphics/ColourValue.h"
#include "Input/InputManager.h"
#include "DrawTextUtilities.h"
#include "Menu.h"

#include "Interface/Preferences.h"
#include "System/SystemInit.h"


CRomPreferencesScreen::CRomPreferencesScreen( CUIContext * p_context, const RomID & rom_id )
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

	mElements.Add(std::make_unique<CTextureHashFrequency>( &mRomPreferences.CheckTextureHashFrequency, "Texture Update Check",	"Whether to check for texture updates between frames. Disable this to improve framerate at the expense of graphics quality in some ROMs." ) );
	mElements.Add(std::make_unique<CAdjustFrameskipSetting>( &mRomPreferences.Frameskip, "Frameskip", "This determines how many frames are skipped before rendering a new frame. Increasing this value should give a small speedup, at the expense of more jerky graphics." ) );
	mElements.Add(std::make_unique<CZoomSetting>( &mRomPreferences.ZoomX, "Zoom", "Increase screen size, the value will override the default screen size, 100% is default." ) );
	mElements.Add(std::make_unique<CFSkipSetting>( &mRomPreferences.SpeedSyncEnabled, "Limit Framerate", "Limit the refresh rate to 50/25Hz (PAL) or 60/30Hz (NTSC)." ) );
	mElements.Add(std::make_unique<CBoolSetting>( &mRomPreferences.DynarecEnabled, "Dynamic Recompilation", "Dynamic recompilation gives a considerable speed-up for the ROM emulation.", "Enabled", "Disabled" ) );
	mElements.Add(std::make_unique<CBoolSetting>( &mRomPreferences.PatchesEnabled, "High Level Emulation", "Whether to use replicated OS function calls (faster) instead of emulating the real ones (slower) (WARNING, can cause instability and/or crash on certain ROMs).", "Enabled", "Disabled" ) );
	mElements.Add(std::make_unique<CAudioSetting>( &mRomPreferences.AudioEnabled, "Audio", "Whether or not to enable audio emulation, and whether to process the audio asynchronously(fast/buggy) or synchronously(slow)." ) );
#ifndef DAEDALUS_POSIX // Not implemented in Posix Platforms
	mElements.Add(std::make_unique<CAdjustControllerSetting>( &mRomPreferences.ControllerIndex, "Controller" ) );
#endif
//	mElements.Add( new CUISpacer( 16 ) );

	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&CRomPreferencesScreen::OnConfirm, this ), "Save & Return", "Confirm changes to settings and return." ) );
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&CRomPreferencesScreen::OnCancel, this ), "Cancel", "Cancel changes to settings and return." ) );

}


CRomPreferencesScreen::~CRomPreferencesScreen() {}



void	CRomPreferencesScreen::Update( float elapsed_time[[maybe_unused]], const glm::vec2 & stick[[maybe_unused]], u32 old_buttons, u32 new_buttons )
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

		auto element = mElements.GetSelectedElement();
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

void	CRomPreferencesScreen::Render()
{
	mpContext->ClearBackground();

	u32		font_height = mpContext->GetFontHeight();
	u32		line_height = font_height + 2;

	const auto title_text = "Rom Preferences";
	mpContext->SetFontStyle( CUIContext::FS_HEADING );
	u32		heading_height = mpContext->GetFontHeight();
	auto y = MENU_TOP + heading_height;
	mpContext->DrawTextAlign( LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_CENTRE, y, title_text, mpContext->GetDefaultTextColour() ); y += heading_height;
	mpContext->SetFontStyle( CUIContext::FS_REGULAR );

	y += 2;

	mpContext->DrawTextAlign( LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_CENTRE, y, mRomName.c_str(), mpContext->GetDefaultTextColour() ); y += line_height;

	y += 4;

	mElements.Draw( mpContext, LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_CENTRE, y );

	auto element = mElements.GetSelectedElement();
	if( element != nullptr )
	{
		const auto	 p_description = element->GetDescription();

		mpContext->DrawTextArea( DESCRIPTION_AREA_LEFT,
								 DESCRIPTION_AREA_TOP,
								 DESCRIPTION_AREA_RIGHT - DESCRIPTION_AREA_LEFT,
								 DESCRIPTION_AREA_BOTTOM - DESCRIPTION_AREA_TOP,
								 p_description,
								 DrawTextUtilities::TextWhite,
								 VA_BOTTOM );
	}
}


void	CRomPreferencesScreen::Run()
{
	CUIScreen::Run();
}


void	CRomPreferencesScreen::OnConfirm()
{
	ctx.preferences->SetRomPreferences( mRomID, mRomPreferences );

	ctx.preferences->Commit();

	mRomPreferences.Apply(ctx);

	mIsFinished = true;
}


void	CRomPreferencesScreen::OnCancel()
{
	mIsFinished = true;
}
