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


#include "Base/Types.h"
#include "Core/ROM.h"

#include <stdio.h>
#include <cstring>

#include "Interface/Cheats.h"
#include "Core/CPU.h"
#include "Core/PIF.h"		// CController
#include "Core/R4300.h"
#include "RomFile/ROMBuffer.h"
#include "Core/ROMImage.h"

#include "Interface/ConfigOptions.h"
#include "Debug/DBGConsole.h"
#include "Debug/DebugLog.h"
#include "Interface/RomDB.h"
#include "Utility/MathUtil.h"
#include "OSHLE/patch.h"			// Patch_ApplyPatches
#include "Ultra/ultra_os.h"		// System type
#include "Ultra/ultra_R4300.h"
#include "HLEAudio/AudioPlugin.h"
#include "HLEGraphics/GraphicsPlugin.h"
#include "Utility/CRC.h"
#include "Utility/FramerateLimiter.h"

#include "Base/Macros.h"
#include "Interface/Preferences.h"
#include "RomFile/RomFile.h"
#include "Utility/Stream.h"
#include "Debug/Synchroniser.h"
#include "System/SystemInit.h"

#if defined(DAEDALUS_ENABLE_DYNAREC_PROFILE)
// This isn't really the most appropriate place. Need to check with
// the graphics plugin really
u32 g_dwNumFrames = 0;
#endif


static void DumpROMInfo( const ROMHeader & header )
{
	// The "Header" is actually something to do with the PI_DOM_*_OFS values...
	DBGConsole_Msg(0, "Header:          0x%02x%02x%02x%02x", header.x1, header.x2, header.x3, header.x4);
	DBGConsole_Msg(0, "Clockrate:       0x%08x", header.ClockRate);
	DBGConsole_Msg(0, "BootAddr:        0x%08x", BSWAP32(header.BootAddress));
	DBGConsole_Msg(0, "Release:         0x%08x", header.Release);
	DBGConsole_Msg(0, "CRC1:            0x%08x", header.CRC1);
	DBGConsole_Msg(0, "CRC2:            0x%08x", header.CRC2);
	DBGConsole_Msg(0, "Unknown0:        0x%08x", header.Unknown0);
	DBGConsole_Msg(0, "Unknown1:        0x%08x", header.Unknown1);
	DBGConsole_Msg(0, "ImageName:       '%s'",   header.Name);
	DBGConsole_Msg(0, "Unknown2:        0x%08x", header.Unknown2);
	DBGConsole_Msg(0, "Unknown3:        0x%04x", header.Unknown3);
	DBGConsole_Msg(0, "Unknown4:        0x%02x", header.Unknown4);
	DBGConsole_Msg(0, "Manufacturer:    0x%02x", header.Manufacturer);
	DBGConsole_Msg(0, "CartID:          0x%04x", header.CartID);
	DBGConsole_Msg(0, "CountryID:       0x%02x - '%c'", header.CountryID, (char)header.CountryID);
	DBGConsole_Msg(0, "Unknown5:        0x%02x", header.Unknown5);

}

