/*
Copyright (C) 2006 StrmnNrmn

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

#include "Debug/DBGConsole.h"
#include "Utility/MathUtil.h"
#include "RomFile/RomFile.h"
#include "RomFile/RomFileCache.h"
#include "RomFile/RomFileMemory.h"
#include <vector>
#include <iostream>

#ifdef DAEDALUS_PSP
extern bool PSP_IS_SLIM;
#endif

namespace
{
	static  u32	CACHE_SIZE;
	static  u32 CHUNK_SIZE;
	static  u32	STORAGE_BYTES;

	static const u32	INVALID_ADDRESS = u32( ~0 );
}

struct SChunkInfo
{
	u32				StartOffset;
	mutable u32		LastUseIdx;

	bool		ContainsAddress( u32 address ) const
	{
		return address >= StartOffset && address < StartOffset + CHUNK_SIZE;
	}

	bool		InUse() const
	{
		return StartOffset != INVALID_ADDRESS;
	}

};


ROMFileCache::ROMFileCache()
    : mpROMFile(nullptr)
    , mChunkMapEntries(0)
    , mMRUIdx(0)
    , mpChunkInfo(CACHE_SIZE)  // Initialize with default size
{

#ifdef DAEDALUS_PSP
    CHUNK_SIZE = 8 * 1024;
    CACHE_SIZE = PSP_IS_SLIM ? 1024 : 256; // Conditional initialization
#else
    CHUNK_SIZE = 2 * 1024;
    CACHE_SIZE = 1024;
#endif

    STORAGE_BYTES = CACHE_SIZE * CHUNK_SIZE;

    #ifdef DAEDALUS_ENABLE_ASSERTS
    DAEDALUS_ASSERT((1 << (sizeof(CacheIdx) * 8)) > CACHE_SIZE, "Need to increase size of CacheIdx typedef to allow sufficient entries to be indexed");
    #endif

    // mpStorage = static_cast<u8*>(CROMFileMemory::Get()->Alloc(STORAGE_BYTES));

    // Initialize mpChunkInfo with default constructor
    mpChunkInfo.resize(CACHE_SIZE);  
	mpChunkMap.resize(mChunkMapEntries, static_cast<CacheIdx>(-1));
	mpStorage.resize(STORAGE_BYTES);



}


ROMFileCache::~ROMFileCache()
{
	// CROMFileMemory::Get()->Free( mpStorage );

	// delete [] mpChunkInfo;
}

bool ROMFileCache::Open(std::shared_ptr<ROMFile> p_rom_file) {
    mpROMFile = p_rom_file;
	std::cout << "Size of: mpChunkInfo :" << sizeof(mpChunkInfo) << std::endl;
    // Get ROM size and calculate the number of chunks
    u32 rom_size = p_rom_file->GetRomSize();
    u32 rom_chunks = AlignPow2(rom_size, CHUNK_SIZE) / CHUNK_SIZE;

    // Allocate and initialize chunk map
    mChunkMapEntries = rom_chunks;
    // mpChunkMap = std::vector<CacheIdx>(rom_chunks, INVALID_IDX);
 mpChunkMap = std::vector<CacheIdx>(rom_chunks, static_cast<u16>(-1));
    // Initialize chunk info array
    for (u32 i = 0; i < CACHE_SIZE; ++i) {
        mpChunkInfo[i].StartOffset = INVALID_ADDRESS;
        mpChunkInfo[i].LastUseIdx = 0;
    }

    return true;
}


void	ROMFileCache::Close()
{
	// delete [] mpChunkMap;
	// mpChunkMap = NULL;
	mChunkMapEntries = 0;
	mpROMFile = NULL;
}


inline u32 AddressToChunkMapIndex( u32 address )
{
	return address / CHUNK_SIZE;
}


inline u32 GetChunkStartAddress( u32 address )
{
	return ( address / CHUNK_SIZE ) * CHUNK_SIZE;
}



void ROMFileCache::PurgeChunk(CacheIdx cache_idx) {
    #ifdef DAEDALUS_ENABLE_ASSERTS
    // Validate cache index
    DAEDALUS_ASSERT(cache_idx < CACHE_SIZE, "Invalid chunk index");
    #endif

    SChunkInfo& chunk_info = mpChunkInfo[cache_idx];
    u32 current_chunk_address = chunk_info.StartOffset;

    if (chunk_info.InUse()) {
        // Print debug message for purging in use chunk
        //DBGConsole_Msg(0, "[CRomCache - purging %02x %08x-%08x", cache_idx, current_chunk_address, current_chunk_address + CHUNK_SIZE);

        u32 chunk_map_idx = AddressToChunkMapIndex(current_chunk_address);

        #ifdef DAEDALUS_ENABLE_ASSERTS
        // Validate chunk map index and consistency
        DAEDALUS_ASSERT(chunk_map_idx < mChunkMapEntries, "Chunk address is out of range?");
        DAEDALUS_ASSERT(mpChunkMap[chunk_map_idx] == cache_idx, "Chunk map inconsistency");
        #endif

        // Mark the chunk as not cached
        mpChunkMap[chunk_map_idx] = INVALID_IDX;
    } else {
        // Print debug message for purging unused chunk
        //DBGConsole_Msg(0, "[CRomCache - purging %02x (unused)", cache_idx);
    }

    // Reset chunk info
    chunk_info.StartOffset = INVALID_ADDRESS;
    chunk_info.LastUseIdx = 0;
}

ROMFileCache::CacheIdx ROMFileCache::GetCacheIndex(u32 address) {
    // Compute chunk map index
    u32 chunk_map_idx = AddressToChunkMapIndex(address);
    
    #ifdef DAEDALUS_ENABLE_ASSERTS
    // Validate chunk map index
    DAEDALUS_ASSERT(chunk_map_idx < mChunkMapEntries, "Chunk address is out of range?");
    #endif

    // Check if the chunk is already cached
    CacheIdx idx = mpChunkMap[chunk_map_idx];
    if (idx != INVALID_IDX) {
        return idx;
    }

    // Find the least recently used chunk
    CacheIdx selected_idx = 0;
    u32 oldest_timestamp = mpChunkInfo[0].LastUseIdx;

    for (CacheIdx i = 1; i < CACHE_SIZE; ++i) {
        u32 timestamp = mpChunkInfo[i].LastUseIdx;
        if (timestamp < oldest_timestamp) {
            oldest_timestamp = timestamp;
            selected_idx = i;
        }
    }

    // Purge the least recently used chunk
    PurgeChunk(selected_idx);

    // Load the new chunk
    SChunkInfo& chunk_info = mpChunkInfo[selected_idx];
    chunk_info.StartOffset = GetChunkStartAddress(address);
    chunk_info.LastUseIdx = ++mMRUIdx;

    #ifdef DAEDALUS_ENABLE_ASSERTS
    DAEDALUS_ASSERT(chunk_map_idx < mChunkMapEntries, "Chunk address is out of range?");
    #endif

    mpChunkMap[chunk_map_idx] = selected_idx;

    u32 storage_offset = selected_idx * CHUNK_SIZE;
    u8* p_dst = mpStorage.data() + storage_offset;

    // Load the chunk data
    mpROMFile->ReadChunk(chunk_info.StartOffset, p_dst, CHUNK_SIZE);

    return selected_idx;
}

bool ROMFileCache::GetChunk(u32 rom_offset, u8** p_p_chunk_base, u32* p_chunk_offset, u32* p_chunk_size) {
    // Compute chunk map index
    u32 chunk_map_idx = AddressToChunkMapIndex(rom_offset);
    
    // Validate chunk map index
    if (chunk_map_idx >= mChunkMapEntries) {
        return false;
    }

    // Compute cache index
    CacheIdx idx = GetCacheIndex(rom_offset);

    const SChunkInfo& chunk_info = mpChunkInfo[idx];
    #ifdef DAEDALUS_ENABLE_ASSERTS
    // Validate cache index
    DAEDALUS_ASSERT(idx < CACHE_SIZE, "Invalid chunk index!");

    
    // Validate chunk info consistency
    DAEDALUS_ASSERT(AddressToChunkMapIndex(chunk_info.StartOffset) == chunk_map_idx, "Inconsistent map indices");
    DAEDALUS_ASSERT(chunk_info.ContainsAddress(rom_offset), "Address is out of range for chunk");
    #endif

    // Set chunk base, offset, and size
    u32 storage_offset = idx * CHUNK_SIZE;
    *p_p_chunk_base = mpStorage.data() + storage_offset;
    *p_chunk_offset = chunk_info.StartOffset;
    *p_chunk_size = CHUNK_SIZE;

    // Update last use index
    chunk_info.LastUseIdx = ++mMRUIdx;

    return true;
}