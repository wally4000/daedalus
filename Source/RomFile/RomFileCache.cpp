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
#include "System/SystemInit.h"


#ifdef DAEDALUS_PSP
extern bool PSP_IS_SLIM;
#endif



ROMFileCache::ROMFileCache()
:	mpROMFile( nullptr )
,	mChunkMapEntries( 0 )
,	mMRUIdx( 0 )
{
#ifdef DAEDALUS_PSP
	CHUNK_SIZE = 8 * 1024;
	CACHE_SIZE = PSP_IS_SLIM ? 1024 : 256;
#else
	CHUNK_SIZE = 2 * 1024;
	CACHE_SIZE = 1024;
#endif

	STORAGE_BYTES = CACHE_SIZE * CHUNK_SIZE;
#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( (1<<(sizeof(CacheIdx)*8)) > CACHE_SIZE, "Need to increase size of CacheIdx typedef to allow sufficient entries to be indexed" );
#endif
	mpStorage   = std::make_unique<u8[]>(STORAGE_BYTES);
	mpChunkInfo.resize(CACHE_SIZE);
}

ROMFileCache::~ROMFileCache() {}

bool	ROMFileCache::Open( std::unique_ptr<ROMFile> p_rom_file )
{
	mpROMFile = std::move(p_rom_file);

	u32 rom_size = mpROMFile->GetRomSize(); 
	u32	rom_chunks = AlignPow2( rom_size, CHUNK_SIZE ) / CHUNK_SIZE;

	mChunkMapEntries = rom_chunks;
	mpChunkMap.assign(rom_chunks, INVALID_IDX);

	for (auto & chunk : mpChunkInfo)
	{
		chunk.StartOffset = INVALID_ADDRESS;
		chunk.LastUseIdx = 0;
	}

	return true;
}

void	ROMFileCache::Close()
{
	mpROMFile = nullptr;
	mpChunkMap.clear();
	mpChunkInfo.clear();
}

inline u32 AddressToChunkMapIndex( u32 address )
{
	return address / CHUNK_SIZE;
}

inline u32 GetChunkStartAddress( u32 address )
{
	return ( address / CHUNK_SIZE ) * CHUNK_SIZE;
}


void	ROMFileCache::PurgeChunk( CacheIdx cache_idx )
{
	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( cache_idx < CACHE_SIZE, "Invalid chunk index" );
	#endif

	SChunkInfo & chunk_info( mpChunkInfo[ cache_idx ] );
	u32		current_chunk_address( chunk_info.StartOffset );
	if( chunk_info.InUse() )
	{
		//DBGConsole_Msg( 0, "[CRomCache - purging %02x %08x-%08x", cache_idx, chunk_info.StartOffset, chunk_info.StartOffset + CHUNK_SIZE );
		u32		chunk_map_idx( AddressToChunkMapIndex( current_chunk_address ) );
		#ifdef DAEDALUS_ENABLE_ASSERTS
		DAEDALUS_ASSERT( chunk_map_idx < mChunkMapEntries, "Chunk address is out of range?" );
		DAEDALUS_ASSERT( mpChunkMap[ chunk_map_idx ] == cache_idx, "Chunk map inconsistancy" );
		#endif
		// Scrub down the chunk map to show it's no longer cached
		mpChunkMap[ chunk_map_idx ] = INVALID_IDX;
	}
	else
	{
		//DBGConsole_Msg( 0, "[CRomCache - purging %02x (unused)", cache_idx );
	}

	// Scrub these down
	chunk_info.StartOffset = INVALID_ADDRESS;
	chunk_info.LastUseIdx = 0;
}

ROMFileCache::CacheIdx	ROMFileCache::GetCacheIndex( u32 address )
{
	u32	chunk_map_idx =  AddressToChunkMapIndex( address );

	//	Check if this chunk is already cached, load if necessary
	CacheIdx idx =  mpChunkMap[ chunk_map_idx ];

	if(idx == INVALID_IDX)
	{
		CacheIdx selected_idx = 0;
		u32	oldest_timestamp =  mpChunkInfo[ 0 ].LastUseIdx;

		for(CacheIdx i = 1; i < CACHE_SIZE; ++i)
		{
			if (mpChunkInfo[i].LastUseIdx < oldest_timestamp)
			{
				oldest_timestamp = mpChunkInfo[i].LastUseIdx;
				selected_idx = i;
			}
		}

		PurgeChunk( selected_idx );

		SChunkInfo &chunk_info =  mpChunkInfo[ selected_idx ];
		chunk_info.StartOffset = GetChunkStartAddress( address );
		chunk_info.LastUseIdx = ++mMRUIdx;

		mpChunkMap[ chunk_map_idx ] = selected_idx;

		u32	storage_offset =  selected_idx * CHUNK_SIZE;
		u8 *p_dst = mpStorage.get() + storage_offset;

		//DBGConsole_Msg( 0, "[CRomCache - loading %02x, %08x-%08x", selected_idx, chunk_info.StartOffset, chunk_info.StartOffset + CHUNK_SIZE );
		mpROMFile->ReadChunk( chunk_info.StartOffset, p_dst, CHUNK_SIZE );

		idx = selected_idx;
	}

	return idx;
}

bool	ROMFileCache::GetChunk( u32 rom_offset, u8 ** p_p_chunk_base, u32 * p_chunk_offset, u32 * p_chunk_size )
{
	u32	chunk_map_idx =  AddressToChunkMapIndex( rom_offset );

	if(chunk_map_idx < mChunkMapEntries)
	{
		CacheIdx idx = GetCacheIndex( rom_offset );
		#ifdef DAEDALUS_ENABLE_ASSERTS
		DAEDALUS_ASSERT( idx < CACHE_SIZE, "Invalid chunk index!" );
		#endif

		if (idx != INVALID_IDX)
		{
			SChunkInfo &chunk_info = mpChunkInfo[ idx ];

			u32	storage_offset = idx * CHUNK_SIZE;

		*p_p_chunk_base = mpStorage.get() + storage_offset;
		*p_chunk_offset = chunk_info.StartOffset;
		*p_chunk_size = CHUNK_SIZE;	
		return true;
		}
		else
		{
			// Invalid address, no chunk available
			return false;
		}
	}	
}