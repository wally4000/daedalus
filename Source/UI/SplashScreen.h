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


#ifndef UI_SPLASHSCREEN_H_
#define UI_SPLASHSCREEN_H_

class CUIContext;


#include "UIContext.h"
#include "UIScreen.h"

class CSplashScreen : public CUIScreen
{
	public:

		CSplashScreen( CUIContext * p_context );
		~CSplashScreen();

		// CSplashScreen
		virtual void				Run();

		// CUIScreen
		virtual void				Update( float elapsed_time, const glm::vec2 & stick, u32 old_buttons, u32 new_buttons );
		virtual void				Render();
		virtual bool				IsFinished() const									{ return mIsFinished; }
	private:
		bool						mIsFinished;
		float						mElapsedTime;
		std::shared_ptr<CNativeTexture>		mpTexture;
};


#endif // UI_SPLASHSCREEN_H_
