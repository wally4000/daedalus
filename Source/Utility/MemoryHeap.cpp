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

//Define line below to show amount of allocated VRAM //Corn
#include <cstdio>


#include "Utility/MemoryHeap.h"

#include <string.h>
#include <iostream>
#include <cstring>



//
//  Allocator taken from.
//  Taken from http://svn.ps2dev.org/filedetails.php?repname=psp&path=%2Ftrunk%2Fpspgl%2Fpspgl_vidmem.c&rev=0&sc=0
//
//	We probably need something a bit more suitable for our requirements eventually.
//


std::unique_ptr<CMemoryHeap> CMemoryHeap::Create(u32 size)
{
	return std::make_unique<CMemoryHeap>(size);
}

std::unique_ptr<CMemoryHeap> CMemoryHeap::Create(void* base_ptr, u32 size)
{
	return std::make_unique<CMemoryHeap>(base_ptr, size);
}


CMemoryHeap::CMemoryHeap( u32 size )
:	mBasePtr( new u8[ size ] )
,	mTotalSize( size )
,	mDeleteOnDestruction( true )
,	mpMemMap( NULL )
,	mMemMapLen( 0 )
#ifdef SHOW_MEM
,	mMemAlloc( 0 )
#endif
{
}


CMemoryHeap::CMemoryHeap( void * base_ptr, u32 size )
:	mBasePtr( reinterpret_cast< u8 * >( base_ptr ) )
,	mTotalSize( size )
,	mDeleteOnDestruction( false )
,	mpMemMap( NULL )
,	mMemMapLen( 0 )
#ifdef SHOW_MEM
,	mMemAlloc( 0 )
#endif
{
}


CMemoryHeap::~CMemoryHeap()
{
	if( mDeleteOnDestruction )
	{
		delete [] mBasePtr;
	}
}


bool	CMemoryHeap::IsFromHeap( void * ptr ) const
{
	return ptr >= mBasePtr && ptr < (mBasePtr + mTotalSize);
}


void * CMemoryHeap::InsertNew( u32 idx, u8 * adr, u32 size )
{
	Chunk *tmp = reinterpret_cast< Chunk * >( realloc(mpMemMap, (mMemMapLen + 1) * sizeof(mpMemMap[0]) ) );
	if( tmp == NULL )
	{
		return NULL;
	}

	mpMemMap = tmp;
	memmove(&mpMemMap[idx+1], &mpMemMap[idx], (mMemMapLen-idx) * sizeof(mpMemMap[0]));
	mMemMapLen++;
	mpMemMap[idx].Ptr = adr;
	mpMemMap[idx].Length = size;
//Tag seems broken - Kreationz
//#ifdef DAEDALUS_DEBUG_MEMORY
//	mpMemMap[idx].Tag = tag;
//#endif

	return mpMemMap[idx].Ptr;
}


void* CMemoryHeap::Alloc( u32 size )
{
	// FIXME(strmnnrmn): This code was removed at some point - does something else help guarantee alignment?
	// Might be that we just want to lower it to 4 or 8.
	// Ensure that all memory is 16-byte aligned
	//size = AlignPow2( size, 16 );

	u8 * adr = mBasePtr;
	u32 i;

	for (i=0; i<mMemMapLen; ++i)
	{
		if( mpMemMap[i].Ptr != NULL )
		{
			u8 * new_adr = mpMemMap[i].Ptr;
			if( adr + size <= new_adr )
			{
#ifdef SHOW_MEM
				mMemAlloc += size;
				printf("VRAM %d +\n", mMemAlloc);
#endif
				return InsertNew( i, adr, size );
			}

			adr = new_adr + mpMemMap[i].Length;
		}
	}

	if( adr + size > mBasePtr + mTotalSize )
	{
		#ifdef DAEDALUS_ENABLE_ASSERTS
		DAEDALUS_ASSERT( false, "Out of VRAM/RAM memory" );
		#endif
		return NULL;
	}

#ifdef SHOW_MEM
	mMemAlloc += size;
	printf("VRAM %d +\n", mMemAlloc);
#endif
	return InsertNew( mMemMapLen, adr, size );
}


void  CMemoryHeap::Free( void * ptr )
{
	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( ptr == NULL || IsFromHeap( ptr ), "Memory is not from this heap" );
#endif
	if( ptr == nullptr ) 
		return;

	for ( u32 i=0; i < mMemMapLen; ++i )
	{
		if( mpMemMap[i].Ptr == ptr )
		{

#ifdef SHOW_MEM
			mMemAlloc -= mpMemMap[i].Length;
#endif
			mMemMapLen--;
			if (i < mMemMapLen)
			{
				std::memcpy(&mpMemMap[i], &mpMemMap[i + 1], (mMemMapLen - i) * sizeof(mpMemMap[0]));

			}
			// Try to shrink memory

			if (mMemMapLen > 0 && (mMemMapLen % 64 == 0))  // Shrink only every 64 deletions
            {
                Chunk  *tmp = reinterpret_cast<Chunk*>( realloc( mpMemMap, mMemMapLen * sizeof(mpMemMap[0]) ) );
                if( tmp != nullptr )
                {
                    mpMemMap = tmp;
                }
                else 
                {
                    std::cerr << "Memory Realloc Failed, old memory still valid. Length: " << mMemMapLen << std::endl;
                }
            }
			return;
		}
	}
#ifdef SHOW_MEM
	printf("VRAM %d -\n", mMemAlloc);
#endif
}

void CMemoryHeap::Reset()
{
	if (mpMemMap)
	{
		free(mpMemMap);
		mpMemMap = nullptr;
		mMemMapLen = 0;
	}

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