static void ROM_SimulatePIFBoot( ECicType cic_chip, u32 Country )
{
	// Copy low 1000 bytes to MEM_SP_MEM
	RomBuffer::GetRomBytesRaw( (u8*)g_pMemoryBuffers[MEM_SP_MEM] + RAMROM_BOOTSTRAP_OFFSET,
		   RAMROM_BOOTSTRAP_OFFSET,
		   RAMROM_GAME_OFFSET - RAMROM_BOOTSTRAP_OFFSET );

	// Need to copy to SP_IMEM for CIC-6105 boot.
	u8 * pIMemBase = (u8*)g_pMemoryBuffers[ MEM_SP_MEM ] + 0x1000;

	gCPUState.CPUControl[C0_RAND]._u32 = 0x1F;
	gCPUState.CPUControl[C0_COUNT]._u32 = 0x5000;
	Memory_MI_SetRegister(MI_VERSION_REG, 0x02020102);
	Memory_SP_SetRegister(SP_STATUS_REG, SP_STATUS_HALT);
	gCPUState.CPUControl[C0_CAUSE]._u32 = 0x0000005C;
	gCPUState.CPUControl[C0_CONTEXT]._u32 = 0x007FFFF0;
	gCPUState.CPUControl[C0_EPC]._u32 = 0xFFFFFFFF;
	gCPUState.CPUControl[C0_BADVADDR]._u32 	= 0xFFFFFFFF;
	gCPUState.CPUControl[C0_ERROR_EPC]._u32	= 0xFFFFFFFF;
	gCPUState.CPUControl[C0_CONFIG]._u32	= 0x0006E463;
	gCPUState.CPUControl[C0_SR]._u32		= 0x34000000;	//*SR_FR |*/ SR_ERL | SR_CU2|SR_CU1|SR_CU0;
	//R4300_SetSR(0x34000000);

	// From R4300 manual
	//gCPUState.CPUControl[C0_SR]._u32   = 0x70400004;	//*SR_FR |*/ SR_ERL | SR_CU2|SR_CU1|SR_CU0;
	R4300_SetSR(0x70400004);
	gCPUState.CPUControl[C0_PRID]._u32   = 0x00000b10;	// Was 0xb00 - test rom reports 0xb10!!
	gCPUState.CPUControl[C0_WIRED]._u32  = 0x0;

	gCPUState.FPUControl[0]._u32 = 0x00000511;

	((u32 *)g_pMemoryBuffers[MEM_RI_REG])[3] = 1;					// RI_CONFIG_REG Skips most of init

	gGPR[0]._u64=0x0000000000000000LL;
	gGPR[1]._u64=0x0000000000000000LL;
	gGPR[2]._u64=0xFFFFFFFFD1731BE9LL;
	gGPR[3]._u64=0xFFFFFFFFD1731BE9LL;
	gGPR[4]._u64=0x0000000000001BE9LL;
	gGPR[5]._u64=0xFFFFFFFFF45231E5LL;
	gGPR[6]._u64=0xFFFFFFFFA4001F0CLL;
	gGPR[7]._u64=0xFFFFFFFFA4001F08LL;
	gGPR[8]._u64=0x00000000000000C0LL;
	gGPR[9]._u64=0x0000000000000000LL;
	gGPR[10]._u64=0x0000000000000040LL;
	gGPR[11]._u64=0xFFFFFFFFA4000040LL;
	gGPR[16]._u64=0x0000000000000000LL;
	gGPR[17]._u64=0x0000000000000000LL;
	gGPR[18]._u64=0x0000000000000000LL;
	gGPR[19]._u64=0x0000000000000000LL;
	gGPR[20]._u64=ctx.romInfo->TvType;
	gGPR[21]._u64=0x0000000000000000LL;
	gGPR[23]._u64=0x0000000000000006LL;
	gGPR[24]._u64=0x0000000000000000LL;
	gGPR[25]._u64=0xFFFFFFFFD73f2993LL;
	gGPR[26]._u64=0x0000000000000000LL;
	gGPR[27]._u64=0x0000000000000000LL;
	gGPR[28]._u64=0x0000000000000000LL;
	gGPR[29]._u64=0xFFFFFFFFA4001FF0LL;
	gGPR[30]._u64=0x0000000000000000LL;
	gGPR[31]._u64=0xFFFFFFFFA4001554LL;

	switch (Country) {
		case 0x44: //Germany
		case 0x46: //french
		case 0x49: //Italian
		case 0x50: //Europe
		case 0x53: //Spanish
		case 0x55: //Australia
		case 0x58: // ????
		case 0x59: // X (PAL)
			switch (cic_chip) {
				case CIC_6102:
					gGPR[5]._u64=0xFFFFFFFFC0F1D859LL;
					gGPR[14]._u64=0x000000002DE108EALL;
					gGPR[24]._u64=0x0000000000000000LL;
					break;
				case CIC_6103:
					gGPR[5]._u64=0xFFFFFFFFD4646273LL;
					gGPR[14]._u64=0x000000001AF99984LL;
					gGPR[24]._u64=0x0000000000000000LL;
					break;
				case CIC_6105:
					*(u32 *)&pIMemBase[0x04] = 0xBDA807FC;
					gGPR[5]._u64=0xFFFFFFFFDECAAAD1LL;
					gGPR[14]._u64=0x000000000CF85C13LL;
					gGPR[24]._u64=0x0000000000000002LL;
					break;
				case CIC_6106:
					gGPR[5]._u64=0xFFFFFFFFB04DC903LL;
					gGPR[14]._u64=0x000000001AF99984LL;
					gGPR[24]._u64=0x0000000000000002LL;
					break;
				default:
					break;
			}

			gGPR[20]._u64=0x0000000000000000LL;
			gGPR[23]._u64=0x0000000000000006LL;
			gGPR[31]._u64=0xFFFFFFFFA4001554LL;
			break;
		case 0x37: // 7 (Beta)
		case 0x41: // ????
		case 0x45: //USA
		case 0x4A: //Japan
		default:
			switch (cic_chip) {
				case CIC_6102:
					gGPR[5]._u64=0xFFFFFFFFC95973D5LL;
					gGPR[14]._u64=0x000000002449A366LL;
					break;
				case CIC_6103:
					gGPR[5]._u64=0xFFFFFFFF95315A28LL;
					gGPR[14]._u64=0x000000005BACA1DFLL;
					break;
				case CIC_6105:
					*(u32  *)&pIMemBase[0x04] = 0x8DA807FC;
					gGPR[5]._u64=0x000000005493FB9ALL;
					gGPR[14]._u64=0xFFFFFFFFC2C20384LL;
					break;
				case CIC_6106:
					gGPR[5]._u64=0xFFFFFFFFE067221FLL;
					gGPR[14]._u64=0x000000005CD2B70FLL;
					break;
				default:
					break;
			}
			gGPR[20]._u64=0x0000000000000001LL;
			gGPR[23]._u64=0x0000000000000000LL;
			gGPR[24]._u64=0x0000000000000003LL;
			gGPR[31]._u64=0xFFFFFFFFA4001550LL;
	}

	switch (cic_chip) {
		case CIC_6101:
			gGPR[22]._u64=0x000000000000003FLL;
			break;
		case CIC_6102:
			gGPR[1]._u64=0x0000000000000001LL;
			gGPR[2]._u64=0x000000000EBDA536LL;
			gGPR[3]._u64=0x000000000EBDA536LL;
			gGPR[4]._u64=0x000000000000A536LL;
			gGPR[12]._u64=0xFFFFFFFFED10D0B3LL;
			gGPR[13]._u64=0x000000001402A4CCLL;
			gGPR[15]._u64=0x000000003103E121LL;
			gGPR[22]._u64=0x000000000000003FLL;
			gGPR[25]._u64=0xFFFFFFFF9DEBB54FLL;
			break;
		case CIC_6103:
			gGPR[1]._u64=0x0000000000000001LL;
			gGPR[2]._u64=0x0000000049A5EE96LL;
			gGPR[3]._u64=0x0000000049A5EE96LL;
			gGPR[4]._u64=0x000000000000EE96LL;
			gGPR[12]._u64=0xFFFFFFFFCE9DFBF7LL;
			gGPR[13]._u64=0xFFFFFFFFCE9DFBF7LL;
			gGPR[15]._u64=0x0000000018B63D28LL;
			gGPR[22]._u64=0x0000000000000078LL;
			gGPR[25]._u64=0xFFFFFFFF825B21C9LL;
			break;
		case CIC_6105:
			*(u32  *)&pIMemBase[0x00] = 0x3C0DBFC0;
			*(u32  *)&pIMemBase[0x08] = 0x25AD07C0;
			*(u32  *)&pIMemBase[0x0C] = 0x31080080;
			*(u32  *)&pIMemBase[0x10] = 0x5500FFFC;
			*(u32  *)&pIMemBase[0x14] = 0x3C0DBFC0;
			*(u32  *)&pIMemBase[0x18] = 0x8DA80024;
			*(u32  *)&pIMemBase[0x1C] = 0x3C0BB000;
			gGPR[1]._u64=0x0000000000000000LL;
			gGPR[2]._u64=0xFFFFFFFFF58B0FBFLL;
			gGPR[3]._u64=0xFFFFFFFFF58B0FBFLL;
			gGPR[4]._u64=0x0000000000000FBFLL;
			gGPR[12]._u64=0xFFFFFFFF9651F81ELL;
			gGPR[13]._u64=0x000000002D42AAC5LL;
			gGPR[15]._u64=0x0000000056584D60LL;
			gGPR[22]._u64=0x0000000000000091LL;
			gGPR[25]._u64=0xFFFFFFFFCDCE565FLL;
			break;
		case CIC_6106:
			gGPR[1]._u64=0x0000000000000000LL;
			gGPR[2]._u64=0xFFFFFFFFA95930A4LL;
			gGPR[3]._u64=0xFFFFFFFFA95930A4LL;
			gGPR[4]._u64=0x00000000000030A4LL;
			gGPR[12]._u64=0xFFFFFFFFBCB59510LL;
			gGPR[13]._u64=0xFFFFFFFFBCB59510LL;
			gGPR[15]._u64=0x000000007A3C07F4LL;
			gGPR[22]._u64=0x0000000000000085LL;
			gGPR[25]._u64=0x00000000465E3F72LL;
			break;
		default:
			break;
	}

	// Also need to set up PI_BSD_DOM1 regs etc!
	CPU_SetPC(0xA4000040);
}

