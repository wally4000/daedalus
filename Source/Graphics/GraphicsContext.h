/*
Copyright (C) 2001 StrmnNrmn

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

#pragma once

#ifndef GRAPHICS_GRAPHICSCONTEXT_H_
#define GRAPHICS_GRAPHICSCONTEXT_H_

#include "Base/Types.h"
#include <memory> 

class c32;


class CGraphicsContext 
{
public:
	static bool Create(); 
	static void Destroy(); 
	virtual ~CGraphicsContext() {}

	enum ETargetSurface
	{
		TS_BACKBUFFER,
		TS_FRONTBUFFER,
	};

	virtual bool Initialise() = 0;

	virtual bool IsInitialised() const = 0;

	virtual void SwitchToChosenDisplay() {};
	virtual void SwitchToLcdDisplay() {};
	virtual void StoreSaveScreenData() {};

#ifdef DAEDALUS_CTR
	virtual void ResetVertexBuffer() = 0;
#endif
#ifdef DAEDALUS_PSP
	virtual void CleanupDisplayLists() {} 
#endif
	virtual void ClearAllSurfaces() = 0;
	virtual void ClearToBlack() = 0;
	virtual void ClearZBuffer() = 0;
	virtual void ClearColBuffer(const c32 & colour) = 0;
	virtual void ClearColBufferAndDepth(const c32 & colour) = 0;

	virtual	void BeginFrame() = 0;
	virtual void EndFrame() = 0;
	virtual void UpdateFrame( bool wait_for_vbl ) = 0;

	virtual void GetScreenSize(u32 * width, u32 * height) const = 0;
	virtual void ViewportType(u32 * width, u32 * height) const = 0;

	virtual void SetDebugScreenTarget( ETargetSurface buffer ) = 0;

	virtual void DumpNextScreen() = 0;
	virtual void DumpScreenShot() = 0;
};

#endif // GRAPHICS_GRAPHICSCONTEXT_H_
