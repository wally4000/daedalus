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


#ifndef UI_CHEATOPTIONSSCREEN_H_
#define UI_CHEATOPTIONSSCREEN_H_

#include <memory> 
#include "Interface/RomID.h"
#include "UIContext.h"
#include "UIScreen.h"
#include "UISetting.h"
#include "UISpacer.h"
#include "UICommand.h"
#include "Interface/Cheats.h"
#include "Interface/Preferences.h"

class CUIContext;


class CCheatOptionsScreen : public CUIScreen
{
	public:

		CCheatOptionsScreen( CUIContext * p_context, const RomID & rom_id );
		~CCheatOptionsScreen();
		// CCheatOptionsScreen

		virtual void				Run();

		// CUIScreen
		virtual void				Update( float elapsed_time, const glm::vec2 & stick, u32 old_buttons, u32 new_buttons );
		virtual void				Render();
		virtual bool				IsFinished() const									{ return mIsFinished; }

	private:
		void				OnConfirm();
		void				OnCancel();
		RomID						mRomID;
		std::string					mRomName;
		SRomPreferences				mRomPreferences;
		bool						mIsFinished;
		CUIElementBag				mElements;
};


class CCheatType : public CUISetting
{
public:
	CCheatType( u32 i,const char * name, bool * cheat_enabled, const char * description )
		:	CUISetting( name, description )
		,	mIndex( i )
		,	mCheatEnabled( cheat_enabled )
	{
	}
	// Make read only the cheat list if enable cheat code option is disable
	virtual bool			IsReadOnly() const
	{
		if(!*mCheatEnabled)
		{
			//Disable all active cheat codes
			CheatCodes_Disable( mIndex );
			return true;
		}
		return false;
	}

	virtual bool			IsSelectable()	const	{ return !IsReadOnly(); }

	virtual	void			OnSelected()
	{

		if(!codegrouplist[mIndex].active)
		{
			codegrouplist[mIndex].active = true;
		}
		else
		{
			CheatCodes_Disable( mIndex);
		}

	}
	virtual const char *	GetSettingName() const
	{
		return codegrouplist[mIndex].active ? "Enabled" : "Disabled";
	}

private:
	u32						mIndex;
	bool *					mCheatEnabled;
};


class CCheatNotFound : public CUISetting
	{
	public:
		CCheatNotFound(  const char * name )
			:	CUISetting( name, "" )
		{
		}
		// Always show as read only when no cheats are found
		virtual bool			IsReadOnly()	const	{ return true; }
		virtual bool			IsSelectable()	const	{ return false; }
		virtual	void			OnSelected()			{ }

		//virtual	void			OnSelected(){}

		virtual const char *	GetSettingName() const	{ return "Disabled";	}
	};



#endif // UI_CHEATOPTIONSSCREEN_H_