bool ROM_ReBoot()
{
	//
	// Find out the CIC type and initialise various systems based on the CIC type
	//
	u8	rom_base[ RAMROM_GAME_OFFSET ];
	RomBuffer::GetRomBytesRaw( rom_base, 0, RAMROM_GAME_OFFSET );

	ctx.romInfo->cic_chip = ROM_GenerateCICType( rom_base );

#ifdef DAEDALUS_DEBUG_CONSOLE
	if (ctx.romInfo->cic_chip == CIC_UNKNOWN)
	{
		//DAEDALUS_ERROR( "Unknown CIC CRC: 0x%08x\nAssuming CIC-6102", crc );
		//DBGConsole_Msg(0, "[MUnknown CIC CRC: 0x%08x]", crc );
		DBGConsole_Msg(0, "[MUnknown CIC]" );
	}
	else
	{
		DBGConsole_Msg(0, "[MRom uses %s]", ROM_GetCicName( ctx.romInfo->cic_chip ) );
	}
#endif
	// XXXX Update this rom's boot info
#ifdef DAEDALUS_ENABLE_DYNAREC_PROFILE
	g_dwNumFrames = 0;
#endif

#ifdef DAEDALUS_ENABLE_OS_HOOKS
	Patch_Reset();
#endif

	ROM_SimulatePIFBoot( ctx.romInfo->cic_chip, ctx.romInfo->rh.CountryID );

	return true;
}

