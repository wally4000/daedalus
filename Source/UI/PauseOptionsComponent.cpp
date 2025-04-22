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


#include "Core/CPU.h"
#include "Core/ROM.h"
#include "Interface/SaveState.h"
#include "HLEGraphics/DisplayListDebugger.h"
#include "Graphics/ColourValue.h"
#include "Graphics/GraphicsContext.h"
#include "DrawTextUtilities.h"
#include "AdvancedOptionsScreen.h"
#include "SavestateSelectorComponent.h"
#include "CheatOptionsScreen.h"
#include "Dialogs.h"
#include "PauseOptionsComponent.h"
#include "Menu.h"
#include "RomPreferencesScreen.h"




extern bool gTakeScreenshot;
extern bool gTakeScreenshotSS;




CPauseOptionsComponent::CPauseOptionsComponent( CUIContext * p_context,  std::function<void()> on_resume, std::function<void()> on_reset)
:	CUIComponent( p_context )
,	mOnResume( on_resume )
,	mOnReset( on_reset )
{
	mElements.Add(std::make_unique<CUISpacer>( 10 ) );
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&CPauseOptionsComponent::EditPreferences, this), "Edit Preferences", "Edit various preferences for this rom."));
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&CPauseOptionsComponent::AdvancedOptions, this ), "Advanced Options", "Edit advanced options for this rom." ) );
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&CPauseOptionsComponent::CheatOptions, this ), "Cheats", "Edit advanced options for this rom." ) );
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&CPauseOptionsComponent::SaveState, this ), "Save State", "Save the current state." ) );
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&CPauseOptionsComponent::LoadState, this ), "Load/Delete State", "Restore or delete a previously saved state." ) );
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&CPauseOptionsComponent::TakeScreenshot,this ), "Take Screenshot", "Take a screenshot on resume." ) );

#if defined (DAEDALUS_DEBUG_DISPLAYLIST) && defined (DAEDALUS_PSP)
		mElements.Add( std::make_unique<CUICommandImpl>( std::bind(&CPauseOptionsComponent::DebugDisplayList, this ), "Debug Display List", "Debug display list on resume." ) );
#endif

#ifdef DAEDALUS_DEBUG_CONSOLE
	//mElements.Add( new CUICommandImpl(std::bind(&CPauseOptionsComponent::CPU_DumpFragmentCache, this ), "Dump Fragment Cache", "Dump the contents of the dynarec fragment cache to disk." ) );
	//	mElements.Add( new CUICommandImpl(std::bind(&CPauseOptionsComponent::CPU_ResetFragmentCache, this ), "Clear Fragment Cache", "Clear the contents of the dynarec fragment cache." ) );
	//	mElements.Add( new CUICommandImpl(std::bind(&CPauseOptionsComponent::CPauseOptionsComponent::ProfileNextFrame, this ), "Profile Frame", "Profile the next frame on resume." ) );
#endif

	mElements.Add(std::make_unique<CUISpacer>( 16 ) );
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&CPauseOptionsComponent::OnResume, this ), "Resume Emulation", "Resume emulation." ) );

#ifdef DAEDALUS_DIALOGS
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&CPauseOptionsComponent::ExitConfirmation, this ), "Return to Main Menu", "Return to the main menu." ) );
#else
	mElements.Add(std::make_unique<CUICommandImpl>(std::bind(&CPauseOptionsComponent::OnReset, this ), "Return to Main Menu", "Return to the main menu." ) );
#endif
}

CPauseOptionsComponent::~CPauseOptionsComponent() {}


void	CPauseOptionsComponent::Update( float elapsed_time [[maybe_unused]], const glm::vec2 & stick [[maybe_unused]], u32 old_buttons, u32 new_buttons )
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


void	CPauseOptionsComponent::Render()
{

	mElements.Draw( mpContext, LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_CENTRE, BELOW_MENU_MIN );

	auto element = mElements.GetSelectedElement();
	if( element != nullptr )
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
#ifdef DAEDALUS_DIALOGS

void CPauseOptionsComponent::ExitConfirmation()
{
	if(gShowDialog->Render( mpContext,"Return to main menu?", false) )
	{
		(mOnReset)();
	}
}
#endif

void CPauseOptionsComponent::OnResume() { (mOnResume)(); }


void CPauseOptionsComponent::OnReset()
{
	(mOnReset)();
}


void	CPauseOptionsComponent::EditPreferences()
{
	auto	edit_preferences = std::make_unique<CRomPreferencesScreen>( mpContext, ctx.romInfo->mRomID );
	edit_preferences->Run();
}


void	CPauseOptionsComponent::AdvancedOptions()
{
	auto advanced_options = std::make_unique<CAdvancedOptionsScreen>( mpContext, ctx.romInfo->mRomID );
	advanced_options->Run();
}

void	CPauseOptionsComponent::CheatOptions()
{
	auto cheat_options = std::make_unique<CCheatOptionsScreen>( mpContext, ctx.romInfo->mRomID );
	cheat_options->Run();
}


void	CPauseOptionsComponent::SaveState()
{
auto onSaveStateSlotSelected = [this](const std::filesystem::path slot) { this->OnSaveStateSlotSelected(slot);
};

auto component = std::make_unique<CSavestateSelectorComponent>(mpContext, CSavestateSelectorComponent::AT_SAVING, onSaveStateSlotSelected, ctx.romInfo->settings.GameName.c_str());

	auto screen = std::make_unique<CUIComponentScreen>( mpContext, std::move(component), SAVING_TITLE_TEXT );
	screen->Run();
	(mOnResume)();
}


void	CPauseOptionsComponent::LoadState()
{
auto onLoadStateSlotSelected = [this](const std::filesystem::path slot) {
    this->OnLoadStateSlotSelected(slot);
};

auto component = std::make_unique<CSavestateSelectorComponent>(mpContext, CSavestateSelectorComponent::AT_LOADING, onLoadStateSlotSelected, ctx.romInfo->settings.GameName.c_str());

	auto screen =  std::make_unique<CUIComponentScreen>( mpContext, std::move(component), LOADING_TITLE_TEXT );
	screen->Run();
	(mOnResume)();
}


void	CPauseOptionsComponent::OnSaveStateSlotSelected( const std::filesystem::path& filename )
{
	std::filesystem::remove( filename ); // Ensure that we're re-creating the file
	CPU_RequestSaveState( filename );

	gTakeScreenshot = true;
	gTakeScreenshotSS = true;
}


void	CPauseOptionsComponent::OnLoadStateSlotSelected( const std::filesystem::path& filename )
{
	CPU_RequestLoadState( filename );
}


void CPauseOptionsComponent::TakeScreenshot()
{
	gTakeScreenshot = true;
	(mOnResume)();
}

#ifdef DAEDALUS_DEBUG_DISPLAYLIST

void CPauseOptionsComponent::DebugDisplayList()
{
	DLDebugger_RequestDebug();
	(mOnResume)();
}
#endif
