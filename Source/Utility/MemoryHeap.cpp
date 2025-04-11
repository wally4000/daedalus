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


#include "Utility/MemoryHeap.h"
#include <memory>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <malloc.h>
#include "SysPSP/Utility/CacheUtil.h"
std::vector<MemoryBlock> mBlockList;


std::unique_ptr<CMemoryHeap> CMemoryHeap::Create(u32 size)
{
	return std::make_unique<CMemoryHeap>(size);
}

std::unique_ptr<CMemoryHeap> CMemoryHeap::Create(void* base_ptr, u32 size)
{
	return std::make_unique<CMemoryHeap>(base_ptr, size);
}


CMemoryHeap::CMemoryHeap(u32 size)
: mTotalSize(size)
#ifdef SHOW_MEM
, mMemAlloc(0)
#endif
{
	void* ptr = malloc_64(size);
	if (!ptr)
	{
		std::cerr << "Failed to allocate aligned memory" << std::endl;
	}

    mBasePtr = std::unique_ptr<u8[]>(reinterpret_cast<u8*>(ptr));
}

CMemoryHeap::CMemoryHeap( void * base_ptr, u32 size )
:	mBasePtr( static_cast<u8*>( base_ptr ) )
,	mTotalSize( size )
#ifdef SHOW_MEM
,	mMemAlloc( 0 )
#endif
{
}


CMemoryHeap::~CMemoryHeap() {}


void* CMemoryHeap::Alloc(u32 size)
{
	if (size == 0 || size > mTotalSize)
		return nullptr;

	// Try to reuse a freed block
	for (auto& block : mBlockList)
	{
		if (!block.used && block.size >= size)
		{
			block.used = true;
			mUsedSize += size;
			return block.ptr;
		}
	}

	u8* base = mBasePtr.get();
	u8* addr = base;

	for (auto& chunk : mChunks)
		addr = chunk.Ptr + chunk.Length;

	if (addr + size > base + mTotalSize)
		return nullptr;

	mUsedSize += size;
	mChunks.push_back({ addr, size });
	mBlockList.push_back({ addr, size, true });
	return addr;
}




void * CMemoryHeap::InsertNew( u8 * adr, u32 size )
{
	Chunk newChunk{ adr, size};
	mChunks.emplace_back(newChunk);

	mUsedSize += size;
#ifdef SHOW_MEM
	mMemAlloc += size;
#endif 
	return newChunk.Ptr;
}

bool	CMemoryHeap::IsFromHeap( void * ptr ) const
{
	auto base = mBasePtr.get();
	return (ptr >= base) && (ptr < (base + mTotalSize));
}

void CMemoryHeap::Free(void * ptr)
{
	for (auto& block : mBlockList)
	{
		if (block.ptr == ptr && block.used)
		{
			block.used = false;
			mUsedSize -= block.size;
			return;
		}
	}
	std::cerr << "Attempted to free untracked or already freed block\n";
}


void CMemoryHeap::Reset()
{
	mChunks.clear();
#ifdef SHOW_MEM
	mMemAlloc = 0;
#endif

}

#ifdef DAEDALUS_DEBUG_MEMORY

void CMemoryHeap::DisplayDebugInfo() const
{
	printf( "  #  Address    Length  Tag\n" );

	for (u32 i=0; i<mMemMapLen; ++i)
	{
		const Chunk &	chunk( mpMemMap[ i ] );

		printf( "%02d: %p %8d", i, chunk.Ptr, chunk.Length );
#ifdef DAEDALUS_DEBUG_MEMORY
		printf( " %05d", chunk.Tag );
#endif
		printf( "\n" );
	}
}
#endif