void ROM_Unload()
{
}

//Most hacks are for the PSP, due the limitations of the hardware, and because we prefer speed over accuracy
void SpecificGameHacks( const ROMHeader & id )
{
	printf("ROM ID[%04X]\n", id.CartID);

	ctx.romInfo->HACKS_u32 = 0;	//Default to no game hacks

	switch( id.CartID )
	{
	case 0x324a: ctx.romInfo->GameHacks = WONDER_PROJECTJ2;	break;
	case 0x4547: ctx.romInfo->GameHacks = GOLDEN_EYE;			break;
	case 0x5742: ctx.romInfo->GameHacks = SUPER_BOWLING;		break;
	case 0x514D: ctx.romInfo->GameHacks = PMARIO;				break;
	case 0x5632: ctx.romInfo->GameHacks = CHAMELEON_TWIST_2;	break;
	case 0x4154: ctx.romInfo->GameHacks = TARZAN;				break;
	case 0x4643: ctx.romInfo->GameHacks = CLAY_FIGHTER_63;		break;
	case 0x504A: ctx.romInfo->GameHacks = ISS64;				break;
	case 0x5944: ctx.romInfo->GameHacks = DKR;					break;
	case 0x3247: ctx.romInfo->GameHacks = EXTREME_G2;			break;
	case 0x5359: ctx.romInfo->GameHacks = YOSHI;				break;
	case 0x4C42: ctx.romInfo->GameHacks = BUCK_BUMBLE;			break;
	case 0x4441: ctx.romInfo->GameHacks = WORMS_ARMAGEDDON;	break;
	case 0x3357: ctx.romInfo->GameHacks = WCW_NITRO;			break;

	case 0x464A:	// Jet Force Geminy
	case 0x5647:	// Glover
	ctx.romInfo->SET_ROUND_MODE = true;
		break;
	case 0x4B42:	//Banjo-Kazooie
	ctx.romInfo->TLUT_HACK = true;
	//	ctx.romInfo->DISABLE_LBU_OPT = true;
		break;
	//case 0x5750:	//PilotWings64
	case 0x4450:	//Perfect Dark
	ctx.romInfo->DISABLE_LBU_OPT = true;
		break;
	case 0x5941:	//AIDYN_CRONICLES
	ctx.romInfo->ALPHA_HACK = true;
	ctx.romInfo->GameHacks = AIDYN_CRONICLES;
		break;
	case 0x424C:	//Mario Party 1
	ctx.romInfo->DISABLE_SIM_CVT_D_S = true;
		break;
	case 0x4A54:	//Tom and Jerry
	case 0x4d4a:	//Earthworm Jim
	case 0x5150:	//PowerPuff Girls
	ctx.romInfo->DISABLE_SIM_CVT_D_S = true;
	ctx.romInfo->LOAD_T1_HACK = true;
		break;
	case 0x5144:	//Donald Duck
	case 0x3259:	//Rayman2
	ctx.romInfo->SET_ROUND_MODE = true;
	ctx.romInfo->LOAD_T1_HACK = true;
	ctx.romInfo->T1_HACK = true;
		break;
	case 0x3358:	//GEX3
	case 0x3258:	//GEX64
	ctx.romInfo->GameHacks = GEX_GECKO;
		break;
	case 0x4c5a:	//ZELDA_OOT
	ctx.romInfo->ZELDA_HACK = true;
	ctx.romInfo->GameHacks = ZELDA_OOT;
		break;
	case 0x4F44:	//DK64
	ctx.romInfo->SET_ROUND_MODE = true;
	ctx.romInfo->GameHacks = DK64;
		break;
	case 0x535a:	//ZELDA_MM
	ctx.romInfo->TLUT_HACK = true;
	ctx.romInfo->ZELDA_HACK = true;
	ctx.romInfo->GameHacks = ZELDA_MM;
		break;
	case 0x5653:	//SSV
	ctx.romInfo->LOAD_T1_HACK = true;
	ctx.romInfo->TLUT_HACK = true;
		break;
	case 0x5547:	//Sin and punishment
	ctx.romInfo->TLUT_HACK = true;
	ctx.romInfo->GameHacks = SIN_PUNISHMENT;
		break;
	case 0x3742:	//Banjo Tooie
	ctx.romInfo->GameHacks = BANJO_TOOIE;
	ctx.romInfo->TLUT_HACK = true;
		break;
	case 0x5544:	//Duck Dodgers
	case 0x3653:	//Star soldier - vanishing earth
	case 0x324C:	//Top Gear Rally 2
	case 0x5247:	//Top Gear Rally
	case 0x4552:	//Resident Evil 2
	case 0x4446:	//Flying Dragon
	case 0x534E:	//Beetle Racing
	ctx.romInfo->TLUT_HACK = true;
		break;
	case 0x4641:	//Animal crossing
	ctx.romInfo->TLUT_HACK = true;
	ctx.romInfo->GameHacks = ANIMAL_CROSSING;
		break;
	case 0x4842:	//Body Harvest
	case 0x434E:	//Nightmare Creatures
	case 0x5543:	//Cruisn' USA
	ctx.romInfo->GameHacks = BODY_HARVEST;
		break;
	default:
		break;
	}
}

