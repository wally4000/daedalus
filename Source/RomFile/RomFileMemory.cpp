/*
Copyright (C) 2012 StrmnNrmn

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


#include "Base/Types.h"
#include "RomFile/RomFileMemory.h"

#include <iostream> 


extern bool PSP_IS_SLIM;


CROMFileMemory::~CROMFileMemory() {
	mRomMemoryHeap.reset();
}


CROMFileMemory::CROMFileMemory()
{
#ifdef DAEDALUS_PSP
	//
	// Allocate large memory heap for SLIM+ (32Mb) Used for ROM Buffer and ROM Cache
	// Otherwise allocate small memory heap for PHAT (2Mb) Used for ROM cache only
	//
	if (PSP_IS_SLIM)
	{
		mRomMemoryHeap = std::unique_ptr<CMemoryHeap>(CMemoryHeap::Create(16 * 1024 * 1024));
	}
	else
	{
		mRomMemoryHeap = std::unique_ptr<CMemoryHeap>(CMemoryHeap::Create(2 * 1024 * 1024));
	}
	
#endif
	mRomMemoryHeap = CMemoryHeap::Create(16 * 1024 * 1024);
}

void * CROMFileMemory::Alloc( u32 size )
{
		void * ptr = nullptr;
	// #ifdef DAEDALUS_PSP
		return mRomMemoryHeap->Alloc( size );
	// #else
	// 	return malloc( size );
	// #endif

	if (ptr == nullptr)
	{
		std::cerr << "Memory allocation failed for size: " << size << " bytes." << std::endl;
	}
	return ptr;
}

