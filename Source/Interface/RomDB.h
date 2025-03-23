/*

  Copyright (C) 2002 StrmnNrmn

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

#ifndef INTERFACE_ROMDB_H_
#define INTERFACE_ROMDB_H_

#include "Core/ROM.h"
#include "Base/Types.h"

#include <algorithm>
#include <vector>
#include <fstream>
#include <cstring>

#include "Core/ROM.h"
#include "Core/ROMImage.h"
#include "Debug/DBGConsole.h"
#include "Interface/RomDB.h"
#include "Utility/MathUtil.h"
#include "RomFile/RomFile.h"
#include "Utility/Stream.h"
#include "Utility/Paths.h"
#include <filesystem>
class	RomID;

class	CRomDB
{
	public:
     CRomDB();
		 ~CRomDB();

		 bool			OpenDB( const std::filesystem::path filename );												// Open the specified rom db, or creates a new one if it does not currently exist

		 void			Reset();																		// Resets the contents of the database
		 bool			Commit();																		// Commits any changes made to the database to disk

		 void			AddRomDirectory(const std::filesystem::path& directory);

		 bool			QueryByFilename( const std::filesystem::path& filename, RomID * id, u32 * rom_size, ECicType * cic_type );		// Query a rom from the database
		 bool			QueryByID( const RomID & id, u32 * rom_size, ECicType * cic_type ) const;						// Query a rom from the database
		 const char *	QueryFilenameFromID( const RomID & id ) const;

  private:

  void			AddRomFile(const std::filesystem::path& filename);

  void			AddRomEntry( const std::filesystem::path& filename, const RomID & id, u32 rom_size, ECicType cic_type );


// For serialisation we used a fixed size struct for ease of reading
struct RomFilesKeyValue
{
  RomFilesKeyValue()
  {
    memset( FileName, 0, sizeof( FileName ) );
  }
  RomFilesKeyValue( const RomFilesKeyValue & rhs )
  {
    memset( FileName, 0, sizeof( FileName ) );
    strcpy( FileName, rhs.FileName );
    ID = rhs.ID;
  }
  RomFilesKeyValue & operator=( const RomFilesKeyValue & rhs )
  {
    if( this == &rhs )
      return *this;
    memset( FileName, 0, sizeof( FileName ) );
    strcpy( FileName, rhs.FileName );
    ID = rhs.ID;
    return *this;
  }

  RomFilesKeyValue( const std::filesystem::path filename, const RomID & id )
  {
    memset( FileName, 0, sizeof( FileName ) );
    strcpy( FileName, filename.string().c_str() );
    ID = id;
  }

  // This is actually kMaxPathLen+1, but we need to ensure that it doesn't change if we ever change the kMaxPathLen constant.
  static const u32 kMaxFilenameLen = 260;
  char		FileName[ kMaxFilenameLen + 1 ];
  RomID		ID;
};

struct SSortByFilename {
bool operator()(const RomFilesKeyValue& a, const RomFilesKeyValue& b) const {
    return std::string_view(a.FileName) < std::string_view(b.FileName);
}
bool operator()(std::string_view a, const RomFilesKeyValue& b) const {
    return a < b.FileName;
}
bool operator()(const RomFilesKeyValue& a, std::string_view b) const {
    return a.FileName < b;
}
};

struct RomDetails
{
  RomDetails()
    :	ID()
    ,	RomSize( 0 )
    ,	CicType( CIC_UNKNOWN )
  {
  }

  RomDetails( const RomID & id, u32 rom_size, ECicType cic_type )
    :	ID( id )
    ,	RomSize( rom_size )
    ,	CicType( cic_type )
  {
  }

  RomID				ID;
  u32					RomSize;
  ECicType			CicType;
};

struct SSortDetailsByID
{
  bool operator()( const RomDetails & a, const RomDetails & b ) const
  {
    return a.ID < b.ID;
  }
  bool operator()( const RomID & a, const RomDetails & b ) const
  {
    return a < b.ID;
  }
  bool operator()( const RomDetails & a, const RomID & b ) const
  {
    return a.ID < b;
  }
};
using FilenameVec = std::vector< RomFilesKeyValue >;
using DetailsVec = std::vector< RomDetails >;

std::filesystem::path			mRomDBFileName;
FilenameVec						mRomFiles;
DetailsVec						mRomDetails;
bool							mDirty;
};

extern std::unique_ptr<CRomDB> gRomDB;

#endif // INTERFACE_ROMDB_H_
