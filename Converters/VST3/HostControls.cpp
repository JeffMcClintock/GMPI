//#include "pch.h"

// Fix for <sstream> on Mac (sstream uses undefined int_64t)
#include "mp_api.h"
#include <sstream>

#include "HostControls.h"
#include "se_datatypes.h"
#include "midi_defs.h"

using namespace std;

struct HostControlStruct // holds XML -> enum info
{
	const wchar_t* display;
	HostControls id;
	int datatype;
	int automation;
};

/* categories:
- Voice - Note expression. Polyphonic parameters targeted at individual voices.
- Time  - Song position and tempo related.
*/

static const HostControlStruct lookup[] =
{
	{(L"PatchCommands")			,HC_PATCH_COMMANDS, DT_INT,ControllerType::None},

	{(L"MidiChannelIn")			,HC_MIDI_CHANNEL, DT_INT,ControllerType::None},

	{(L"ProgramNamesList")		,HC_PROGRAM_NAMES_LIST, DT_TEXT,ControllerType::None},

	{(L"Program")				,HC_PROGRAM, DT_INT,ControllerType::None},

	{(L"ProgramName")			,HC_PROGRAM_NAME, DT_TEXT,ControllerType::None},

	{(L"Voice/Trigger")			,HC_VOICE_TRIGGER, DT_FLOAT,ControllerType::Trigger << 24},

	{(L"Voice/Gate")			,HC_VOICE_GATE, DT_FLOAT,ControllerType::Gate << 24},

	{(L"Voice/Pitch")			,HC_VOICE_PITCH, DT_FLOAT,ControllerType::Pitch << 24},

	{(L"Voice/VelocityKeyOn")	,HC_VOICE_VELOCITY_KEY_ON, DT_FLOAT,ControllerType::VelocityOn << 24},

	{(L"Voice/VelocityKeyOff")	,HC_VOICE_VELOCITY_KEY_OFF, DT_FLOAT,ControllerType::VelocityOff << 24},

	{(L"Voice/Aftertouch")		,HC_VOICE_AFTERTOUCH, DT_FLOAT,ControllerType::PolyAftertouch << 24},

	{(L"Voice/VirtualVoiceId")	,HC_VOICE_VIRTUAL_VOICE_ID, DT_INT,ControllerType::VirtualVoiceId << 24},

	{(L"Voice/Active")			,HC_VOICE_ACTIVE				, DT_FLOAT,ControllerType::Active << 24},

	{(L"XXXXXXXXXX")			,HC_UNUSED						, DT_FLOAT, ControllerType::None},// was HC_VOICE_RESET -"Voice/Reset"

	{(L"VoiceAllocationMode")	,HC_VOICE_ALLOCATION_MODE		, DT_INT, ControllerType::None},

	{(L"Bender")				,HC_PITCH_BENDER				, DT_FLOAT, ControllerType::Bender << 24},

	{(L"HoldPedal")				,HC_HOLD_PEDAL					, DT_FLOAT, (ControllerType::CC << 24) | 64}, // CC 64 = Hold Pedal

	{(L"Channel Pressure")		,HC_CHANNEL_PRESSURE			, DT_FLOAT, ControllerType::ChannelPressure << 24},

	{(L"Time/BPM")				,HC_TIME_BPM					, DT_FLOAT, ControllerType::BPM << 24},

	{(L"Time/SongPosition")		,HC_TIME_QUARTER_NOTE_POSITION	, DT_FLOAT, ControllerType::SongPosition << 24},

	{(L"Time/TransportPlaying")	,HC_TIME_TRANSPORT_PLAYING		, DT_BOOL, ControllerType::TransportPlaying << 24},
	{(L"Polyphony")				,HC_POLYPHONY					, DT_INT, ControllerType::None},
	{(L"ReserveVoices")			,HC_POLYPHONY_VOICE_RESERVE		, DT_INT, ControllerType::None},
	{(L"Oversampling/Rate")		,HC_OVERSAMPLING_RATE			, DT_ENUM, ControllerType::None},
	{(L"Oversampling/Filter")	, HC_OVERSAMPLING_FILTER		, DT_ENUM, ControllerType::None},
	{(L"User/Int0")				, HC_USER_SHARED_PARAMETER_INT0	, DT_INT, ControllerType::None},
	{(L"Time/BarStartPosition"), HC_TIME_BAR_START				, DT_FLOAT, ControllerType::barStartPosition << 24},
	{(L"Time/Timesignature/Numerator"), HC_TIME_NUMERATOR		, DT_INT, ControllerType::timeSignatureNumerator << 24},
	{(L"Time/Timesignature/Denominator"), HC_TIME_DENOMINATOR	, DT_INT, ControllerType::timeSignatureDenominator << 24},

	// VST3 Note-expression (and MIDI 2.0 support)
	// multitrack studio: "VST3 instruments that support VST3 note expression receive per-note Pitch Bend, Volume, Pan, Expression, Brightness and Vibrato Depth. Poly Aftertouch is sent as well."

	// MIDI 2.0 Per-Note Expression 7
	{(L"Voice/Volume")		, HC_VOICE_VOLUME, DT_FLOAT, (ControllerType::VoiceNoteExpression << 24) | (ControllerType::kVolumeTypeID << 16)},

	// MIDI 2.0 Per-Note Expression 10
	{(L"Voice/Pan")			, HC_VOICE_PAN, DT_FLOAT, (ControllerType::VoiceNoteExpression << 24) | (ControllerType::kPanTypeID << 16)},

	// MIDI 2.0 Per-Note Pitch Bend Message (was "Tuning"). bi-polar. 0 = -10 octaves, 1.0 = +10 octaves. 
	{(L"Voice/Bender")		, HC_VOICE_PITCH_BEND, DT_FLOAT, (ControllerType::VoiceNoteExpression << 24) | (ControllerType::kTuningTypeID << 16)},

	// MIDI 2.0 Per-Note Vibrato Depth. per-note CC 8
	{(L"Voice/Vibrato")		, HC_VOICE_VIBRATO, DT_FLOAT, (ControllerType::VoiceNoteExpression << 24) | (ControllerType::kVibratoTypeID << 16)},

	// MIDI 2.0 Per-Note Expression. per-note CC 11
	{(L"Voice/Expression")	, HC_VOICE_EXPRESSION, DT_FLOAT, (ControllerType::VoiceNoteExpression << 24) | (ControllerType::kExpressionTypeID << 16)},

	// MIDI 2.0 Per-Note Expression 5
	{(L"Voice/Brightness")	, HC_VOICE_BRIGHTNESS, DT_FLOAT, (ControllerType::VoiceNoteExpression << 24) | (ControllerType::kBrightnessTypeID << 16)},

	{(L"Voice/UserControl0")	, HC_VOICE_USER_CONTROL0, DT_FLOAT, (ControllerType::VoiceNoteExpression << 24) | (ControllerType::kCustomStart << 16)},
	{(L"Voice/UserControl1")	, HC_VOICE_USER_CONTROL1, DT_FLOAT, (ControllerType::VoiceNoteExpression << 24) | ((ControllerType::kCustomStart + 1) << 16)},
	{(L"Voice/UserControl2")	, HC_VOICE_USER_CONTROL2, DT_FLOAT, (ControllerType::VoiceNoteExpression << 24) | ((ControllerType::kCustomStart + 2) << 16)},
	{(L"Voice/PortamentoEnable")	, HC_VOICE_PORTAMENTO_ENABLE, DT_FLOAT, (ControllerType::VoiceNoteControl << 24) | ((ControllerType::kPortamentoEnable) << 16)},


	{(L"SnapModulation")		, HC_SNAP_MODULATION__DEPRECATED, DT_INT, ControllerType::None},
	{( L"Portamento" )		, HC_PORTAMENTO, DT_FLOAT, ( ControllerType::CC << 24 ) | 5}, // CC 5 = Portamento Time 
	{( L"Voice/GlideStartPitch" ), HC_GLIDE_START_PITCH, DT_FLOAT, ControllerType::GlideStartPitch << 24},
	{( L"BenderRange" )		, HC_BENDER_RANGE, DT_FLOAT, ( ControllerType::RPN << 24 ) | 0}, // RPN 0000 = Pitch bend range (course = semitones, fine = cents).

	{L"SubPatchCommands"		, HC_SUB_PATCH_COMMANDS, DT_INT, ControllerType::None},
	{L"Processor/OfflineRenderMode", HC_PROCESS_RENDERMODE, DT_ENUM, ControllerType::None}, // 0 = :Live", 2 = "Preview" (Offline)

	{L"User/Int1"				, HC_USER_SHARED_PARAMETER_INT1	, DT_INT, ControllerType::None},
	{L"User/Int2"				, HC_USER_SHARED_PARAMETER_INT2	, DT_INT, ControllerType::None},
	{L"User/Int3"				, HC_USER_SHARED_PARAMETER_INT3	, DT_INT, ControllerType::None},
	{L"User/Int4"				, HC_USER_SHARED_PARAMETER_INT4	, DT_INT, ControllerType::None},
	{L"PatchCables"				, HC_PATCH_CABLES				, DT_BLOB, ControllerType::None},

	{L"Processor/SilenceOptimisation", HC_SILENCE_OPTIMISATION	, DT_BOOL, ControllerType::None},

	{L"ProgramCategory"			,HC_PROGRAM_CATEGORY			, DT_TEXT, ControllerType::None},
	{L"ProgramCategoriesList"	,HC_PROGRAM_CATEGORIES_LIST		, DT_TEXT, ControllerType::None},

	{L"MpeMode"					,HC_MPE_MODE					, DT_INT, ControllerType::None},
	{L"Presets/ProgramModified"	,HC_PROGRAM_MODIFIED			, DT_BOOL, ControllerType::None},
	{L"Presets/CanUndo"			,HC_CAN_UNDO					, DT_BOOL, ControllerType::None},
	{L"Presets/CanRedo"			,HC_CAN_REDO					, DT_BOOL, ControllerType::None },

	// MAINTAIN ORDER TO PRESERVE OLDER WAVES EXPORTS DSP.XML consistancy
};

