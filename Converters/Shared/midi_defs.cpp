/* ----------------------------------------------------------------------- */

/* Title:        MIDI File routines					                       */

/* Disk Ref:     midi_defs.cpp                                             */

/* ----------------------------------------------------------------------- */

#include "pch.h"
#include <string>
#include <iomanip>
#include <algorithm>

// Fix for <sstream> on Mac (sstream uses undefined int_64t)
#include "./modules/se_sdk3/mp_api.h"
#include <sstream>

#include "midi_defs.h"

#include "sample.h"

#include "conversion.h"
#include "modules/se_sdk3/it_enum_list.h"


using namespace std;



const wchar_t* CONTROLLER_ENUM_LIST =

	L"<none>=-1,"

	L"0 - Bank Select,"

	L"1 - Mod Wheel,"

	L"2 - Breath,"

	L"3,"

	L"4 - Foot pedal,"

	L"5 - Portamento Time,"

	L"6 - Data Entry Slider,"

	L"7 - Volume,"

	L"8 - Balance,"

	L"9,"

	L"10 - Pan,"

	L"11 - Expression,"

	L"12 - Effect 1 Control,"

	L"13 - Effect 2 Control,"

	L"14,15,"

	L"16 - General Purpose Slider,"

	L"17 - General Purpose Slider,"

	L"18 - General Purpose Slider,"

	L"19 - General Purpose Slider,"

	L"20,21,22,23,24,25,26,27,28,29,30,31,32,"

	L"33 - Mod Wheel LSB=33,"

	L"34 - Breath LSB,"

	L"35 - LSB,"

	L"36 - Foot LSB,"

	L"37 - Portamento Time LSB,"

	L"38 - Data Entry Slider LSB,"

	L"39 - Volume LSB,"

	L"40 - Balance LSB,41,"

	L"42 - Pan LSB=42,"

	L"43 - Expression LSB,"

	L"44 - Effect 1 LSB,"

	L"45 - Effect 2 LSB,46,47,"

	L"48 - General Purpose LSB=48,"

	L"49 - General Purpose LSB,"

	L"50 - General Purpose LSB,"

	L"51 - General Purpose LSB,"

	L"52,53,54,55,56,57,58,59,60,61,62,63,"

	L"64 - Hold Pedal=64,"

	L"65 - Portamento,"

	L"66 - Sustenuto,"

	L"67 - Soft pedal,"

	L"68 - Legato,"

	L"69 - Hold 2 Pedal,"

	L"70 - Sound Variation,"

	L"71 - Sound Timbre,"

	L"72 - Release Time,"

	L"73 - Attack Time,"

	L"74 - Sound Brightness,"

	L"75 - Sound Control,"

	L"76 - Sound Control,"

	L"77 - Sound Control,"

	L"78 - Sound Control,"

	L"79 - Sound Control,"

	L"80 - General Purpose button,"

	L"81 - General Purpose button,"

	L"82 - General Purpose button,"

	L"83 - General Purpose button,"

	L"84,85,86,87,"

	L"88 - Hires Velocity Prefix,"

	L"89,90,"

	L"91 - Effects Level,"

	L"92 - Tremelo Level,"

	L"93 - Chorus Level,"

	L"94 - Celeste level,"

	L"95 - phaser level,"

	L"96 - Data button Increment,"

	L"97 - data button decrement,"

	L"98 - NRPN LO,"

	L"98/99 - NRPN HI," //. Sets parameter for data button and data entry slider controllers

	L"100 - RPN LO,"

	L"100/101 - RPN HI,"

	//		- 0000 Pitch bend range (course = semitones, fine = cents)

	//		- 0001 Master fine tune (cents) 0x2000 = standard

	//		- 0002 Mater fine tune semitones 0x40 = standard (fine byte ignored)

	//		- 3FFF RPN Reset

	L"102=102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,"

	L"120 - All sound off,"

	L"121 - All controllers off,"

	L"122 - local keybord on/off,"

	L"123 - All notes off,"

	L"124 - Omni Off,"

	L"125 - Omni on,"

	L"126 - Monophonic,"

	L"127 - Polyphonic";

vector< wstring > CONTROLLER_DESCRIPTION;

void init_controller_descriptions()
{
	if( CONTROLLER_DESCRIPTION.size() == 128 )
		return;

	CONTROLLER_DESCRIPTION.assign(128, L"");

	it_enum_list itr( CONTROLLER_ENUM_LIST );

	for( itr.First() ; !itr.IsDone() ; itr.Next() )
	{
		enum_entry* e = itr.CurrentItem();
		if( e->value >= 0 ) // ignore "none"
		{
			CONTROLLER_DESCRIPTION[e->value] = e->text;
		}
	}

	// provide default descriptions
	for( int i = 0 ; i < 128 ; i++ )
	{
		if( CONTROLLER_DESCRIPTION[i].empty() )
		{
			std::wostringstream oss;
			oss << L"CC" << setw(3) << i;
			CONTROLLER_DESCRIPTION[i] = oss.str();
		}
	}
}

