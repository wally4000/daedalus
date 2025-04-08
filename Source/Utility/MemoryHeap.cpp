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

std::unique_ptr<CMemoryHeap> CMemoryHeap::Create(u32 size)
{
	return std::make_unique<CMemoryHeap>(size);
}

std::unique_ptr<CMemoryHeap> CMemoryHeap::Create(void* base_ptr, u32 size)
{
	return std::make_unique<CMemoryHeap>(base_ptr, size);
}


CMemoryHeap::CMemoryHeap( u32 size )
:	mTotalSize(size)
#ifdef SHOW_MEM
,	mMemAlloc( 0 )
#endif
{
    void* ptr = nullptr;
    #if defined(DAEDALUS_PSP)
        ptr =  memalign(64, size);
        if (!ptr)
        {
            std::cerr << "Failed to allocate aligned memory on PSP." << std::endl;
            // throw std::bad_alloc();
        }
    #else
        if (posix_memalign(&ptr, 16, size) != 0)
        {
            std::cerr << "Failed to allocate aligned memory on non-PSP system." << std::endl;
            throw std::bad_alloc();
        }
    #endif

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


CMemoryHeap::~CMemoryHeap()
{
}



// void* CMemoryHeap::Alloc( u32 size )
// {
// 	// FIXME(strmnnrmn): This code was removed at some point - does something else help guarantee alignment?
// 	// Might be that we just want to lower it to 4 or 8.
// 	// Ensure that all memory is 16-byte aligned
// 	//size = AlignPow2( size, 16 );

	

// 	u8 * adr = mBasePtr;
// 	u32 i;

// 	for (i=0; i<mMemMapLen; ++i)
// 	{
// 		if( mpMemMap[i].Ptr != NULL )
// 		{
// 			u8 * new_adr = mpMemMap[i].Ptr;
// 			if( adr + size <= new_adr )
// 			{
// #ifdef SHOW_MEM
// 				mMemAlloc += size;
// 				printf("VRAM %d +\n", mMemAlloc);
// #endif
// 				return InsertNew( i, adr, size );
// 			}

// 			adr = new_adr + mpMemMap[i].Length;
// 		}
// 	}

// 	if( adr + size > mBasePtr + mTotalSize )
// 	{
// 		#ifdef DAEDALUS_ENABLE_ASSERTS
// 		DAEDALUS_ASSERT( false, "Out of VRAM/RAM memory" );
// 		#endif
// 		return NULL;
// 	}

// #ifdef SHOW_MEM
// 	mMemAlloc += size;
// 	printf("VRAM %d +\n", mMemAlloc);
// #endif
// 	return InsertNew( mMemMapLen, adr, size );
// }

void* CMemoryHeap::Alloc(u32 size)
{
	if (size == 0 || size > mTotalSize)
	{
		std::cerr << "Invalid memory allocation size requested: " << size << " bytes." << std::endl;
		return nullptr;
	}

	u8* addr = mBasePtr.get();

	for (auto& chunk : mChunks)
	{
		if (addr + size <= chunk.Ptr)
		{
			return InsertNew(addr, size);
		}
		addr = chunk.Ptr + chunk.Length; 
	}
	
	if ( addr + size > mBasePtr.get() + mTotalSize)
	{
		std::cerr << "Out of memory!" << std::endl;
		return nullptr;
	}

	return InsertNew(addr, size);
}





void * CMemoryHeap::InsertNew( u8 * adr, u32 size )
{
	Chunk newChunk{ adr, size};
	mChunks.emplace_back(newChunk);
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

void  CMemoryHeap::Free( void * ptr )
{
	auto it = std::find_if(mChunks.begin(), mChunks.end(), [ptr](const Chunk& chunk)
	{
		return chunk.Ptr == ptr;
	});

	if (it != mChunks.end())
	{
	#ifdef SHOW_MEM
		mMemAlloc -= it->Length;
	#endif
		mChunks.erase(it);
	}
	else{
		std::cerr << "Attempted to free memory that was not allocated by this heap." << std::endl;
	}
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