HostControls StringToHostControl( const wstring& txt )
{
	const int size = (sizeof(lookup) / sizeof(HostControlStruct));

//	_RPT1(_CRT_WARN, "StringToHostControl %s.\n", WStringToUtf8(txt).c_str());

	HostControls returnVal = HC_NONE;

	if( txt.compare( L"" ) != 0 )
	{
		int j;

		for( j = 0 ; j < size ; j++ )
		{
			if( txt.compare( lookup[j].display ) == 0  )
			{
				returnVal = lookup[j].id;
				break;
			}
		}

		if( j == size )
		{
			// Avoiding msg box in plugin
			assert(false);
//			GetApp()->SeMessageBox( errorText ,MB_OK|MB_ICONSTOP );
		}
	}

	return returnVal;
}

int GetHostControlDatatype( HostControls hostControlId )
{
	const int DATAYPE_INFO_COUNT = (sizeof(lookup) / sizeof(HostControlStruct));
	if(hostControlId < DATAYPE_INFO_COUNT)
	{
		return lookup[hostControlId].datatype;
	}

	assert(false);
	return -1;
}

const wchar_t* GetHostControlName( HostControls hostControlId )
{
	const int DATAYPE_INFO_COUNT = (sizeof(lookup) / sizeof(HostControlStruct));
	if(hostControlId < DATAYPE_INFO_COUNT)
	{
		return lookup[hostControlId].display;
	}

	assert(false);
	return L"";
}