/*



 = {

{"0 - Bank Select"},

{"1 - Mod Wheel"},

{"2 - Breath"},

{"3"},

{"4 - Foot pedal"},

{"5 - Portamento Time"},

{"6 - Data Entry Slider"},

{"7 - Volume"},

{"8 - Balance"},

{"9"},

{"10 - Pan"},

{"11 - Expression"},

{"12 - Effect 1 Control"},

{"13 - Effect 2 Control"},

{"14"},{"15"},

{"16 - General Purpose Slider"},

{"17 - General Purpose Slider"},

{"18 - General Purpose Slider"},

{"19 - General Purpose Slider"},

{"20"},{"21"},{"22"},{"23"},{"24"},{"25"},{"26"},{"27"},{"28"},{"29"},{"30"},{"31"},

{"64 - Hold Pedal"},

{"65 - Portamento"},

{"66 - Sustenuto"},

{"67 - Soft pedal"},

{"68 - Legato"},

{"69 - Hold 2 Pedal"},

{"70 - Sound Variation"},

{"71 - Sound Timbre"},

{"72 - Release Time"},

{"73 - Attack Time"},

{"74 - Sound Brightness"},

{"75 - Sound Control"},

{"76 - Sound Control"},

{"77 - Sound Control"},

{"78 - Sound Control"},

{"79 - Sound Control"},

{"80 - General Purpose button"},

{"81 - General Purpose button"},

{"82 - General Purpose button"},

{"83 - General Purpose button"},

{"84"},{"85"},{"86"},{"87"},{"88"},{"89"},{"90"},

{"91 - Effects Level"},

{"92 - Tremelo Level"},

{"93 - Chorus Level"},

{"94 - Celeste level"},

{"95 - phaser level"},

{"96 - Data button Increment"},

{"97 - data button decrement"},

{"98/99 - NRPN=99"}, //. Sets paramter for data button and data entry slider controllers

{"100/101 - RPN=101"},

//		- 0000 Pitch bend range (course = semitones, fine = cents)

//		- 0001 Master fine tune (cents) 0x2000 = standard

//		- 0002 Mater fine tune semitones 0x40 = standard (fine byte ignored)

//		- 3FFF RPN Reset

{"102=102"},{"103"},{"104"},{"105"},{"106"},{"107"},{"108"},{"109"},{"110"},{"111"},{"112"},{"113"},{"114"},{"115"},{"116"},{"117"},{"118"},{"119"},

{"120 - All sound off"},

{"121 - All controllers off"},

{"122 - local keybord on/off"},

{"123 - All notes off"},

{"124 - Omni Off"},

{"125 - Omni on"},

{"126 - Monophonic"},

{"127 - Polyphonic"}};

*/

/* Master Volume control (universal sysex)

0xF0  SysEx

0x7F  Realtime

0x7F  The SysEx channel. Could be from 0x00 to 0x7F.

	  Here we set it to "disregard channel".

0x04  Sub-ID -- Device Control

0x01  Sub-ID2 -- Master Volume

0xLL  Bits 0 to 6 of a 14-bit volume

0xMM  Bits 7 to 13 of a 14-bit volume

0xF7  End of SysEx
*/

// Conversion routines

// controller messages come in 2 7-bit 'bytes'
// these routines combine the 2 bytes into a destination variable
void cntrl_update_lsb(unsigned short& var, short lb)
{
	var = ( var & 0x3F80) + lb;			// mask off low bits and replace
}

void cntrl_update_msb(unsigned short& var, short hb)
{
	var = (var & 0x7f) + ( hb << 7 );	// mask off high bits and replace
}

void cntrl_update_lsb(short& var, short lb)
{
	var = ( var & 0x3F80) + lb;			// mask off low bits and replace
}

void cntrl_update_msb(short& var, short hb)
{
	var = (var & 0x7f) + ( hb << 7 );	// mask off high bits and replace
}

/*

// convert 14 bit range to SAMPLE range

int cntrl_to_sample( short control_val )

{

	double val = control_val / (double) MAX _CNTRL_VAL;

	val *= MAX_SAMPLE;

	return (int) val;

}

*/

// convert 14 bit range to normalized float range ( 0.0 - 1.0 )
float cntrl_to_fsample( short control_val )
{
	return (std::min)(1.0f, (std::max)(0.0f, control_val / (float) MAX_CNTRL_VAL));
}

bool ControllerType::isPolyphonic(int controller)
{
	return controller >= GlideStartPitch;
}

