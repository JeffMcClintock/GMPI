/* Title: Standard MIDI File General Header */
/* Disk Ref: midi_defs.h */
/* #include "midi_defs.h" */
#if !defined(_midi_defs_h_)
#define _midi_defs_h_

#include <string>
#include <vector>
//#include "sample.h"

#define NOTE_OFF				0x80
#define NOTE_OFF_SIZE			2
#define NOTE_ON					0x90
#define NOTE_ON_SIZE			2
#define POLY_AFTERTOUCH			0xA0
#define POLY_AFTERTOUCH_SIZE	2
#define CONTROL_CHANGE			0xB0
#define CONTROL_CHANGE_SIZE		2
#define PROGRAM_CHANGE			0xC0
#define PROGRAM_CHANGE_SIZE		1
#define CHANNEL_PRESSURE		0xD0
#define CHANNEL_PRESSURE_SIZE	1
#define PITCHBEND				0xE0
#define PITCHBEND_SIZE			2
#define SYSTEM_MSG				0xF0	/* F0 -> FF */
#define SYSTEM_EXCLUSIVE		0xF0
#define MIDI_CLOCK				0xF8
#define MIDI_CLOCK_START		0xFA
#define MIDI_CLOCK_CONT			0xFB
#define MIDI_CLOCK_STOP			0xFC
#define ACTIVE_SENSING			0xFE

/* whats the biggest 14-bit controller value (assuming low 7 bits may be 0) */
#define MAX_CNTRL_VAL	0x3F80

/* whats the biggest 14-bit controller value (assuming low bits in use). used for RPN.  */
#define MAX_FULL_CNTRL_VAL		0x3FFF
#define NRPN_MSB				99
#define NRPN_LSB				98
#define RPN_MSB					101
#define RPN_LSB					100
#define RPN_CONTROLLER			6
#define MOD_WHEEL_MSB			1
#define MIDI_CC_ALL_NOTES_OFF	123
#define MIDI_CC_ALL_SOUND_OFF	120

/* What's the bigget SYSEX SE can handle. */
#define MAX_SYSEX_SIZE	64

struct ControllerType
{
	enum EcontrollerType
	{
		Learn			= -2,
		None			= -1,
		NullType		= 0,			/* ignore. */
		CC				= 2,
		RPN,
		NRPN,
		SYSEX,
		Bender,
		ChannelPressure,				/* monophonic aftertouch. */
		BPM,
		TransportPlaying,
		SongPosition,
/*
		// Monophonic note info based on last played note.
		MonoGate,
		MonoTrigger,
		MonoPitch,
		MonoVelocityOn,
		MonoVelocityOff,
*/

		GlideStartPitch = 18,			/* only polyphonic allowed after here. see ControllerType::isPolyphonic(). */
		Pitch			= 19,
		Gate,
		VelocityOn,
		VelocityOff,
		PolyAftertouch,
		Trigger,
		Active,			/* note-stealing. float. Active (10) = not stolen. 0.0 - Voice in rapid-mute (steal) phase. */
		VoiceNoteControl,	// see NoteControlTypeIDs. Internal control of voices. Distinct from Steinberg Note-Expression support which does not cover all the bases.		
		VirtualVoiceId, /* int. usually same as MIDI note number. */

		barStartPosition,
		timeSignatureNumerator,
		timeSignatureDenominator,

		VoiceNoteExpression,
	};

	enum NoteExpressionTypeIDs
	{
		kVolumeTypeID = 0,		///< Volume, plain range [0 = -oo , 0.25 = 0dB, 0.5 = +6dB, 1 = +12dB]: plain = 20 * log (4 * norm)
		kPanTypeID,				///< Panning (L-R), plain range [0 = left, 0.5 = center, 1 = right]
		kTuningTypeID,			///< Tuning, plain range [0 = -120.0 (ten octaves down), 0.5 none, 1 = +120.0 (ten octaves up)]
		///< plain = 240 * (norm - 0.5) and norm = plain / 240 + 0.5
		///< oneOctave is 12.0 / 240.0; oneHalfTune = 1.0 / 240.0;
		kVibratoTypeID,			///< Vibrato
		kExpressionTypeID,		///< Expression
		kBrightnessTypeID,		///< Brightness
		kTextTypeID,			///< TODO:
		kPhonemeTypeID,			///< TODO:

		kCustomStart = 200	///< custom note change type ids must start from here (VST3 uses 100000, but we are limited to 8 bits here.)
	};

	enum NoteControlTypeIDs
	{
		kVoiceId,				// (int) MIDI key number.
		kActive,
		kTrigger,
		kGate,
		kPitch,
		kVelocityOn,
		kVelocityOff,
		kPolyAftertouch,

		kPortamentoEnable = 20, // Specific to Unison Module.
	};

	static bool isPolyphonic(int controller);
};
#define MIDDLE_C	65

/* what is typical time between midi controller messages */
/* (controlls ammount of smoothing applied to pitch bend etc) */
/* calculated as fraction of a second i.e 60 -> 1/60 seconds */
#define MIDI_CONTROLLER_SMOOTHING		60
#define MIDI_MSG(chan, type, b2, b3)	(type + chan + (b2 << 8) + (b3 << 16))
extern const wchar_t*		CONTROLLER_ENUM_LIST;
extern std::vector<std::wstring>	CONTROLLER_DESCRIPTION;
extern void init_controller_descriptions();
void		cntrl_update_lsb(unsigned short& var, short lb);
void		cntrl_update_msb(unsigned short& var, short lb);
void		cntrl_update_lsb(short& var, short lb);
void		cntrl_update_msb(short& var, short lb);

/* int cntrl_to_sample( short control_val ); */
float		cntrl_to_fsample(short control_val);
#endif