// Copy across text, nullptr terminate, and strip spaces
void ROM_GetRomNameFromHeader( std::string & rom_name, const ROMHeader & header )
{
	char	buffer[20+1];
	memcpy( buffer, header.Name, 20 );
	buffer[20] = '\0';

	rom_name = buffer;

	const char * whitespace_chars = " \t\r\n";
	rom_name.erase( 0, rom_name.find_first_not_of(whitespace_chars) );
	rom_name.erase( rom_name.find_last_not_of(whitespace_chars)+1 );
}

bool ROM_LoadFile()
{
	RomID		rom_id;
	u32			rom_size;
	ECicType	boot_type;

	if (ROM_GetRomDetailsByFilename(ctx.romInfo->mFileName, &rom_id, &rom_size, &boot_type ))
	{
		RomSettings			settings;
		SRomPreferences		ROMpreferences;

		if (!ctx.romSettingsDB->GetSettings( rom_id, &settings ))
		{
			settings.Reset();
		}
		if (!ctx.preferences->GetRomPreferences( rom_id, &ROMpreferences ))
		{
			ROMpreferences.Reset();
		}

		return ROM_LoadFile( rom_id, settings, ROMpreferences );
	}

	return false;
}

void ROM_UnloadFile()
{
	// Copy across various bits
	ctx.romInfo->mRomID = RomID();
	ctx.romInfo->settings = RomSettings();
}

