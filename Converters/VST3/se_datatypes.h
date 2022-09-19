#if !defined(_se_datatypes_h_inc_)
#define _se_datatypes_h_inc_
#pragma once

// handy macro to pack a 4 char message id into an int
// e.g. int id = id_to_int('d','u','c','k')
#define id_to_int(c1,c2,c3,c4) ((c1) + ((c2) << 8 ) + ((c3) << 16 ) + ((c4) << 24 ))

enum EDirection : uint8_t {DR_IN, DR_OUT, DR_UNUSED, DR_PARAMETER, DR_FEATURE=DR_OUT,DR_CNTRL=DR_IN }; // plug direction
enum EPlugDataType : int8_t { DT_ENUM, DT_TEXT, DT_MIDI2, DT_MIDI = DT_MIDI2, DT_DOUBLE, DT_BOOL, DT_FSAMPLE, DT_FLOAT, DT_VST_PARAM, DT_INT, DT_INT64, DT_BLOB, DT_CLASS, DT_STRING_UTF8, DT_NONE=-1 };  //plug datatype

// Backward compatible verions
typedef int EDirection_int;
typedef int EPlugDataType_int;

// ST_STOP, ST_ONE_OFF obsolete. Use ST_STATIC or ST_RUN.
enum state_type : int8_t { ST_STOP, ST_ONE_OFF, ST_STATIC=1, ST_RUN }; // Order is important for comparissons

enum ug_event_type {
		  UET_STAT_CHANGE__DEPRECATED
	, UET_SUSPEND
		, UET_MIDI__DEPRECATED			// obsolete, used only by SDK2. Use UET_EVENT_MIDI.
		, UET_RUN_FUNCTION__DEPRECATED // old, use UET_RUN_FUNCTION2 instead
	, UET_IO_FUNC
		, UET_UI_NOTIFY__DEPRECATED
		, UET_PROG_CHANGE__DEPRECATED
		, UET_NOTE_MUTE__DEPRECATED
		, UET_START_PORTAMENTO__DEPRECATED
		, UET_WS_TABLE_CHANGE__DEPRECATED
		, UET_DELAYED_GATE__DEPRECATED
		, UET_PARAM_AUTOMATION__DEPRECATED
	, UET_VOICE_REFRESH
	, UET_VOICE_OUTPUT_STATUS
	, UET_NULL			// do nothing
	, UET_GENERIC1		// MODULES CAN USE FOR WHATEVER
	, UET_GENERIC2
	, UET_RUN_FUNCTION2
	, UET_SERVICE_GUI_QUE
	, UET_DEACTIVATE_VOICE
	, UET_FORCE_NEVER_SLEEP
	, UET_PLAY_WAITING_NOTES

	// BELOW HERE. Only events published in SDK, must match events in MP_API.h
	, UET_EVENT_SETPIN = 100
	, UET_EVENT_STREAMING_START
	, UET_EVENT_STREAMING_STOP
	, UET_EVENT_MIDI
	, UET_GRAPH_START
};

// posible plug Flags values
// is this an 'active' input (ie you can't combine voices into it) eg filter cutoff
// or input to non-linear process (clipper, distortion etc)
// obsolete, use IO_LINEAR_INPUT instead (it's the oposit though)
#define IO_POLYPHONIC_ACTIVE	1
// midi channel selection etc should should ignore patch changes
#define IO_IGNORE_PATCH_CHANGE	2
// auto-rename on new connection
#define IO_RENAME				4
// plugs which are automaticly duplicated (like a container's 'spare' plug)
#define IO_AUTODUPLICATE		8
#define IO_FILENAME				16
// ALLOW USER TO SET THE VALUE OF THIS OUTPUt eg on 'constant value' ug
#define IO_SETABLE_OUTPUT		32
// plugs which can be duplicated/deleted by CUG
#define IO_CUSTOMISABLE			64
// plugs which handle multiple inputs, must belong to an Adder ug
#define IO_ADDER				128
// plugs which are private or obsolete, but are enabled on load if connected somewhere
#define IO_HIDE_PIN				256
#define IO_PRIVATE				IO_HIDE_PIN /* obsolete use IO_HIDE_PIN */
#define IO_DISABLE_IF_POS		IO_HIDE_PIN
// set this if this input can handle more than one polyphonic voice
#define IO_LINEAR_INPUT			512
#define IO_UI_COMMUNICATION		1024
#define IO_AUTO_ENUM			0x800
#define IO_HIDE_WHEN_LOCKED		0x1000
// DON'T EXPOSE AS PLUG (TO SAVE SPACE), Deprecated. Use IO_MINIMISED. 
#define IO_PARAMETER_SCREEN_ONLY	0x2000
#define IO_DONT_CHECK_ENUM		0x4000
// don't use IO_UI_DUAL_FLAG by itself, use IO_UI_COMMUNICATION_DUAL
#define IO_UI_DUAL_FLAG			0x8000
// obsolete, use IO_PATCH_STORE instead
#define IO_UI_COMMUNICATION_DUAL (IO_UI_DUAL_FLAG | IO_UI_COMMUNICATION)
// Patch store is similar to dual but for DSP output plugs that appear as input plugs on UI (Output parameters) could consolodate?
#define IO_PATCH_STORE			0x10000
// Private parameter (not exposed to user of VST plugin)
#define IO_PAR_PRIVATE			0x20000
// minimised (not exposed on structure view (only properties window), see also IO_PARAMETER_SCREEN_ONLY
#define IO_MINIMISED			0x40000
#define IO_CONTAINER_PLUG		0x80000
#define IO_OLD_STYLE_GUI_PLUG	0x100000
#define IO_HOST_CONTROL			0x200000
#define IO_PAR_POLYPHONIC		0x400000
// (bit 24 of 32)
#define IO_PAR_POLYPHONIC_GATE	    0x800000

// sdk3 parameter values saved in patch (not output parameters).
#define IO_PARAMETER_PERSISTANT  	0x1000000

// When first connected, copy pin default, metadata etc to module's first parameter.
#define IO_AUTOCONFIGURE_PARAMETER	0x2000000

// Use Text pin like BLOB, ignore locale when translating to UTF-16 to preserve binary data.
#define IO_BINARY_DATA			    0x4000000

// Changes to this pin require re-drawing the whole module (Controls on Parent).
#define IO_REDRAW_ON_CHANGE			0x8000000

#endif
