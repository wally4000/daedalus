/*
Copyright (C) 2001 StrmnNrmn

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

#ifndef CORE_PIF_H_
#define CORE_PIF_H_

#include <memory>

#include "Core/CPU.h"
#include "Core/Memory.h"
#include "Core/ROM.h"
#include "Core/Save.h"
#include "Debug/DBGConsole.h"
#include "Input/InputManager.h"
#include "Utility/MathUtil.h"
#include "Ultra/ultra_os.h"
#include "Interface/Preferences.h"
class CController; 
extern std::unique_ptr<CController> gController;


// XXXX GCC
//enum	ESaveType;
//#include "ROM.h"

bool Create_Controller();
void Destroy_Controller();


class CController 
{
	public:
		CController();
		~CController();

		//
		// CController implementation
		//
		bool			OnRomOpen();
		void			OnRomClose();

		void			Process();
		static bool				Reset() { return gController->OnRomOpen(); }
		static void				RomClose() { gController->OnRomClose(); }
	private:
		//
		//
		//
		bool			ProcessController(u8 *cmd, u32 device);
		bool			ProcessEeprom(u8 *cmd);

		void			CommandReadEeprom(u8 *cmd);
		void			CommandWriteEeprom(u8* cmd);
		void			CommandReadMemPack(u32 channel, u8 *cmd);
		void			CommandWriteMemPack(u32 channel, u8 *cmd);
		void			CommandReadRumblePack(u8 *cmd);
		void			CommandWriteRumblePack(u32 channel, u8 *cmd);
		void			CommandReadRTC(u8 *cmd);

		u8				CalculateDataCrc(const u8 * pBuf) const;
		bool			IsEepromPresent() const						{ return mpEepromData != nullptr; }

		void			n64_cic_nus_6105();
		u8				Byte2Bcd(s32 n)								{ n %= 100; return ((n / 10) << 4) | (n % 10); }



#ifdef DAEDALUS_DEBUG_PIF
		void			DumpInput() const;
#endif

	private:

		enum
		{
			CONT_GET_STATUS      = 0x00,
			CONT_READ_CONTROLLER = 0x01,
			CONT_READ_MEMPACK    = 0x02,
			CONT_WRITE_MEMPACK   = 0x03,
			CONT_READ_EEPROM     = 0x04,
			CONT_WRITE_EEPROM    = 0x05,
			CONT_RTC_STATUS		 = 0x06,
			CONT_RTC_READ		 = 0x07,
			CONT_RTC_WRITE		 = 0x08,
			CONT_RESET           = 0xFF
		};

		enum
		{
			CONT_TX_SIZE_CHANSKIP   = 0x00,					// Channel Skip
			CONT_TX_SIZE_DUMMYDATA  = 0xFF,					// Dummy Data
			CONT_TX_SIZE_FORMAT_END = 0xFE,					// Format End
			CONT_TX_SIZE_CHANRESET  = 0xFD,					// Channel Reset
		};

		enum
		{
			CONT_STATUS_PAK_PRESENT      = 0x01,
			CONT_STATUS_PAK_CHANGED      = 0x02,
			CONT_STATUS_PAK_ADDR_CRC_ERR = 0x04
		};

		u8 *			mpPifRam;

#ifdef DAEDALUS_DEBUG_PIF
		u8				mpInput[ PIF_RAM_SIZE ];
#endif

		enum EPIFChannels
		{
			PC_CONTROLLER_0 = 0,
			PC_CONTROLLER_1,
			PC_CONTROLLER_2,
			PC_CONTROLLER_3,
			PC_EEPROM,
			PC_UNKNOWN_1,
			NUM_CHANNELS,
		};

		// Please update the memory allocated for mempack if we change this value
		static const u32 NUM_CONTROLLERS = 4;

		OSContPad		mContPads[ NUM_CONTROLLERS ];
		bool			mContPresent[ NUM_CONTROLLERS ];

		u8 *			mpEepromData;
		u8				mEepromContType;					// 0, CONT_EEPROM or CONT_EEP16K

		u8 *			mMemPack[ NUM_CONTROLLERS ];

#ifdef DAEDALUS_DEBUG_PIF
		std::ofstream		mDebugFile;
#endif

};



#endif // CORE_PIF_H_
