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


#ifndef UTILITY_ROMFILEMEMORY_H_
#define UTILITY_ROMFILEMEMORY_H_

#include "Base/Types.h"

#include "Utility/MemoryHeap.h"

class CROMFileMemory
{
public:
	static CROMFileMemory& Get();
	void* Alloc( u32 size );
	void Free( void * ptr );

private:
	~CROMFileMemory();
	CROMFileMemory();
	CROMFileMemory(const CROMFileMemory&) = delete;
	CROMFileMemory& operator=(const CROMFileMemory&) = delete;

	#ifdef DAEDALUS_PSP
		CMemoryHeap *	mRomMemoryHeap;
	#endif
};


#endif // UTILITY_ROMFILEMEMORY_H_
