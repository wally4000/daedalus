#ifndef INTERFACE_ROMID_H_
#define INTERFACE_ROMID_H_

#include "Core/ROMImage.h"

class RomID
{
	public:
		RomID()
		{
			CRC[0] = 0;
			CRC[1] = 0;
			CountryID = 0;
		}

		RomID( u32 crc1, u32 crc2, u8 country_id )
		{
			CRC[0] = crc1;
			CRC[1] = crc2;
			CountryID = country_id;
		}

		explicit RomID( const ROMHeader & header )
		{
			CRC[0] = header.CRC1;
			CRC[1] = header.CRC2;
			CountryID = header.CountryID;
		}

		bool Empty() const
		{
			return CRC[0] == 0 && CRC[1] == 0 && CountryID == 0;
		}

		bool operator==( const RomID & id ) const		{ return Compare( id ) == 0; }
		bool operator!=( const RomID & id ) const		{ return Compare( id ) != 0; }
		bool operator<( const RomID & rhs ) const		{ return Compare( rhs ) < 0; }

		s32 Compare( const RomID & rhs ) const
		{
			s32		diff;

			diff = CRC[0] - rhs.CRC[0];
			if( diff != 0 )
				return diff;

			diff = CRC[1] - rhs.CRC[1];
			if( diff != 0 )
				return diff;

			diff = CountryID - rhs.CountryID;
			if( diff != 0 )
				return diff;

			return 0;
		}

		u32		CRC[2];
		u8		CountryID;
};

#endif//  INTERFACE_ROMID_IDH_