const wchar_t* GetHostControlNameByAutomation(int automation)
{
	const int DATAYPE_INFO_COUNT = (sizeof(lookup) / sizeof(HostControlStruct));

	for (int j = 0; j < DATAYPE_INFO_COUNT; j++)
	{
		if (lookup[j].automation == automation)
		{
			return lookup[j].display;
			break;
		}
	}

	assert(false);
	return L"";
}

int GetHostControlAutomation( HostControls hostControlId )
{
	const int DATAYPE_INFO_COUNT = (sizeof(lookup) / sizeof(HostControlStruct));
	if(hostControlId < DATAYPE_INFO_COUNT)
	{
		return lookup[hostControlId].automation;
	}

	assert(false);
	return ControllerType::None;
}

// Most host controls 'belong' to the Patch Automator, however a handfull apply to the local parent container.
bool HostControlAttachesToParentContainer( HostControls hostControlId )
{
	switch( hostControlId )
	{
		case HC_VOICE_TRIGGER:
		case HC_VOICE_GATE:
		case HC_VOICE_PITCH:
		case HC_VOICE_VELOCITY_KEY_ON:
		case HC_VOICE_VELOCITY_KEY_OFF:
		case HC_VOICE_AFTERTOUCH:
		case HC_VOICE_VIRTUAL_VOICE_ID:
		case HC_VOICE_ACTIVE:
		case HC_VOICE_ALLOCATION_MODE:
		case HC_PITCH_BENDER:
		case HC_HOLD_PEDAL:
		case HC_CHANNEL_PRESSURE :
        case HC_POLYPHONY:
        case HC_POLYPHONY_VOICE_RESERVE:
        case HC_OVERSAMPLING_RATE:
        case HC_OVERSAMPLING_FILTER:
		case HC_VOICE_VOLUME:
		case HC_VOICE_PAN:
		case HC_VOICE_TUNING:
		case HC_VOICE_PITCH_BEND:
		case HC_VOICE_VIBRATO:
		case HC_VOICE_EXPRESSION:
		case HC_VOICE_BRIGHTNESS:
		case HC_VOICE_USER_CONTROL0:
		case HC_VOICE_USER_CONTROL1:
		case HC_VOICE_USER_CONTROL2:
		case HC_VOICE_PORTAMENTO_ENABLE:
		case HC_SNAP_MODULATION__DEPRECATED:
		case HC_PORTAMENTO:
		case HC_GLIDE_START_PITCH:
		case HC_BENDER_RANGE:
			return true;
		break;

		default:
            return false;
		break;
	}

	return false;
}