bool ROM_LoadFile(const RomID & rom_id, const RomSettings & settings, const SRomPreferences & ROMpreferences )
{
	DBGConsole_Msg(0, "Reading rom image: [C%s]", ctx.romInfo->mFileName.string().c_str());

	// Get information about the rom header
	RomBuffer::GetRomBytesRaw( &ctx.romInfo->rh, 0, sizeof(ROMHeader) );

	//	Swap into native format
	ROMFile::ByteSwap_3210( &ctx.romInfo->rh, sizeof(ROMHeader) );

	#ifdef DAEDALUS_ENABLE_ASSERTS
	DAEDALUS_ASSERT( RomID( ctx.romInfo->rh ) == rom_id, "Why is the rom id incorrect?" );
	#endif
	// Copy across various bits
	ctx.romInfo->mRomID   = rom_id;
	ctx.romInfo->settings = settings;
	ctx.romInfo->TvType   = ROM_GetTvTypeFromID( ctx.romInfo->rh.CountryID );

	// Game specific hacks..
	SpecificGameHacks( ctx.romInfo->rh );

	DumpROMInfo( ctx.romInfo->rh );

	// Read and apply preferences from preferences.ini
	ROMpreferences.Apply(ctx);

	// Parse cheat file this rom, if cheat feature is enabled
	// This is also done when accessing the cheat menu
	// But we do this when ROM is loaded too, to allow any forced enabled cheats to work.
	if (gCheatsEnabled)
	{
		CheatCodes_Read( ctx.romInfo->settings.GameName.c_str(), "Daedalus.cht", ctx.romInfo->rh.CountryID );
	}

	DBGConsole_Msg(0, "[G%s]", ctx.romInfo->settings.GameName.c_str());
	DBGConsole_Msg(0, "This game has been certified as [G%s] (%s)", ctx.romInfo->settings.Comment.c_str(), ctx.romInfo->settings.Info.c_str());
	DBGConsole_Msg(0, "SaveType: [G%s]", ROM_GetSaveTypeName( ctx.romInfo->settings.SaveType ) );
	DBGConsole_Msg(0, "ApplyPatches: [G%s]", gOSHooksEnabled ? "on" : "off");
	DBGConsole_Msg(0, "Check Texture Hash Freq: [G%d]", gCheckTextureHashFrequency);
	DBGConsole_Msg(0, "SpeedSync: [G%d]", gSpeedSyncEnabled);
	DBGConsole_Msg(0, "DynaRec: [G%s]", gDynarecEnabled ? "on" : "off");
	DBGConsole_Msg(0, "Cheats: [G%s]", gCheatsEnabled ? "on" : "off");
	//Patch_ApplyPatches();

	return true;
}

