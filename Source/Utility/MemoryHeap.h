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

#pragma once

#ifndef UTILITY_MEMORYHEAP_H_
#define UTILITY_MEMORYHEAP_H_

#include "Base/Types.h"
#include "Utility/MathUtil.h"
#include <memory> 
#include <vector>
#include <iostream>
#include <algorithm>
struct Chunk
{
	u8 *	Ptr;
	u32		Length;
	bool Used;
};

class CMemoryHeap
{
public:
	static std::unique_ptr<CMemoryHeap> Create( u32 size );					
	static std::unique_ptr<CMemoryHeap> Create( void * base_ptr, u32 size );

	CMemoryHeap(u32 size );
	CMemoryHeap( void * base_ptr, u32 size);
	~CMemoryHeap();

	void *		Alloc( u32 size );
	void		Free( void * ptr );
	void		Reset();
	bool		IsFromHeap( void * ptr ) const;
#ifdef DAEDALUS_DEBUG_MEMORY
	//u32		GetAvailableMemory() const;
	void		DisplayDebugInfo() const;
#endif

	private:
		std::unique_ptr<u8[]> mBasePtr;
		u32	mTotalSize = 0;
		u32 mUsedSize = 0;
		std::vector<Chunk> mChunks;
		
#ifdef SHOW_MEM
u32					mMemAlloc;
#endif
};

#endif // UTILITY_MEMORYHEAP_H_
