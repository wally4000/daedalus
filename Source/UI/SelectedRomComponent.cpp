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

#include "Core/ROM.h"
#include "Graphics/ColourValue.h"
#include "Input/InputManager.h"
#include "DrawTextUtilities.h"
#include "AdvancedOptionsScreen.h"
#include "CheatOptionsScreen.h"
#include "Menu.h"
#include "RomPreferencesScreen.h"
#include "SelectedRomComponent.h"



CSelectedRomComponent::~CSelectedRomComponent() {}



CSelectedRomComponent::CSelectedRomComponent( CUIContext * p_context, std::function<void()> on_start_emulation )
:	CUIComponent( p_context )
,	OnStartEmulation( on_start_emulation )
{
	mElements.Add( std::make_unique<CUICommandImpl>( std::bind(&CSelectedRomComponent::EditPreferences, this ), "Edit Preferences", "Edit various preferences for this rom." ) );
	mElements.Add( std::make_unique<CUICommandImpl>( std::bind(&CSelectedRomComponent::AdvancedOptions, this ), "Advanced Options", "Edit advanced options for this rom." ) );
	mElements.Add( std::make_unique<CUICommandImpl>( std::bind(&CSelectedRomComponent::CheatOptions, this ), "Cheats", "Enable and select cheats for this rom." ) );

	mElements.Add(std::make_unique<CUISpacer>( 16 ) );

	u32 i = mElements.Add(std::make_unique<CUICommandImpl>( std::bind(&CSelectedRomComponent::StartEmulation, this ), "Start Emulation", "Start emulating the selected rom." ) );

	mElements.SetSelected( i );
}



void	CSelectedRomComponent::Update( float elapsed_time[[maybe_unused]], const glm::vec2 & stick[[maybe_unused]], u32 old_buttons, u32 new_buttons )
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
		if( element != nullptr )
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


void	CSelectedRomComponent::Render()
{
	mElements.Draw( mpContext, LIST_TEXT_LEFT, LIST_TEXT_WIDTH, AT_CENTRE, BELOW_MENU_MIN );

	auto element = mElements.GetSelectedElement();
	if( element != NULL )
	{
		const auto p_description = element->GetDescription();

		mpContext->DrawTextArea( DESCRIPTION_AREA_LEFT,
								 DESCRIPTION_AREA_TOP,
								 DESCRIPTION_AREA_RIGHT - DESCRIPTION_AREA_LEFT,
								 DESCRIPTION_AREA_BOTTOM - DESCRIPTION_AREA_TOP,
								 p_description,
								 DrawTextUtilities::TextWhite,
								 VA_BOTTOM );
	}
}

void	CSelectedRomComponent::EditPreferences()
{
	auto edit_preferences = std::make_unique<CRomPreferencesScreen>( mpContext, mRomID );
	edit_preferences->Run();
}


void	CSelectedRomComponent::AdvancedOptions()
{
	auto advanced_options = std::make_unique<CAdvancedOptionsScreen>( mpContext, mRomID );
	advanced_options->Run();
}

void	CSelectedRomComponent::CheatOptions()
{
	auto cheat_options = std::make_unique<CCheatOptionsScreen>( mpContext, mRomID );
	cheat_options->Run();
}


void	CSelectedRomComponent::StartEmulation()
{
	if(OnStartEmulation != NULL)
	{
		OnStartEmulation();
	}
}
