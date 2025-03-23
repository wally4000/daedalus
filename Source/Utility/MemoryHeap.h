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
struct Chunk
{
	u8 *	Ptr;
	u32		Length;
#ifdef DAEDALUS_DEBUG_MEMORY
	u32		Tag;
#endif
};



class CMemoryHeap
{
public:
	static std::unique_ptr<CMemoryHeap> Create( u32 size );						// Allocate and manage a new region of this size
	static std::unique_ptr<CMemoryHeap> Create( void * base_ptr, u32 size );		// Manage this region of pre-allocated memory

	CMemoryHeap(u32 size );
	CMemoryHeap( void * base_ptr, u32 size);
	~CMemoryHeap();
	void *		Alloc( u32 size );
	void		Free( void * ptr );

	bool		IsFromHeap( void * ptr ) const;		// Does this chunk of memory belong to this heap?
#ifdef DAEDALUS_DEBUG_MEMORY
	//u32		GetAvailableMemory() const;
	void		DisplayDebugInfo() const;
#endif
	private:

	void *				InsertNew( u32 idx, u8 * adr, u32 size );


	private:
		u8 *				mBasePtr;
		u32					mTotalSize;
		bool				mDeleteOnDestruction;
	
		Chunk *				mpMemMap;
		u32					mMemMapLen;

		
#ifdef SHOW_MEM
u32					mMemAlloc;
#endif
};

#endif // UTILITY_MEMORYHEAP_H_
