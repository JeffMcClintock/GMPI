#pragma once

#include <string>

/*
#include "HostControls.h"

To add a host control:
 - Add entry to HostControls enum here.
 - Add entry to StringToHostControl().
 - CPatchManager::GetHostGeneratedParameter()
 */

enum HostControls { HC_NONE=-1, HC_PATCH_COMMANDS, HC_MIDI_CHANNEL, HC_PROGRAM_NAMES_LIST,

					HC_PROGRAM, HC_PROGRAM_NAME, HC_VOICE_TRIGGER, HC_VOICE_GATE, HC_VOICE_PITCH,

					HC_VOICE_VELOCITY_KEY_ON, HC_VOICE_VELOCITY_KEY_OFF, HC_VOICE_AFTERTOUCH, HC_VOICE_VIRTUAL_VOICE_ID,

					HC_VOICE_ACTIVE, HC_UNUSED /*was HC_VOICE_RESET,*/,
					HC_VOICE_ALLOCATION_MODE, // includes mono-note-priority.

					HC_PITCH_BENDER, HC_HOLD_PEDAL, HC_CHANNEL_PRESSURE,

					HC_TIME_BPM, HC_TIME_QUARTER_NOTE_POSITION,
					HC_TIME_TRANSPORT_PLAYING,
					HC_POLYPHONY,
					HC_POLYPHONY_VOICE_RESERVE,
					HC_OVERSAMPLING_RATE,		// NOT REALLY automation. Just need a mechanism to restart DSP.
					HC_OVERSAMPLING_FILTER,
					HC_USER_SHARED_PARAMETER_INT0,
					HC_TIME_BAR_START,
					HC_TIME_NUMERATOR,
					HC_TIME_DENOMINATOR,

					HC_VOICE_VOLUME,
					HC_VOICE_PAN,
					HC_VOICE_PITCH_BEND,
					HC_VOICE_VIBRATO,
					HC_VOICE_EXPRESSION,
					HC_VOICE_BRIGHTNESS,
					HC_VOICE_USER_CONTROL0,
					HC_VOICE_USER_CONTROL1,
					HC_VOICE_USER_CONTROL2,

					HC_VOICE_PORTAMENTO_ENABLE,
					HC_SNAP_MODULATION__DEPRECATED,
					HC_PORTAMENTO,
					HC_GLIDE_START_PITCH,
					HC_BENDER_RANGE,
					HC_SUB_PATCH_COMMANDS,
					HC_PROCESS_RENDERMODE,

					HC_USER_SHARED_PARAMETER_INT1,
					HC_USER_SHARED_PARAMETER_INT2,
					HC_USER_SHARED_PARAMETER_INT3,
					HC_USER_SHARED_PARAMETER_INT4,

					HC_PATCH_CABLES,
					HC_SILENCE_OPTIMISATION,

					HC_PROGRAM_CATEGORY,
					HC_PROGRAM_CATEGORIES_LIST,

					HC_MPE_MODE,
					HC_PROGRAM_MODIFIED, // Hmm, seems 'HC_PROGRAM_MODIFIED' and 'HC_CAN_UNDO' are synonymous (assuming undo manager enabled)
					HC_CAN_UNDO,
					HC_CAN_REDO,

					// leave last
					HC_VOICE_TUNING, // psudo host-control. used only for persistance of tuning table in getPersisentHostControl().
					HC_NUM_HOST_CONTROLS,
};

enum class EPatchCommands
{
	Undo = 17,
	Redo,
	CompareGet_A,
	CompareGet_B,
	CompareGet_CopyAB
};

HostControls StringToHostControl( const std::wstring& txt );
int GetHostControlDatatype( HostControls hc );
const wchar_t* GetHostControlName( HostControls hc );
const wchar_t* GetHostControlNameByAutomation(int automation);
int GetHostControlAutomation(HostControls hostControlId);
bool HostControlAttachesToParentContainer( HostControls hostControlId );
bool HostControlisPolyphonic(HostControls hostControlId);
bool AffectsVoiceAllocation(HostControls hostControlId);