bool ROM_GetRomName( const std::filesystem::path &filename, std::string & game_name )
{
	auto p_rom_file = ROMFile::Create( filename );
	if (p_rom_file == nullptr)
	{
		return false;
	}

	CNullOutputStream messages;

	if (!p_rom_file->Open( messages ))
	{
		// delete p_rom_file;
		return false;
	}

	// Only read in the header
	const u32	bytes_to_read = sizeof(ROMHeader);
	u32			size_aligned = AlignPow2( bytes_to_read, 4 );	// Needed?
	u8 *		p_bytes = new u8[size_aligned];

	if (!p_rom_file->LoadData( bytes_to_read, p_bytes, messages ))
	{
		// Lots of files don't have any info - don't worry about it
		delete [] p_bytes;
		// delete p_rom_file;
		return false;
	}

	//	Swap into native format
	ROMFile::ByteSwap_3210( p_bytes, bytes_to_read );

	// Get the address of the rom header
	// Setup the rom id and size
	const ROMHeader * prh = reinterpret_cast<const ROMHeader *>( p_bytes );
	ROM_GetRomNameFromHeader( game_name, *prh );

	delete [] p_bytes;
	return true;
}

bool ROM_GetRomDetailsByFilename( const std::filesystem::path &filename, RomID * id, u32 * rom_size, ECicType * boot_type )
{
	return ctx.romDB->QueryByFilename( filename.c_str(), id, rom_size, boot_type );
}

bool ROM_GetRomDetailsByID( const RomID & id, u32 * rom_size, ECicType * boot_type )
{
	return ctx.romDB->QueryByID( id, rom_size, boot_type );
}

// Association between a country id value, tv type and name
struct CountryIDInfo
{
	s8				CountryID;
	const char *	CountryName;
	u32				TvType;
};

static constexpr std::array<CountryIDInfo, 13> g_CountryCodeInfo {
// static const CountryIDInfo g_CountryCodeInfo[] =
{
	{  0,  "0",			OS_TV_NTSC },
	{ '7', "Beta",		OS_TV_NTSC },
	{ 'A', "NTSC",		OS_TV_NTSC },
	{ 'D', "Germany",	OS_TV_PAL },
	{ 'E', "USA",		OS_TV_NTSC },
	{ 'F', "France",	OS_TV_PAL },
	{ 'I', "Italy",		OS_TV_PAL },
	{ 'J', "Japan",		OS_TV_NTSC },
	{ 'P', "Europe",	OS_TV_PAL },
	{ 'S', "Spain",		OS_TV_PAL },
	{ 'U', "Australia", OS_TV_PAL },
	{ 'X', "PAL",		OS_TV_PAL },
	{ 'Y', "PAL",		OS_TV_PAL }
}};

// Get a string representing the country name from an ID value
const char * ROM_GetCountryNameFromID( u8 country_id )
{
	for (u32 i = 0; i < g_CountryCodeInfo.size(); i++)
	{
		if (g_CountryCodeInfo[i].CountryID == country_id)
		{
			return g_CountryCodeInfo[i].CountryName;
		}
	}

	return "?";
}

u32 ROM_GetTvTypeFromID( u8 country_id )
{
	for (u32 i = 0; i < g_CountryCodeInfo.size(); i++)
	{
		if (g_CountryCodeInfo[i].CountryID == country_id)
		{
			return g_CountryCodeInfo[i].TvType;
		}
	}

	return OS_TV_NTSC;
}
