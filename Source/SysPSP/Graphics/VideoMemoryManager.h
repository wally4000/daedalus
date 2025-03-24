/*
Copyright (C) 2005 StrmnNrmn

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


#ifndef SYSPSP_GRAPHICS_VIDEOMEMORYMANAGER_H_
#define SYSPSP_GRAPHICS_VIDEOMEMORYMANAGER_H_

#include "Base/Types.h"


#include "Utility/VolatileMem.h"
#include "Utility/MemoryHeap.h"
#include "Utility/MathUtil.h"

bool Init_PSP_VideoMemoryManager();
void Destroy_PSP_VideoMemoryManager();

class CVideoMemoryManager
{
public:
	CVideoMemoryManager();
	 ~CVideoMemoryManager();

	 bool			Alloc( u32 size, void ** data, bool * isvidmem );
	 void			Free( void * ptr );
#ifdef DAEDALUS_DEBUG_MEMORY
	 void			DisplayDebugInfo() = 0;
#endif

private:
	std::unique_ptr<CMemoryHeap>	mVideoMemoryHeap;
	std::unique_ptr<CMemoryHeap>	mRamMemoryHeap;
};

extern std::unique_ptr<CVideoMemoryManager> gVideo_MemoryManager;

#endif // SYSPSP_GRAPHICS_VIDEOMEMORYMANAGER_H_