bool HostControlisPolyphonic(HostControls hostControlId)
{
	switch (hostControlId)
	{
	case HC_VOICE_TRIGGER:
	case HC_VOICE_GATE:
	case HC_VOICE_PITCH:
	case HC_VOICE_VELOCITY_KEY_ON:
	case HC_VOICE_VELOCITY_KEY_OFF:
	case HC_VOICE_AFTERTOUCH:
	case HC_VOICE_VIRTUAL_VOICE_ID:
	case HC_VOICE_ACTIVE:
	case HC_VOICE_PORTAMENTO_ENABLE:
	case HC_VOICE_VOLUME:
	case HC_VOICE_PAN:
//nope	case HC_VOICE_TUNING:
	case HC_VOICE_PITCH_BEND:
	case HC_VOICE_VIBRATO:
	case HC_VOICE_EXPRESSION:
	case HC_VOICE_BRIGHTNESS:
	case HC_VOICE_USER_CONTROL0:
	case HC_VOICE_USER_CONTROL1:
	case HC_VOICE_USER_CONTROL2:
	case HC_GLIDE_START_PITCH:

		return true;
		break;

	default:
		return false;
		break;
	}

	return false;
}

bool AffectsVoiceAllocation(HostControls hostControlId)
{
	switch( hostControlId )
	{
	case HC_VOICE_ALLOCATION_MODE:
	case HC_PORTAMENTO:
		return true;
		break;

	default:
		return false;
		break;
	}

	return false;
}