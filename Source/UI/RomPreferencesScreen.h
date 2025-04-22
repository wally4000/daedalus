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


#ifndef UI_ROMPREFERENCESSCREEN_H_
#define UI_ROMPREFERENCESSCREEN_H_

#include "UIContext.h"
#include "UIScreen.h"
#include "UISetting.h"
#include "UISpacer.h"
#include "UICommand.h"

namespace
{

	class CTextureHashFrequency : public CUISetting
	{
	public:
		CTextureHashFrequency( ETextureHashFrequency * setting, const std::string name, const char * description )
			:	CUISetting( name, description )
			,	mSetting( setting )
		{
		}

		virtual	void			OnNext()				{ *mSetting = ETextureHashFrequency( ( *mSetting + 1 ) % NUM_THF ); }
		virtual	void			OnPrevious()			{ *mSetting = ETextureHashFrequency( ( *mSetting + NUM_THF - 1 ) % NUM_THF ); }

		virtual const char *	GetSettingName() const	{ return Preferences_GetTextureHashFrequencyDescription( *mSetting ); }

	private:
		ETextureHashFrequency *	mSetting;
	};



	class CAdjustControllerSetting : public CUISetting
	{
	public:
		CAdjustControllerSetting( u32 * setting, const char * name )
			:	CUISetting( name, "" )
			,	mSetting( setting )
		{
		}

		virtual	void			OnNext()				{ *mSetting = (*mSetting + 1) % ctx.inputManager->GetNumConfigurations(); }
		virtual	void			OnPrevious()			{ *mSetting = (*mSetting + ctx.inputManager->GetNumConfigurations() - 1) % ctx.inputManager->GetNumConfigurations(); }

		virtual const char *	GetSettingName() const	{ return ctx.inputManager->GetConfigurationName( *mSetting ); }
		virtual const std::string	GetDescription() const	{ return ctx.inputManager->GetConfigurationDescription( *mSetting ); }

	private:
		u32 *					mSetting;
	};


	class CAudioSetting : public CUISetting
	{
	public:
		CAudioSetting( EAudioPluginMode * setting, const char * name, const char * description )
			:	CUISetting( name, description )
			,	mSetting( setting )
		{
		}

		virtual	void			OnNext()				{ *mSetting = (*mSetting < APM_ENABLED_SYNC) ? static_cast<EAudioPluginMode>(*mSetting + 1) : APM_DISABLED; }
		virtual	void			OnPrevious()			{ *mSetting = (*mSetting > APM_DISABLED)     ? static_cast<EAudioPluginMode>(*mSetting - 1) : APM_ENABLED_SYNC; }

		virtual const char *	GetSettingName() const
		{
			switch ( *mSetting )
			{
				case APM_DISABLED:			return "Disabled";
				case APM_ENABLED_ASYNC:		return "Asynchronous";
				case APM_ENABLED_SYNC:		return "Synchronous";
			}
			return "Unknown";
		}

	private:
		EAudioPluginMode		*mSetting;
	};

	class CZoomSetting : public CUISetting
	{
	public:
		CZoomSetting( f32 * setting, const char * name, const char * description )
			:	CUISetting( name, description )
			,	mSetting( setting )
		{
		}

		virtual	void			OnNext()				{ *mSetting += 0.01f; *mSetting = *mSetting > 1.5f ? 1.5f : *mSetting;}
		virtual	void			OnPrevious()			{ *mSetting -= 0.01f; *mSetting = *mSetting < 1.0f ? 1.0f : *mSetting;}

		virtual const char *	GetSettingName() const	{ snprintf( mString, sizeof(mString), "%.0f%%", (double)(*mSetting*100.0f) ); return mString; }

	private:
		float *			mSetting;
		mutable char 	mString[8];
	};



	class CAdjustFrameskipSetting : public CUISetting
	{
	public:
		CAdjustFrameskipSetting( EFrameskipValue * setting, const char * name, const char * description )
			:	CUISetting( name, description )
			,	mSetting( setting )
		{
		}

		virtual	void			OnNext()				{ *mSetting = EFrameskipValue( (*mSetting + 1) % NUM_FRAMESKIP_VALUES ); }
		virtual	void			OnPrevious()			{ *mSetting = EFrameskipValue( (*mSetting + NUM_FRAMESKIP_VALUES - 1) % NUM_FRAMESKIP_VALUES ); }

		virtual const char *	GetSettingName() const	{ return Preferences_GetFrameskipDescription( *mSetting ); }

	private:
		EFrameskipValue *		mSetting;
	};

	class CFSkipSetting : public CUISetting
	{
	public:
		CFSkipSetting( u32 * setting, const char * name, const char * description )
			:	CUISetting( name, description )
			,	mSetting( setting )
		{
		}

		virtual	void			OnNext()				{ *mSetting = (*mSetting < 2) ? (*mSetting + 1) : 0; }
		virtual	void			OnPrevious()			{ *mSetting = (*mSetting > 0) ? (*mSetting - 1) : 2; }

		virtual const char *	GetSettingName() const
		{
			switch ( *mSetting )
			{
				case 0:		return "No";
				case 1:		return "Full speed";
				case 2:		return "Half speed";
			}
			return "?";
		}

	private:
		u32		*mSetting;
	};
}


//

class CRomPreferencesScreen :  public CUIScreen
{
	public:

		CRomPreferencesScreen( CUIContext * p_context, const RomID & rom_id );
		~CRomPreferencesScreen();

		// CRomPreferencesScreen
		virtual void				Run();

		// CUIScreen
		virtual void				Update( float elapsed_time, const glm::vec2 & stick, u32 old_buttons, u32 new_buttons );
		virtual void				Render();
		virtual bool				IsFinished() const									{ return mIsFinished; }
		static std::unique_ptr<CRomPreferencesScreen>	Create( CUIContext * p_context, const RomID & rom_id );
	private:
				void				OnConfirm();
				void				OnCancel();
				void				ResetDefaults();

	private:
		RomID						mRomID;
		std::string					mRomName;
		SRomPreferences				mRomPreferences;

		bool						mIsFinished;

		CUIElementBag				mElements;
};

#endif // UI_ROMPREFERENCESSCREEN_H_
