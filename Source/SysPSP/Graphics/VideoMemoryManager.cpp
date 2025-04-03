// Taken from http://svn.ps2dev.org/filedetails.php?repname=psp&path=%2Ftrunk%2Fpspgl%2Fpspgl_vidmem.c&rev=0&sc=0


#include "Base/Types.h"
#include "SysPSP/Graphics/VideoMemoryManager.h"
#include <cstring>
#include <pspge.h>
#include <psputils.h>
#include <iostream>

const u32 ERAM(3 * 512 * 1024);	//Amount of extra (volatile)RAM to use for textures in addition to VRAM //Corn



// CVideoMemoryManager::CVideoMemoryManager()
// :	mVideoMemoryHeap( std::unique_ptr<CMemoryHeap>(CMemoryHeap::Create( make_uncached_ptr( sceGeEdramGetAddr() ), sceGeEdramGetSize() )))
// ,	mRamMemoryHeap( std::unique_ptr<CMemoryHeap>(CMemoryHeap::Create( make_uncached_ptr( (void*)(((u32)malloc_volatile(ERAM + 0xF) + 0xF) & ~0xF) ), ERAM ) ))
// //,	mRamMemoryHeap( CMemoryHeap::Create( 1 * 1024 * 1024 ) )
// {
// 	printf( "vram base: %p\n", sceGeEdramGetAddr() );
// 	printf( "vram size: %d KB\n", sceGeEdramGetSize() / 1024 );
// }

CVideoMemoryManager::CVideoMemoryManager()
{
	// Initialise Video Memory Heap
	void * vram_base = make_uncached_ptr(sceGeEdramGetAddr());
	u32 vram_size = sceGeEdramGetSize();

	if (vram_base == nullptr || vram_size ==0)
	{
		std::cerr << "Failed to initialize VRAM" << std::endl;
	}
	else
	{
		mVideoMemoryHeap = std::unique_ptr<CMemoryHeap>(CMemoryHeap::Create(vram_base, vram_size));
		std::cout << "VRAM base: " << vram_base << std::endl;
		std::cout << "VRAM Size: " << vram_size / 2014 << " KB" << std::endl;

		void* ram_base = malloc_volatile(ERAM + 0xF);
		if (ram_base == nullptr)
		{
			std::cerr << "Failed to allocate volatile RAM" << std::endl;
		}
		else
		{
			ram_base = (void*)(((u32)ram_base +0xF) & ~0xF); // 16 byte alignment
			mRamMemoryHeap = std::unique_ptr<CMemoryHeap>(CMemoryHeap::Create(make_uncached_ptr(ram_base),ERAM));
		}
	}
}


CVideoMemoryManager::~CVideoMemoryManager() {
	mVideoMemoryHeap.reset();
	mRamMemoryHeap.reset();
}

bool CVideoMemoryManager::Alloc(u32 size, void **data, bool *isvidmem)
{
    if (!data || !isvidmem)
        return false;

    *data = nullptr;
    *isvidmem = false;

    // Ensure that all memory is 16-byte aligned
    size = AlignPow2(size, 16);

    // Try to allocate fast VRAM
    void *mem = mVideoMemoryHeap->Alloc(size);
    if (mem != nullptr)
    {
        *data = mem;
        *isvidmem = true;
        return true;
    }
#ifdef DAEDALUS_DEBUG_CONSOLE
    std::cerr << "Failed to allocate " << size << " bytes of VRAM" << std::endl;
#endif

    // Try to allocate normal RAM
    mem = mRamMemoryHeap->Alloc(size);
    if (mem != nullptr)
    {
        *data = mem;
        *isvidmem = false;
        return true;
    }
#ifdef DAEDALUS_DEBUG_CONSOLE
    std::cerr << "Failed to allocate " << size << " bytes of RAM (risk for BSOD)" << std::endl;
#endif

    return false;
}

void  CVideoMemoryManager::Free(void * ptr)
{
	if( ptr == nullptr )
	{
		return;
	}
	else if( mVideoMemoryHeap->IsFromHeap( ptr ) )
	{
		mVideoMemoryHeap->Free( ptr );
	}
	else if( mRamMemoryHeap->IsFromHeap( ptr ) )
	{
		mRamMemoryHeap->Free( ptr );
	}
	#ifdef DAEDALUS_DEBUG_CONSOLE
	else
	{
		DAEDALUS_ERROR( "Memory is not from any of our heaps" );
	}
	#endif
}

#ifdef DAEDALUS_DEBUG_MEMORY

void CVideoMemoryManager::DisplayDebugInfo()
{
	printf( "VRAM\n" );
	mVideoMemoryHeap->DisplayDebugInfo();

	printf( "RAM\n" );
	mRamMemoryHeap->DisplayDebugInfo();
}
#endif
