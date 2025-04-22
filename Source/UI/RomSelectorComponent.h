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


#ifndef UI_ROMSELECTORCOMPONENT_H_
#define UI_ROMSELECTORCOMPONENT_H_

#include "UIComponent.h"
#include <functional>

#include "UIContext.h"
#include "UIScreen.h"

#include "Menu.h"

namespace
{

#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_STATIC_ASSERT( std::size( gCategoryLetters ) == NUM_CATEGORIES +1 );
#endif
	ECategory		GetCategory( char c )
	{
		if (c < 0)
			return C_UNK;

		if( std::isalpha( c ))
		{
			c = std::tolower( c );
			return ECategory( C_A + (c - 'a') );
		}
		else if(std::isdigit(c) )
		{
			return C_NUMBERS;
		}
		else
		{
			return C_UNK;
		}
	}

	char	GetCategoryLetter( ECategory category )
	{
#ifdef DAEDALUS_ENABLE_ASSERTS
		DAEDALUS_ASSERT( category >= 0 && category < NUM_CATEGORIES, "Invalid category" );
		#endif
		return gCategoryLetters[ category ];
	}

}

struct SRomInfo : public RomInfo 
{
	std::filesystem::path	mFilename;
	RomID			mRomID;
	u32				mRomSize;
	ECicType		mCicType;
	RomSettings		mSettings;

	SRomInfo( const std::filesystem::path& filename )
		:	mFilename( filename )
	{
		if ( ROM_GetRomDetailsByFilename( filename, &mRomID, &mRomSize, &mCicType ) )
		{
			if ( !ctx.romSettingsDB->GetSettings( mRomID, &mSettings ) )
			{
				// Create new entry, add
				mSettings.Reset();
				mSettings.Comment = "Unknown";

				// Get internal file name for rom from header, otherwise get filename
				if ( !ROM_GetRomName( filename, mSettings.GameName ) )
				{
					mSettings.GameName = filename.string();
				}
				mSettings.GameName = mSettings.GameName.substr(0,63);
				ctx.romSettingsDB->SetSettings( mRomID, mSettings );
			}
		}
		else
		{
			mSettings.GameName = "Can't get rom info";
		}

	}
};




class CRomSelectorComponent : public CUIComponent
{
	public:

		CRomSelectorComponent( CUIContext * p_context, std::function<void(const std::filesystem::path&)> on_rom_selected );
		~CRomSelectorComponent();

		// CUIComponent
		virtual void				Update( float elapsed_time, const glm::vec2 & stick, u32 old_buttons, u32 new_buttons );
		virtual void				Render();
		static std::unique_ptr<CRomSelectorComponent>	Create( CUIContext * p_context, std::function<void(const std::filesystem::path&)> on_rom_selected );
	private:
				void				UpdateROMList();
				void				RenderPreview();
				void				RenderRomList();
				void				RenderCategoryList();

				void				AddRomDirectory(const std::filesystem::path& p_roms_dir, std::vector<std::unique_ptr<SRomInfo>> & roms);

				ECategory			GetCurrentCategory() const;

				void				DrawInfoText( CUIContext * p_context, s32 y, const char * field_txt, const char * value_txt );

	private:
		std::function<void(const std::filesystem::path&)> mOnRomSelected;
		std::vector<std::unique_ptr<SRomInfo>>		mRomsList;
		std::map< ECategory, u32> 	mRomCategoryMap;
		s32							mCurrentScrollOffset;
		float						mSelectionAccumulator;
		std::string					mSelectedRom;

		bool						mDisplayFilenames;
	//	bool						mDisplayInfo;

		std::shared_ptr<CNativeTexture>		mpPreviewTexture;
		u32							mPreviewIdx;
		float						mPreviewLoadedTime;		// How long the preview has been loaded (so we can fade in)
		float						mTimeSinceScroll;		

		bool						mRomDelete;
};

#endif // UI_ROMSELECTORCOMPONENT_H_
