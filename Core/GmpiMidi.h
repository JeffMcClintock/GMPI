#pragma once
#include <stdint.h>
#include <algorithm>
#include <assert.h>
#include <functional>
#include <math.h>
#include <span>

/*
#include "Core/GmpiMidi.h"

using namespace gmpi::midi;
*/

namespace gmpi
{
namespace midi_utils
{

inline float U7ToFloat(uint8_t v)
{
	constexpr float recip = 1.0f / 127.0f;
	return recip * v;
}

// convert a normalized float to 7-bit
inline uint8_t floatToU7(float f)
{
	return static_cast<uint8_t>(0.5f + f * 127.f);
}

// converts a 14-bit bipolar controller to a normalized value (between 0.0 and 1.0)
inline float bipoler14bitToNormalized(uint8_t msb, uint8_t lsb)
{
	constexpr int centerInt = 0x2000;
	constexpr float scale = 0.5f / (centerInt - 1); // -0.5 -> +0.5

	const int controllerInt = (static_cast<int>(lsb) + (static_cast<int>(msb) << 7)) - centerInt;
	// bender range is 0 -> 8192 (centre) -> 16383
	// which is lopsided (8192 values negative, 8191 positive). So we scale by the smaller number, then clamp any out-of-range value.
	return (std::max)(0.0f, 0.5f + controllerInt * scale);
}

// converts a normalized value to a 14-bit bipolar controler
inline void normalizedToBipoler14bit(float value, uint8_t& lsb_out, uint8_t& msb_out)
{
	const int quantizedControllerValue = (int)(0.5f + value * 0x3FFF);
	lsb_out = static_cast<uint8_t>(quantizedControllerValue & 0x7f);
	msb_out = static_cast<uint8_t>((quantizedControllerValue >> 7) & 0x7f);
}

// convert a float between 0.0 -> 1.0 to a unsigned int 0 -> 0xffffffff
inline uint32_t floatToU32(float v)
{
	// tricky becuase you can't convert a float to u32 without truncating it. (only to signed 32)

	// to avoid overflow, only converts the most significant 30 bits, then shifts, smearing 2 lsbs.
	auto rawValue = static_cast<uint32_t>(v * (static_cast<float>((1 << 30) - 1)));
	rawValue = (std::min)(rawValue, 0x3fffffffu);
	return (rawValue << 2) | (rawValue & 3); // shift and duplicate 2 lsbs
}

// convert a unsigned 8.24 integer fraction to a float. (for note pitch)
inline float u8_24_ToFloat(const uint8_t* m)
{
	// trickyness is we discard lowest bits to avoid overflow to negative.
	constexpr float normalizedToSemitone = 1.0f / static_cast<float>(1 << 16);
	return normalizedToSemitone * float((m[0] << 16) | (m[1] << 8) | m[2]);
}

// convert a unsigned 0.32 integer fraction to a float. (for controllers)
inline float u0_32_ToFloat(const uint8_t* m)
{
	// trickyness is: we discard lowest 8 bits to avoid overflow to negative.
	constexpr float f = 1.0f / (static_cast<float>(1 << 24) - 1);
	return f * float((m[0] << 16) | (m[1] << 8) | m[2]);
}

// upscale a lower resolution value to a higher on.
// always scale the value to the full range
inline uint32_t scaleUp(uint32_t srcVal, int srcBits, int dstBits) {
	// simple bit shift
	int scaleBits = (dstBits - srcBits);
	uint32_t bitShiftedValue = srcVal << scaleBits;
	unsigned int srcCenter = 2 ^ (srcBits - 1);
	if (srcVal <= srcCenter) {
		return bitShiftedValue;
	}
	// expanded bit repeat scheme
	int repeatBits = srcBits - 1;
	int repeatMask = (2 ^ repeatBits) - 1;
	int repeatValue = srcVal & repeatMask;
	if (scaleBits > repeatBits) {
		repeatValue <<= scaleBits - repeatBits;
	}
	else {
		repeatValue >>= repeatBits - scaleBits;
	}
	while (repeatValue != 0) {
		bitShiftedValue |= repeatValue;
		repeatValue >>= repeatBits;
	}
	return bitShiftedValue;
}

inline uint32_t scaleDown(uint32_t srcVal, int srcBits, int dstBits) {
	// simple bit shift
	const int scaleBits = (srcBits - dstBits);
	return srcVal >> scaleBits;
}
} // namespace midi_utils

namespace midi2
{
typedef std::span<const uint8_t> message_view;
}

namespace midi
{
	struct message64
	{
		uint8_t m[8];
	};

	enum MidiStatus //: unsigned char
	{
		MIDI_NoteOff = 0x80,
		MIDI_NoteOn = 0x90,
		MIDI_PolyAfterTouch = 0xA0,
		MIDI_ControlChange = 0xB0,
		MIDI_ProgramChange = 0xC0,
		MIDI_ChannelPressue = 0xD0,
		MIDI_PitchBend = 0xE0,
		MIDI_SystemMessage = 0xF0,
		MIDI_SystemMessageEnd = 0xF7,
	};

	enum MidiSysexTimeMessages
	{
		MIDI_Universal_Realtime = 0x7F,
		MIDI_Universal_NonRealtime = 0x7E,
		MIDI_Sub_Id_Tuning_Standard = 0x08,
	};

	enum MidiLimits
	{
		MIDI_KeyCount = 128,
		MIDI_ControllerCount = 128,
		MIDI_7bit_MinimumValue = 0,
		MIDI_7bit_MaximimumValue = 127,
	};

	enum MidiChannels
	{
		MIDI_ChannelCount = 16,
		MIDI_Channel_MinimumValue = 0,
		MIDI_Channel_MaximimumValue = 15,
	};

	enum Controller
	{
		CC_Bank_lsb = 32,
		CC_Bank_msb = 0,
		CC_HighResolutionVelocityPrefix = 88,
		CC_AllSoundOff = 120,
		CC_ResetAllControllers = 121,
		CC_AllNotesOff = 123,
		CC_MonoOn = 126,
		CC_PolyOn = 127,
	};
}

namespace midi2
{
namespace attribute_type
{
enum
{
	None = 0,
	ManufacturerSpecific = 1,
	ProfileSpecific = 2,
	Pitch = 3,
};
}

enum MessageType //: unsigned char
{
	Utility = 0x0, //  32 bits Utility Messages 
	System = 0x1, //  32 bits System Real Time and System Common Messages (except System Exclusive)
	ChannelVoice32 = 0x2, //  32 bits MIDI 1.0 Channel Voice Messages
	Data64 = 0x3, //  64 bits Data Messages (including System Exclusive)
	ChannelVoice64 = 0x4, //  64 bits MIDI 2.0 Channel Voice Messages
	Data128 = 0x5, // 128 bits Data Messages
	Reserved = 0x6, //  32 bits Reserved for future definition by MMA/AME
};

enum Status //: unsigned char
{
	PolyControlChange = 0x00,
	RPN = 0x02,
	NRPN = 0x03,
	PolyBender = 0x06,

	// classics
	NoteOff = 0x8,
	NoteOn = 0x9,
	PolyAfterTouch = 0xA,
	ControlChange = 0xB,
	ProgramChange = 0xC,
	ChannelPressue = 0xD,
	PitchBend = 0xE,

	PolyNoteManagement = 0xFF,
};

enum PerNoteControllers
{
	PolyModulation = 1, // Modulation
	PolyBreath = 2, // Breath
	PolyPitch = 3, // Absolute Pitch 7.25 – Section 4.2.14.2, see also Poly Bender
	// 4–6 , // Reserved
	PolyVolume = 7, // Volume
	PolyBalance = 8, // Balance
	// 9 , // Reserved
	PolyPan = 10, // Pan
	PolyExpression = 11, // Expression
	// 12–69 , // Reserved
	PolySoundController1 = 70, // Sound Controller 1 Sound Variation
	PolySoundController2 = 71, // Sound Controller 2 Timbre/Harmonic Intensity
	PolySoundController3 = 72, // Sound Controller 3 Release Time
	PolySoundController4 = 73, // Sound Controller 4 Attack Time
	PolySoundController5 = 74, // Sound Controller 5 Brightness
	PolySoundController6 = 75, // Sound Controller 6 Decay Time MMA RP-021 [MMA04]
	PolySoundController7 = 76, //  Sound Controller 7 Vibrato Rate
	PolySoundController8 = 77, //  Sound Controller 8 Vibrato Depth
	PolySoundController9 = 78, //  Sound Controller 9 Vibrato Delay
	PolySoundController10 = 79, //  Sound Controller 10 Undefined
	// 80–90, //  Reserved
	PolyEffects1Depth = 91, //  Effects 1 Depth Reverb Send Level MMA RP-023 [MMA05]
	PolyEffects2Depth = 92, //  Effects 2 Depth (formerly Tremolo Depth)
	PolyEffects3Depth = 93, //  Effects 3 Depth Chorus Send Level MMA RP-023 [MMA05]
	PolyEffects4Depth = 94, //  Effects 4 Depth (formerly Celeste [Detune] Depth)
	PolyEffects5Depth = 95, //  Effects 5 Depth (formerly Phaser Depth)
};

namespace RpnTypes
{
enum {
	PitchBendSensitivity
};
}

inline bool isMidi2Message(std::span<const uint8_t> bytes)
{
	// MIDI 2.0 messages are always at least 4 bytes
	// and first 4-bits are message-type of which only values less than 5 are valid.
	return bytes.size() >= 4 && (bytes[0] >> 4) <= Data128;
}

struct headerInfo
{
	// The most significant 4 bits of every message contain the Message Type (MT).
	uint8_t messageType;
	uint8_t group;
	uint8_t channel;
	uint8_t status;
};

inline headerInfo decodeHeader(midi2::message_view msg)
{
	assert(msg.size() > 0);

	headerInfo hdr{};
	hdr.messageType = static_cast<uint8_t>(msg[0] >> 4);
	hdr.group = static_cast<uint8_t>(msg[0] & 0x0f);

	size_t expectedSize = 0;

	switch (hdr.messageType)
	{
	case 0: //  32 bits Utility Messages 
	case 1: //  32 bits System Real Time and System Common Messages (except System Exclusive)
	case 2: //  32 bits MIDI 1.0 Channel Voice Messages
		expectedSize = 4;
		break;

	case 3: //  64 bits Data Messages (including System Exclusive)
	case 4: //  64 bits MIDI 2.0 Channel Voice Messages
		expectedSize = 8;
		break;

	case 5: // 128 bits Data Messages
		expectedSize = 16;
		break;

	case 6:
	case 7:
		expectedSize = 4; //  32 bits Reserved for future definition by MMA/AME
		break;

	case 8:
	case 9:
	case 0xA:
		expectedSize = 8; //  64 bits Reserved for future definition by MMA/AME
		break;

	case 0xB:
	case 0xC:
		expectedSize = 12; //  64 bits Reserved for future definition by MMA/AME
		break;

	case 0xD:
	case 0xE:
	case 0xF:
		expectedSize = 16; //  128 bits Reserved for future definition by MMA/AME
		break;

	default:
		break;
	}

	const bool valid = expectedSize == msg.size();
	if (!valid)
	{
		hdr.messageType = 0xff; // not recognized
		return hdr;
	}

	if (ChannelVoice64 == hdr.messageType)
	{
		hdr.channel = static_cast<uint8_t>(msg[1] & 0x0f);
		hdr.status = static_cast<uint8_t>(msg[1] >> 4);
	}
	else
	{
		hdr.messageType = 0xff; // not handled
	}

	return hdr;
}

struct noteInfo
{
	uint8_t noteNumber;
	uint8_t attributeType;
	float velocity;
	float attributeValue;
};

inline noteInfo decodeNote(midi2::message_view msg)
{
	assert(msg.size() == 8);

	constexpr float recip = 1.0f / (float)0xffff;
	constexpr float recip2 = 1.0f / (float)(1 << 9);
	return
	{
		msg[2],
		msg[3],
		recip * ((msg[4] << 8) | msg[5]),
		recip2 * ((msg[6] << 8) | msg[7]),
	};
}
struct controller
{
	uint8_t type;
	float value;
};

inline controller decodeController(midi2::message_view msg)
{
	assert(msg.size() == 8);

	return
	{
		msg[2],
		gmpi::midi_utils::u0_32_ToFloat(msg.data() + 4),
	};
}
struct polyController
{
	uint8_t noteNumber;
	uint8_t type;
	float value;
};

inline polyController decodePolyController(midi2::message_view msg)
{
	assert(msg.size() == 8);

	return
	{
		msg[2],
		msg[3],
		gmpi::midi_utils::u0_32_ToFloat(msg.data() + 4),
	};
}

struct RpnInfo
{
	uint16_t rpn;
	uint32_t value;
};

inline RpnInfo decodeRpn(midi2::message_view msg)
{
	assert(msg.size() == 8);

	return
	{
		static_cast<uint16_t>((msg[2] << 8) | msg[3]),
		//				static_cast<uint32_t>((msg[4] << 24) | (msg[5] << 16) | (msg[6] << 8) | msg[7])
						// decode backward compatible 14 bits only.
						static_cast<uint32_t>((msg[4] << 6) | (msg[5] >> 2)) // lower 6 bits are in upper 6 of msg[5]
	};
}

inline midi::message64 makeController(uint8_t controller, float value, uint8_t channel = 0, uint8_t channelGroup = 0)
{
	const auto rawValue = gmpi::midi_utils::floatToU32(value);
	const uint8_t reserved{};

	return midi::message64
	{
		static_cast<uint8_t>((midi2::ChannelVoice64 << 4) | (channelGroup & 0x0f)),
		static_cast<uint8_t>((midi2::ControlChange << 4) | (channel & 0x0f)),
		controller,
		reserved,
		static_cast<uint8_t>((rawValue >> 24)),
		static_cast<uint8_t>((rawValue >> 16) & 0xff),
		static_cast<uint8_t>((rawValue >> 8) & 0xff),
		static_cast<uint8_t>((rawValue >> 0) & 0xff)
	};
}

inline midi::message64 makeBender(float normalized, uint8_t channel = 0, uint8_t channelGroup = 0)
{
	const auto rawValue = gmpi::midi_utils::floatToU32(normalized);

	const uint8_t reserved{};

	return midi::message64
	{
		static_cast<uint8_t>((midi2::ChannelVoice64 << 4) | (channelGroup & 0x0f)),
		static_cast<uint8_t>((midi2::PitchBend << 4) | (channel & 0x0f)),
		reserved,
		reserved,
		static_cast<uint8_t>((rawValue >> 24)),
		static_cast<uint8_t>((rawValue >> 16) & 0xff),
		static_cast<uint8_t>((rawValue >> 8) & 0xff),
		static_cast<uint8_t>((rawValue >> 0) & 0xff)
	};
}

inline midi::message64 makeChannelPressure(float normalized, uint8_t channel = 0, uint8_t channelGroup = 0)
{
	const auto rawValue = gmpi::midi_utils::floatToU32(normalized);

	const uint8_t reserved{};

	return midi::message64
	{
		static_cast<uint8_t>((midi2::ChannelVoice64 << 4) | (channelGroup & 0x0f)),
		static_cast<uint8_t>((midi2::ChannelPressue << 4) | (channel & 0x0f)),
		reserved,
		reserved,
		static_cast<uint8_t>((rawValue >> 24)),
		static_cast<uint8_t>((rawValue >> 16) & 0xff),
		static_cast<uint8_t>((rawValue >> 8) & 0xff),
		static_cast<uint8_t>((rawValue >> 0) & 0xff)
	};
}
inline midi::message64 makeRpnRaw(uint16_t rpn, uint32_t rawValue, uint8_t channel = 0, uint8_t channelGroup = 0)
{
	return midi::message64
	{
		static_cast<uint8_t>((midi2::ChannelVoice64 << 4) | (channelGroup & 0x0f)),
		static_cast<uint8_t>((midi2::RPN << 4) | (channel & 0x0f)),
		static_cast<uint8_t>(rpn >> 8),
		static_cast<uint8_t>(rpn & 0xff),
		static_cast<uint8_t>((rawValue >> 24)),
		static_cast<uint8_t>((rawValue >> 16) & 0xff),
		static_cast<uint8_t>((rawValue >> 8) & 0xff),
		static_cast<uint8_t>((rawValue >> 0) & 0xff)
	};
}
inline midi::message64 makeNrpnRaw(uint16_t rpn, uint32_t rawValue, uint8_t channel = 0, uint8_t channelGroup = 0)
{
	return midi::message64
	{
		static_cast<uint8_t>((midi2::ChannelVoice64 << 4) | (channelGroup & 0x0f)),
		static_cast<uint8_t>((midi2::NRPN << 4) | (channel & 0x0f)),
		static_cast<uint8_t>(rpn >> 8),
		static_cast<uint8_t>(rpn & 0xff),
		static_cast<uint8_t>((rawValue >> 24)),
		static_cast<uint8_t>((rawValue >> 16) & 0xff),
		static_cast<uint8_t>((rawValue >> 8) & 0xff),
		static_cast<uint8_t>((rawValue >> 0) & 0xff)
	};
}

inline midi::message64 makePolyController(uint8_t noteNumber, uint8_t controller, float value, uint8_t channel = 0, uint8_t channelGroup = 0)
{
	const auto rawValue = gmpi::midi_utils::floatToU32(value);

	return midi::message64
	{
		static_cast<uint8_t>((midi2::ChannelVoice64 << 4) | (channelGroup & 0x0f)),
		static_cast<uint8_t>((midi2::PolyControlChange << 4) | (channel & 0x0f)),
		noteNumber,
		controller,
		static_cast<uint8_t>((rawValue >> 24)),
		static_cast<uint8_t>((rawValue >> 16) & 0xff),
		static_cast<uint8_t>((rawValue >> 8) & 0xff),
		static_cast<uint8_t>((rawValue >> 0) & 0xff)
	};
}

inline midi::message64 makePolyBender(uint8_t noteNumber, float normalized, uint8_t channel = 0, uint8_t channelGroup = 0)
{
	const auto rawValue = gmpi::midi_utils::floatToU32(normalized);

	const uint8_t reserved{};

	return midi::message64
	{
		static_cast<uint8_t>((midi2::ChannelVoice64 << 4) | (channelGroup & 0x0f)),
		static_cast<uint8_t>((midi2::PolyBender << 4) | (channel & 0x0f)),
		noteNumber,
		reserved,
		static_cast<uint8_t>((rawValue >> 24)),
		static_cast<uint8_t>((rawValue >> 16) & 0xff),
		static_cast<uint8_t>((rawValue >> 8) & 0xff),
		static_cast<uint8_t>((rawValue >> 0) & 0xff)
	};
}

inline midi::message64 makePolyPressure(uint8_t noteNumber, float normalized, uint8_t channel = 0, uint8_t channelGroup = 0)
{
	const auto rawValue = gmpi::midi_utils::floatToU32(normalized);

	const uint8_t reserved{};

	return midi::message64
	{
		static_cast<uint8_t>((midi2::ChannelVoice64 << 4) | (channelGroup & 0x0f)),
		static_cast<uint8_t>((midi2::PolyAfterTouch << 4) | (channel & 0x0f)),
		noteNumber,
		reserved,
		static_cast<uint8_t>((rawValue >> 24)),
		static_cast<uint8_t>((rawValue >> 16) & 0xff),
		static_cast<uint8_t>((rawValue >> 8) & 0xff),
		static_cast<uint8_t>((rawValue >> 0) & 0xff)
	};
}

inline midi::message64 makeNotePitchMessage(uint8_t noteNumber, float semitone, uint8_t channel = 0, uint8_t channelGroup = 0)
{
	// 7 bits: Pitch in semitones, based on default Note Number equal temperament scale.
	// 25 bits: Fractional Pitch above Note Number (i.e., fraction of one semitone).
	constexpr float semitoneToNormalized = static_cast<float>(1 << 24);

	const int32_t rawValue = static_cast<uint32_t>(semitone * semitoneToNormalized);
	return midi::message64
	{
		static_cast<uint8_t>((midi2::ChannelVoice64 << 4) | (channelGroup & 0x0f)),
		static_cast<uint8_t>((midi2::PolyControlChange << 4) | (channel & 0x0f)),
		noteNumber,
		PolyPitch,
		static_cast<uint8_t>((rawValue >> 24)),
		static_cast<uint8_t>((rawValue >> 16) & 0xff),
		static_cast<uint8_t>((rawValue >> 8) & 0xff),
		static_cast<uint8_t>((rawValue >> 0) & 0xff)
	};
}

inline midi::message64 makeProgramChange(uint8_t program, int bank = -1, uint8_t channel = 0, uint8_t channelGroup = 0)
{
	const uint8_t reserved{};
	const uint8_t optionFlags = bank < 0 ? 0 : 1; // 1 = bank valid
	const int sendBank = bank < 0 ? 0 : bank;

	return midi::message64
	{
		static_cast<uint8_t>((midi2::ChannelVoice64 << 4) | (channelGroup & 0x0f)),
		static_cast<uint8_t>((midi2::ProgramChange << 4) | (channel & 0x0f)),
		reserved,
		optionFlags,
		static_cast<uint8_t>(program & 0x7f),
		reserved,
		static_cast<uint8_t>((sendBank >> 7) & 0x7f), // 7-bits only
		static_cast<uint8_t>((sendBank >> 0) & 0x7f) // 7-bits only
	};
}

inline midi::message64 makeSysex(const uint8_t*& data, int& remain, bool& isFirst, uint8_t channelGroup = 0)
{
	const int chunkSize = 6;
	int status = 0x00;

	if (isFirst)
	{
		status = remain <= chunkSize ? 0x00 : 0x10; // complete, or start of several
	}
	else
	{
		status = remain <= chunkSize ? 0x30 : 0x20; // end, or more to come
	}

	const int bytesThisChunk = (std::min)(remain, chunkSize);

	midi::message64 r
	{
		static_cast<uint8_t>((midi2::Data64 << 4) | (channelGroup & 0x0f)),
		static_cast<uint8_t>(status | bytesThisChunk),
		0,
		0,
		0,
		0,
		0,
		0
	};

	uint8_t* dst = &r.m[2];
	int i = 0;
	for (; i < bytesThisChunk; ++i)
	{
		*dst++ = *data++;
	}

	remain -= bytesThisChunk;
	isFirst = false;

	return r;
}

inline float decodeNotePitch(midi2::message_view msg)
{
	assert(msg.size() == 8);

	return gmpi::midi_utils::u8_24_ToFloat(msg.data() + 4);
}


class NoteMapper
{
protected:
	struct NoteInfo
	{
		int32_t noteId;		// VST3 note ID
		int16_t pitch;		// in MIDI semitones. hmm how do they handle microtuning?
		uint8_t MidiKeyNumber;
		bool held;
	};

	NoteInfo noteIds[128] = {};
	int noteIdsRoundRobin = 0;

public:
	NoteMapper()
	{
		for (size_t i = 0; i < std::size(noteIds); ++i)
			noteIds[i].MidiKeyNumber = static_cast<uint8_t>(i);
	}

	NoteInfo* findNote(int noteId)
	{
		for (int i = 0; i < 128; ++i)
		{
			if (noteIds[i].noteId == noteId)
			{
				return &noteIds[i];
			}
		}

		return {};
	}

	NoteInfo& allocateNote(int noteId, int preferredNoteNumber)
	{
		int res = -1;
		for (int i = 0; i < 128; ++i)
		{
			if (noteIds[i].noteId == noteId)
			{
				res = i;
				break;
			}
		}
#if 1 // set 0 to enable pure round-robin allocation (better for flushing out bugs).
		if (res == -1 && !noteIds[preferredNoteNumber].held)
		{
			res = preferredNoteNumber;
		}
#endif
		if (res == -1)
		{
			for (int i = 0; i < 128; ++i)
			{
				noteIdsRoundRobin = (noteIdsRoundRobin + 1) & 0x7f;

				if (!noteIds[noteIdsRoundRobin].held)
				{
					break;
				}
			}
			res = noteIdsRoundRobin;
		}

		assert(res >= 0 && res < 128);

		noteIds[res].held = true;
		noteIds[res].noteId = noteId;
		return noteIds[res];
	}
};

inline midi::message64 makeNoteOnMessageWithPitch(uint8_t noteNumber, float velocity, float pitch, uint8_t channel = 0, uint8_t channelGroup = 0)
{
	constexpr float scale = static_cast<float>(1 << 16) - 1;
	const int32_t VelocityRaw = static_cast<int32_t>(velocity * scale);

	constexpr float scale2 = static_cast<float>(1 << 9); //  Pitch is a Q7.9 fixed-point unsigned integer that specifies a pitch in semitones
	const int32_t PitchRaw = static_cast<int32_t>(pitch * scale2);

	return midi::message64
	{
		static_cast<uint8_t>((midi2::ChannelVoice64 << 4) | (channelGroup & 0x0f)),
		static_cast<uint8_t>((midi2::NoteOn << 4) | (channel & 0x0f)),
		noteNumber,
		midi2::attribute_type::Pitch,
		static_cast<uint8_t>(VelocityRaw >> 8),
		static_cast<uint8_t>(VelocityRaw & 0xff),
		static_cast<uint8_t>(PitchRaw >> 8),
		static_cast<uint8_t>(PitchRaw & 0xff),
	};
}

inline midi::message64 makeNoteOnMessage(uint8_t noteNumber, float velocity, uint8_t channel = 0, uint8_t channelGroup = 0)
{
	constexpr float scale = static_cast<float>(1 << 16) - 1;

	const int32_t rawValue = static_cast<int32_t>(velocity * scale);
	return midi::message64
	{
		static_cast<uint8_t>((midi2::ChannelVoice64 << 4) | (channelGroup & 0x0f)),
		static_cast<uint8_t>((midi2::NoteOn << 4) | (channel & 0x0f)),
		noteNumber,
		midi2::attribute_type::None,
		static_cast<uint8_t>(rawValue >> 8),
		static_cast<uint8_t>(rawValue & 0xff),
		0,
		0
	};
}

inline midi::message64 makeNoteOffMessage(uint8_t noteNumber, float velocity, uint8_t channel = 0, uint8_t channelGroup = 0)
{
	constexpr float scale = static_cast<float>(1 << 16) - 1;
	const int32_t rawValue = static_cast<int32_t>(velocity * scale);

	return midi::message64
	{
		static_cast<uint8_t>((midi2::ChannelVoice64 << 4) | (channelGroup & 0x0f)),
		static_cast<uint8_t>((midi2::NoteOff << 4) | (channel & 0x0f)),
		noteNumber,
		midi2::attribute_type::None,
		static_cast<uint8_t>(rawValue >> 8),
		static_cast<uint8_t>(rawValue & 0xff),
		0,
		0
	};
}

} // namspace midi2
	
namespace midi1
{
namespace status_type
{
enum
{
	NoteOff = 0x8,
	NoteOn = 0x9,
	PolyAfterTouch = 0xA,
	ControlChange = 0xB,
	ProgramChange = 0xC,
	ChannelPressure = 0xD,
	PitchBend = 0xE,
	System = 0xF,
};
}

struct headerInfo
{
	uint8_t status;
	uint8_t channel;
};

inline headerInfo decodeHeader(gmpi::midi2::message_view msg)
{
	assert(msg.size() > 0);

	uint8_t status = msg[0] >> 4;
	if (status_type::NoteOn == status && 0 == msg[2])
	{
		status = status_type::NoteOff;
	}

	return
	{
		status,
		static_cast<uint8_t>(msg[0] & 0x0f)
	};
}

struct noteInfo
{
	uint8_t noteNumber;
	float velocity;
};

struct controllerInfo
{
	uint8_t controllerNumber;
	float value;
};

struct channelPressureInfo
{
	float pressure;
};

inline noteInfo decodeNote(gmpi::midi2::message_view msg)
{
	assert(msg.size() == 3);

	constexpr float recip = 1.0f / (float)0x7f;
	return
	{
		msg[1],
		msg[2] * recip
	};
}

inline channelPressureInfo decodeChannelPressure(gmpi::midi2::message_view msg)
{
	assert(msg.size() == 2);

	constexpr float recip = 1.0f / (float)0x7f;
	return
	{
		msg[1] * recip
	};
}

inline controllerInfo decodeControllerMsg(gmpi::midi2::message_view msg)
{
	assert(msg.size() == 3);

	constexpr float recip = 1.0f / (float)0x7f;
	return
	{
		msg[1],
		msg[2] * recip
	};
}

inline float decodeBender(gmpi::midi2::message_view msg)
{
	return midi_utils::bipoler14bitToNormalized(msg[2], msg[1]);
}

#if 0
inline float decodeController(uint8_t v)
{
	constexpr float recip = 1.0f / 127.0f;
	return recip * v;
}
#endif
} // namespace midi1

// MIDI CONVERTERS
namespace midi
{
// Convert MIDI 1.0 to MIDI 2.0
class MidiConverter2
{
	std::function<void(const midi2::message_view, int timestamp)> sink;

	// RPN
	unsigned short incoming_rpn[16] = {};
	unsigned short incoming_nrpn[16] = {};
	unsigned short incoming_rpn_value = {};
	// RPNs are 14 bit values, so this value never occurs, represents "no rpn"
	inline static const unsigned short NULL_RPN = 0xffff;
	void cntrl_update_msb(unsigned short& var, short hb) const
	{
		var = (var & 0x7f) + (hb << 7);	// mask off high bits and replace
	}
	void cntrl_update_lsb(unsigned short& var, short lb) const
	{
		var = (var & 0x3F80) + lb;			// mask off low bits and replace
	}

public:
	MidiConverter2(std::function<void(const midi2::message_view, int)> psink) :
		sink(psink)
	{
		std::fill(std::begin(incoming_rpn), std::end(incoming_rpn), NULL_RPN);
		std::fill(std::begin(incoming_nrpn), std::end(incoming_nrpn), NULL_RPN);
	}

	void processMidi(const midi2::message_view msg, int timestamp)
	{
		// MIDI 2.0 messages need no conversion
		if (midi2::isMidi2Message(msg))
		{
			// no conversion.
			sink(msg, timestamp);
		}

		const auto header = midi1::decodeHeader(msg);

		switch (header.status)
		{
		case midi1::status_type::NoteOn:
		{
			auto note = midi1::decodeNote(msg);

			const auto msgout = midi2::makeNoteOnMessage(
				note.noteNumber,
				note.velocity,
				header.channel
			);

			sink({ msgout.m }, timestamp);
		}
		break;

		case midi1::status_type::NoteOff:
		{
			const auto note = midi1::decodeNote(msg);

			const auto msgout = midi2::makeNoteOffMessage(
				note.noteNumber,
				note.velocity,
				header.channel
			);

			sink({ msgout.m }, timestamp);
		}
		break;

		case midi1::status_type::PitchBend:
		{
			const auto msgout = midi2::makeBender(
				midi1::decodeBender(msg),
				header.channel
			);

			sink({ msgout.m }, timestamp);
		}
		break;

		case midi1::status_type::ChannelPressure:
		{
			auto channelPressure = midi1::decodeChannelPressure(msg);

			const auto msgout = midi2::makeChannelPressure(
				channelPressure.pressure,
				header.channel
			);

			sink({ msgout.m }, timestamp);
		}
		break;

		case midi1::status_type::PolyAfterTouch:
		{
			const auto controller = midi1::decodeControllerMsg(msg); // not a normal controller, but same fields.

			const auto msgout = midi2::makePolyPressure(
				controller.controllerNumber, // actually key-number
				controller.value,
				header.channel
			);

			sink({ msgout.m }, timestamp);
		}
		break;

		case midi1::status_type::ControlChange:
		{
			auto controller = midi1::decodeControllerMsg(msg);

			// incoming change of NRPN or RPN (registered parameter number)
			switch (controller.controllerNumber)
			{
			case 101:
				cntrl_update_msb(incoming_rpn[header.channel], static_cast<short>(msg[2]));
				incoming_nrpn[header.channel] = NULL_RPN;	// RPNS are coming, cancel NRPNs msgs
				break;

			case 100:
				cntrl_update_lsb(incoming_rpn[header.channel], static_cast<short>(msg[2]));
				incoming_nrpn[header.channel] = NULL_RPN;	// RPNS are coming, cancel NRPNs msgs
				break;

			case 98:
				cntrl_update_lsb(incoming_nrpn[header.channel], static_cast<short>(msg[2]));
				incoming_rpn[header.channel] = NULL_RPN;	// NRPNs are coming, cancel RPNS msgs
				break;
			case 99:
				cntrl_update_msb(incoming_nrpn[header.channel], static_cast<short>(msg[2]));
				incoming_rpn[header.channel] = NULL_RPN;	// NRPNs are coming, cancel RPNS msgs
				break;

			case 6:	// RPN_CONTROLLER MSB
			{
				cntrl_update_msb(incoming_rpn_value, static_cast<short>(msg[2]));

				if (incoming_rpn[header.channel] != NULL_RPN)
				{
					const auto msgout = midi2::makeRpnRaw(
						incoming_rpn[header.channel],
						gmpi::midi_utils::scaleUp(incoming_rpn_value, 14, 32), // 14 bit value converted to 32-bit value.
						header.channel
					);
					sink({ msgout.m }, timestamp);
				}
				else
				{
					const auto msgout = midi2::makeNrpnRaw(
						incoming_rpn[header.channel],
						gmpi::midi_utils::scaleUp(incoming_rpn_value, 14, 32), // 14 bit value converted to 32-bit value.
						header.channel
					);
					sink({ msgout.m }, timestamp);
				}
			}
			break;

			case 38:	// RPN_CONTROLLER LSB
			{
				cntrl_update_lsb(incoming_rpn_value, static_cast<short>(msg[2]));

				if (incoming_rpn[header.channel] != NULL_RPN)
				{
					const auto msgout = midi2::makeRpnRaw(
						incoming_rpn[header.channel],
						gmpi::midi_utils::scaleUp(incoming_rpn_value, 14, 32), // 14 bit value converted to 32-bit value.
						header.channel
					);

					sink({ msgout.m }, timestamp);
				}
				else
				{
					const auto msgout = midi2::makeNrpnRaw(
						incoming_rpn[header.channel],
						gmpi::midi_utils::scaleUp(incoming_rpn_value, 14, 32), // 14 bit value converted to 32-bit value.
						header.channel
					);
					sink({ msgout.m }, timestamp);
				}
			}
			break;

			default:
			{
				const auto msgout = midi2::makeController(
					controller.controllerNumber,
					controller.value,
					header.channel
				);

				sink({ msgout.m }, timestamp);
			}
			break;
			}

		}
		break;

		case midi1::status_type::ProgramChange:
		{
			const auto bank = -1;
			const auto msgout = midi2::makeProgramChange(
				msg[1],
				bank,
				header.channel
			);

			sink({ msgout.m }, timestamp);
		}
		break;

		case midi1::status_type::System:
		{
			[[maybe_unused]] const auto systemMessageType = header.channel;

			if (systemMessageType == 0) // SYSEX
			{
				bool isFirst = true;
				int remain = static_cast<int>(msg.size()) - 2; // skip leading F0 and trailing F7
				const uint8_t* src = msg.data() + 1; // skip leading F0
				while (remain > 0)
				{
					const auto msgout = midi2::makeSysex(src, remain, isFirst);
					sink({ msgout.m }, timestamp);
				}
			}
			/* TODO other system messages
								clock(decimal 248, hex 0xF8)
								start(decimal 250, hex 0xFA)
								continue (decimal 251, hex 0xFB)
								stop(decimal 252, hex 0xFC)
			*/
#if 0
			auto controller = midi1::decodeControllerMsg(msg);

			const auto msgout = midi2::makeController(
				controller.controllerNumber,
				controller.value,
				header.channel
			);

			sink({ msgout.m }, timestamp);
#endif
		}
		break;
		}
	}

	void setSink(std::function<void(const midi2::message_view, int)> psink)
	{
		sink = psink;
	}
};

// Convert MPE to MIDI 2.0
class MpeConverter : public midi2::NoteMapper
{
	std::function<void(const midi2::message_view, int timestamp)> sink;

	// normalized bender for each chan. center = 0.5
	static const int maxChannels = 16;

	float channelBender[maxChannels];
	float channelPressure[maxChannels];
	float channelBrightness[maxChannels];

	int lowerZoneMasterChannel = 0;
	int upperZoneMasterChannel = -1;

	// for not specifically MPE stuff, just use a normal MIDI 1.0 -> 2.0 converter
	MidiConverter2 midiConverter2;

public:
	MpeConverter(std::function<void(const midi2::message_view, int)> psink) :
		sink(psink),
		midiConverter2([this](const midi2::message_view msg, int timestamp) { sink(msg, timestamp); })
	{
		std::fill(std::begin(channelBender), std::end(channelBender), 0.5f);
		std::fill(std::begin(channelBrightness), std::end(channelBrightness), 0.0f);
		std::fill(std::begin(channelPressure), std::end(channelPressure), 0.0f);
	}

	void processMidi(const midi2::message_view msg, int timestamp)
	{
		// MIDI 2.0 messages need no conversion
		if (midi2::isMidi2Message(msg))
		{
			// no conversion.
			sink(msg, timestamp);
		}
		const auto header = midi1::decodeHeader(msg);

		// 'Master' MIDI Channels are reserved for conveying messages that apply to the entire Zone.
		// Just convert to MIDI 2.0 and pass them though unchanged.
		if (header.channel == lowerZoneMasterChannel || header.channel == upperZoneMasterChannel)
		{
			midiConverter2.processMidi(msg, timestamp);
			return;
		}

		// for an MPE member channel, only note on/off, pitch-bend, and brightness are allowed.
		switch (header.status)
		{
		case midi1::status_type::NoteOn:
		{
			auto note = midi1::decodeNote(msg);
			const auto MpeId = note.noteNumber | (header.channel << 7);
			auto& keyInfo = allocateNote(MpeId, note.noteNumber);

			//			_RPTN(0, "MPE Note-on %d => %d\n", note.noteNumber, keyInfo.MidiKeyNumber);

			// MIDI 2.0 does not automatically reset per-note controls. MPE is expected to.
			// reset per-note bender
			{
				const auto msgout = midi2::makePolyBender(
					keyInfo.MidiKeyNumber,
					channelBender[header.channel]
				);

				//				_RPTN(0, "MPE: Bender %d: %f\n", keyInfo.MidiKeyNumber, channelBender[header.channel]);
				sink({ msgout.m }, timestamp);
			}

			// reset per-note brightness
			{
				const auto msgout = midi2::makePolyController(
					keyInfo.MidiKeyNumber,
					midi2::PolySoundController5, // Brightness
					channelBrightness[header.channel]
				);

				//				_RPTN(0, "MPE: Brightness %d: %f\n", keyInfo.MidiKeyNumber, channelBrightness[header.channel]);
				sink({ msgout.m }, timestamp);
			}

			// reset per-note pressure
			{
				const auto msgout = midi2::makePolyPressure(
					keyInfo.MidiKeyNumber,
					channelPressure[header.channel]
				);

				//				_RPTN(0, "MPE: Pressure %d: %f\n", keyInfo.MidiKeyNumber, channelPressure[header.channel]);
				sink({ msgout.m }, timestamp);
			}

			// !!! TODO: pitch from tuning_table[note_num], not just note_num. (and float)
			// hmm  does midi tuning work in MPE, since it's already microtonal?
			if (keyInfo.pitch != note.noteNumber)
			{
				keyInfo.pitch = note.noteNumber;
				//						_RPTN(0, "      ..pitch = %d\n", (int)keyInfo.pitch);

				// !!! problem!!! w SynthEdit retaining tuning
				// This override is valid only for the one Note containing the Attribute #3: Pitch 7.9; it is not valid for any subsequent Notes
				//const auto msgout = midi2::makeNoteOnMessageWithPitch(
				//	keyInfo.MidiKeyNumber,
				//	note.velocity,
				//	keyInfo.pitch
				//);

				const auto msgout = midi2::makeNotePitchMessage(
					keyInfo.MidiKeyNumber,
					keyInfo.pitch
				);

				sink({ msgout.m }, timestamp);
			}

			{
				//				_RPTN(0, "      ..existing pitch = %d\n", (int)keyInfo.pitch);
				const auto msgout = midi2::makeNoteOnMessage(
					keyInfo.MidiKeyNumber,
					note.velocity
				);

				sink({ msgout.m }, timestamp);
			}
		}
		break;

		case midi1::status_type::NoteOff:
		{
			auto note = midi1::decodeNote(msg);
			const auto MpeId = note.noteNumber | (header.channel << 7);
			auto keyInfo = findNote(MpeId);

			if (keyInfo)
			{
				keyInfo->held = false;

				const auto msgout = midi2::makeNoteOffMessage(
					keyInfo->MidiKeyNumber,
					note.velocity
				);

				//pinMIDIOut.send(out.begin(), out.size());
				sink({ msgout.m }, timestamp);
			}
		}
		break;

		case midi1::status_type::PitchBend:
		{
			channelBender[header.channel] = midi1::decodeBender(msg);

			// TODO: repect any updates to bend-range sent using RPN (48 is default) (on manager channel?

			// find whatever note is playing on this channel. Assumption is only held notes can receive benders.
			// might not hold true for DAW automation which is drawn-on after note-off time
			for (auto& info : noteIds)
			{
				// "The note will cease to be affected by Pitch Bend messages on its Channel after the Note Off message occurs" - spec.
				if (info.held && header.channel == (info.noteId >> 7))
				{
					const auto msgout = midi2::makePolyBender(
						info.MidiKeyNumber,
						channelBender[header.channel]
					);

					//					_RPTN(0, "MPE: Bender %d: %f\n", info.MidiKeyNumber, channelBender[header.channel]);
					sink({ msgout.m }, timestamp);
				}
			}
		}
		break;

		case midi1::status_type::ChannelPressure:
		{
			channelPressure[header.channel] = midi_utils::U7ToFloat(msg[1]);

			// find whatever note is playing on this channel. Assumption is only held notes can receive benders etc.
			// might not hold true for DAW automation which is drawn-on after note-off time
			for (auto& info : noteIds)
			{
				if (info.held && header.channel == (info.noteId >> 7))
				{
					//					_RPTN(0, "MPE: Pressure %d %f\n", info.MidiKeyNumber, normalised);
					const auto msgout = midi2::makePolyPressure(
						info.MidiKeyNumber,
						channelPressure[header.channel]
					);
					//pinMIDIOut.send(out.begin(), out.size());
					sink({ msgout.m }, timestamp);
				}
			}
		}
		break;

		case midi1::status_type::ControlChange:
		{
			// 74 brightness
			if (74 != msg[1])
				break;

			channelBrightness[header.channel] = midi_utils::U7ToFloat(msg[2]);

			// find whatever note is playing on this channel. Assumption is only held notes can receive benders etc.
			// might not hold true for DAW automation which is drawn-on after note-off time
			for (auto& info : noteIds)
			{
				if (info.held && header.channel == (info.noteId >> 7))
				{
					const auto msgout = midi2::makePolyController(
						info.MidiKeyNumber,
						midi2::PolySoundController5, // Brightness
						channelBrightness[header.channel]
					);

					sink({ msgout.m }, timestamp);
					//					_RPTN(0, "MPE: Brightness %d %f (%d)\n", info.MidiKeyNumber, channelBrightness[header.channel], msg[2]);
				}
			}
		}
		break;
		}
	}

	void setSink(std::function<void(const midi2::message_view, int)> psink)
	{
		sink = psink;
	}
};

// Convert 'fat' MPE to MIDI 2.0
// i.e. MPE that has already been naively converted into MIDI 2.0
class FatMpeConverter : public midi2::NoteMapper
{
	std::function<void(const midi2::message_view, int timestamp)> sink;

	// normalized bender for each chan. center = 0.5
	static const int maxChannels = 16;

	float channelBender[maxChannels];
	float channelPressure[maxChannels];
	float channelBrightness[maxChannels];

	int lowerZoneMasterChannel = -1;
	int upperZoneMasterChannel = -1;
	int lower_zone_size = 0; // both 0 = not MPE mode
	int upper_zone_size = 0;

	bool MpeModeDetected = false;
public:

	FatMpeConverter(std::function<void(const midi2::message_view, int)> psink) :
		sink(psink)
	{
		std::fill(std::begin(channelBender), std::end(channelBender), 0.5f);
		std::fill(std::begin(channelBrightness), std::end(channelBrightness), 0.0f);
		std::fill(std::begin(channelPressure), std::end(channelPressure), 0.0f);
	}

	void processMidi(const midi2::message_view msg, int timestamp)
	{
		// This class expects MIDI 2.0 input always.
		assert(midi2::isMidi2Message(msg));

		const auto header = midi2::decodeHeader(msg);

		if (header.messageType != midi2::ChannelVoice64 || header.channel > 15)
			return;

		// Handle MPE Configuration messages. (MCM)
		if (midi2::Status::RPN == header.status && (header.channel == 0 || header.channel == 15))
		{
			const auto rpn = midi2::decodeRpn(msg);

			if (rpn.rpn == 6) // number of channels in MPE zone
			{
				if (0 == header.channel)
				{
					lower_zone_size = rpn.value >> 7; // MSB only

					if (lower_zone_size)
					{
						lowerZoneMasterChannel = header.channel;
						upper_zone_size = (std::min)(upper_zone_size, 14 - lower_zone_size);
					}
					else
						lowerZoneMasterChannel = -1;
				}
				else
				{
					upper_zone_size = rpn.value >> 7;

					if (upper_zone_size)
					{
						upperZoneMasterChannel = header.channel;
						lower_zone_size = (std::min)(lower_zone_size, 14 - upper_zone_size);
					}
					else
						upperZoneMasterChannel = -1;
				}

				// (MPE Mode) shall be enabled in a controller or a synthesizer when at least one MPE Zone is configured.
				// Setting both Zones on the Manager Channels to use no Channels, shall deactivate the MPE Mode.
				MpeModeDetected = lower_zone_size || upper_zone_size;
			}
		}

		// 'Master' MIDI Channels are reserved for conveying messages that apply to the entire Zone.
		if (header.channel == lowerZoneMasterChannel || header.channel == upperZoneMasterChannel)
		{
			// no conversion for master channels
			sink(msg, timestamp);
			return;
		}

		const bool isLowerZone = lower_zone_size > 0 && header.channel <= lower_zone_size;
		const bool isUpperZone = upper_zone_size > 0 && header.channel >= 15 - upper_zone_size;
		if (!isUpperZone && !isLowerZone)
		{
			// no conversion.
			sink(msg, timestamp);
			return;
		}

		// for an MPE member channel, only note on/off, pitch-bend, pressure, and brightness are allowed.
		switch (header.status)
		{
		case midi2::Status::NoteOn:
		{
			const auto note = midi2::decodeNote(msg);
			const auto MpeId = note.noteNumber | (header.channel << 7);
			auto& keyInfo = allocateNote(MpeId, note.noteNumber);

			//			_RPTN(0, "MPE Note-on %d => %d\n", note.noteNumber, keyInfo.MidiKeyNumber);

			// MIDI 2.0 does not automatically reset per-note controls. MPE is expected to.
			// reset per-note bender
			{
				const auto msgout = midi2::makePolyBender(
					keyInfo.MidiKeyNumber,
					channelBender[header.channel]
				);

				//				_RPTN(0, "MPE: Bender %d: %f\n", keyInfo.MidiKeyNumber, channelBender[header.channel]);
				sink({ msgout.m }, timestamp);
			}

			// reset per-note brightness
			{
				const auto msgout = midi2::makePolyController(
					keyInfo.MidiKeyNumber,
					midi2::PolySoundController5, // Brightness
					channelBrightness[header.channel]
				);

				//				_RPTN(0, "MPE: Brightness %d: %f\n", keyInfo.MidiKeyNumber, channelBrightness[header.channel]);
				sink({ msgout.m }, timestamp);
			}

			// reset per-note pressure
			{
				const auto msgout = midi2::makePolyPressure(
					keyInfo.MidiKeyNumber,
					channelPressure[header.channel]
				);

				//				_RPTN(0, "MPE: Pressure %d: %f\n", keyInfo.MidiKeyNumber, channelPressure[header.channel]);
				sink({ msgout.m }, timestamp);
			}

			// !!! TODO: pitch from tuning_table[note_num], not just note_num. (and float)
			// hmm  does midi tuning work in MPE, since it's already microtonal?
			if (keyInfo.pitch != note.noteNumber)
			{
				keyInfo.pitch = note.noteNumber;
				//						_RPTN(0, "      ..pitch = %d\n", (int)keyInfo.pitch);

				// !!! problem!!! w SynthEdit retaining tuning
				// This override is valid only for the one Note containing the Attribute #3: Pitch 7.9; it is not valid for any subsequent Notes
				//const auto msgout = midi2::makeNoteOnMessageWithPitch(
				//	keyInfo.MidiKeyNumber,
				//	note.velocity,
				//	keyInfo.pitch
				//);

				const auto msgout = midi2::makeNotePitchMessage(
					keyInfo.MidiKeyNumber,
					keyInfo.pitch
				);

				sink({ msgout.m }, timestamp);
			}

			{
				//				_RPTN(0, "      ..existing pitch = %d\n", (int)keyInfo.pitch);
				const auto msgout = midi2::makeNoteOnMessage(
					keyInfo.MidiKeyNumber,
					note.velocity
				);

				sink({ msgout.m }, timestamp);
			}
		}
		break;

		case midi2::Status::NoteOff:
		{
			const auto note = midi2::decodeNote(msg);
			const auto MpeId = note.noteNumber | (header.channel << 7);
			auto keyInfo = findNote(MpeId);

			if (keyInfo)
			{
				keyInfo->held = false;

				const auto msgout = midi2::makeNoteOffMessage(
					keyInfo->MidiKeyNumber,
					note.velocity
				);

				sink({ msgout.m }, timestamp);
			}
		}
		break;

		case midi2::Status::PitchBend:
		{
			channelBender[header.channel] = midi2::decodeController(msg).value;

			// TODO: repect any updates to bend-range sent using RPN (48 is default) (on manager channel?

			// find whatever note is playing on this channel. Assumption is only held notes can receive benders.
			// might not hold true for DAW automation which is drawn-on after note-off time
			for (auto& info : noteIds)
			{
				// "The note will cease to be affected by Pitch Bend messages on its Channel after the Note Off message occurs" - spec.
				if (info.held && header.channel == (info.noteId >> 7))
				{
					const auto msgout = midi2::makePolyBender(
						info.MidiKeyNumber,
						channelBender[header.channel]
					);

					//					_RPTN(0, "MPE: Bender %d: %f\n", info.MidiKeyNumber, channelBender[header.channel]);
					sink({ msgout.m }, timestamp);
				}
			}
		}
		break;

		case midi2::Status::ChannelPressue:
		{
			channelPressure[header.channel] = midi2::decodeController(msg).value;

			// find whatever note is playing on this channel. Assumption is only held notes can receive benders etc.
			// might not hold true for DAW automation which is drawn-on after note-off time
			for (auto& info : noteIds)
			{
				if (info.held && header.channel == (info.noteId >> 7))
				{
					//					_RPTN(0, "MPE: Pressure %d %f\n", info.MidiKeyNumber, normalised);
					const auto msgout = midi2::makePolyPressure(
						info.MidiKeyNumber,
						channelPressure[header.channel]
					);

					sink({ msgout.m }, timestamp);
				}
			}
		}
		break;

		case midi2::Status::ControlChange:
		{
			const auto controller = midi2::decodeController(msg);

			// 74 brightness
			if (74 != controller.type)
				break;

			channelBrightness[header.channel] = controller.value;

			// find whatever note is playing on this channel. Assumption is only held notes can receive benders etc.
			// might not hold true for DAW automation which is drawn-on after note-off time
			for (auto& info : noteIds)
			{
				if (info.held && header.channel == (info.noteId >> 7))
				{
					const auto msgout = midi2::makePolyController(
						info.MidiKeyNumber,
						midi2::PolySoundController5, // Brightness
						channelBrightness[header.channel]
					);

					sink({ msgout.m }, timestamp);
					//					_RPTN(0, "MPE: Brightness %d %f (%d)\n", info.MidiKeyNumber, channelBrightness[header.channel], msg[2]);
				}
			}
		}
		break;
		}
	}

	void setSink(std::function<void(const midi2::message_view, int)> psink)
	{
		sink = psink;
	}

	void setMpeMode(int32_t mpeOn)
	{
		switch (mpeOn)
		{
		case 1: // force Off
			lowerZoneMasterChannel = -1;
			upperZoneMasterChannel = -1;
			lower_zone_size = 0;
			upper_zone_size = 0;
			break;

		case 2: // force On (one zone)
			lowerZoneMasterChannel = 0;
			lower_zone_size = 15;
			upperZoneMasterChannel = -1;
			upper_zone_size = 0;
			break;

		default:
			// Auto - do nothing
			break;
		}
	}
};

#pragma warning( push )
#pragma warning( disable : 26495 ) // midi2NoteToKey' is uninitialized

// Convert MIDI 2.0 to MIDI 1.0
class MidiConverter1
{
	std::function<void(const midi2::message_view, int timestamp)> sink;
	float midi2NoteTune[256];
	uint8_t midi2NoteToKey[256];

	struct avoidRepeatedCCs
	{
		// out-of-range value to trigger initial update.
		float unquantized = -1.0f;
		uint8_t quantized = 255;
	};
	avoidRepeatedCCs ControlChangeValue[128];

public:
	MidiConverter1(std::function<void(const midi2::message_view, int)> psink) :
		sink(psink)
	{
		for (size_t i = 0; i < std::size(midi2NoteToKey); ++i)
		{
			midi2NoteToKey[i] = static_cast<uint8_t>(i);
			midi2NoteTune[i] = static_cast<float>(i);
		}
	}

	void setSink(std::function<void(const midi2::message_view, int)> psink)
	{
		sink = psink;
	}

	void processMidi(const midi2::message_view msg, int timestamp)
	{
		// MIDI 1.0 messages need no conversion
		if (!midi2::isMidi2Message(msg))
		{
			// no conversion.
			sink(msg, timestamp);
		}

		const auto header = midi2::decodeHeader(msg);

		// only 8-byte messages supported. only 16 channels supported
		if (header.messageType != midi2::ChannelVoice64 || header.channel > 15)
			return;

		switch (header.status)
		{
		case midi2::NoteOn:
		{
			const auto note = midi2::decodeNote(msg);
#if 0
			uint8_t keyNumber = note.noteNumber & 0x7f; // clamp to range [0 - 127] 

			// derive keynumber from pitch, since noteNumber may have no relation to pitch.
			if (midi2::attribute_type::Pitch == note.attributeType)
			{
			}
#endif
			const auto keyNumber = static_cast<uint8_t>(static_cast<int>(floorf(0.5f + midi2NoteTune[note.noteNumber])) & 0x7f);

			midi2NoteToKey[note.noteNumber] = keyNumber;

			uint8_t msgout[] = {
				static_cast<uint8_t>((midi1::status_type::NoteOn << 4) | header.channel),
				keyNumber,
				(std::max)((uint8_t)1, gmpi::midi_utils::floatToU7(note.velocity)) // note on w Vel zero is note OFF (don't allow)
			};

			sink({ msgout }, timestamp);
		}
		break;

		case midi2::NoteOff:
		{
			// TODO: !!! lookup nearest key number based on pitch, store it for note-off.
			const auto note = midi2::decodeNote(msg);

			uint8_t msgout[] = {
				static_cast<uint8_t>((midi1::status_type::NoteOff << 4) | header.channel),
				midi2NoteToKey[note.noteNumber],
				gmpi::midi_utils::floatToU7(note.velocity)
			};

			sink({ msgout }, timestamp);

			// reset pitch of note number.
			midi2NoteToKey[note.noteNumber] = note.noteNumber;
		}
		break;

		case midi2::ControlChange:
		{
			// avoid sending same value repeatedly
			const auto controller = midi2::decodeController(msg);
			if (controller.type < 128)
			{
				const auto newQuantizedValue = gmpi::midi_utils::floatToU7(controller.value);

				// 'thin' repeated 7-bit values. Unless it apears to be on purpose (e.g. all notes off)
				// Send exact (deliberate) repeats, but not 'close' (different but close unquantized) repeats
				if (ControlChangeValue[controller.type].quantized != newQuantizedValue || controller.value == ControlChangeValue[controller.type].unquantized)
				{
					ControlChangeValue[controller.type].quantized = newQuantizedValue;
					ControlChangeValue[controller.type].unquantized = controller.value;

					uint8_t msgout[] = {
						static_cast<uint8_t>((midi1::status_type::ControlChange << 4) | header.channel),
						controller.type,
						gmpi::midi_utils::floatToU7(controller.value)
					};

					sink({ msgout }, timestamp);
				}
			}
		}
		break;

		case midi2::PolyControlChange:
		{
			const auto polyController = midi2::decodePolyController(msg);

			if (polyController.type == midi2::PolyPitch)
			{
				const auto semitones = midi2::decodeNotePitch(msg);
				midi2NoteTune[polyController.noteNumber] = semitones;
			}
		}
		break;
		/*
					case midi2::PolyBender:
					{
						const auto bender = midi2::decodeController(midiMessage, size);
		//					_RPTN(0, "decodePolyBender %f\n", bender.value);
						const int32_t automation_id = (ControllerType::VoiceNoteExpression << 24) | ((ControllerType::kTuningTypeID & 0xFF) << 16) | bender.noteNumber;
						vst_Automation(voiceState->voiceControlContainer_, timestamp, automation_id, bender.value);
					}
					break;
		*/
		case midi2::PolyAfterTouch:
		{
			const auto aftertouch = midi2::decodePolyController(msg);
			//				_RPTN(0, "decodePolyPressure %d %f\n",aftertouch.noteNumber, aftertouch.value);

			uint8_t msgout[] = {
				static_cast<uint8_t>((midi1::status_type::PolyAfterTouch << 4) | header.channel),
				midi2NoteToKey[aftertouch.noteNumber],
				gmpi::midi_utils::floatToU7(aftertouch.value)
			};

			sink({ msgout }, timestamp);
		}
		break;

		case midi2::PitchBend:
		{
			const auto normalized = midi2::decodeController(msg).value;

			uint8_t msgout[] = {
				static_cast<uint8_t>((midi1::status_type::PitchBend << 4) | header.channel),
				0,
				0
			};

			gmpi::midi_utils::normalizedToBipoler14bit(normalized, msgout[1], msgout[2]);

			sink({ msgout }, timestamp);
		}
		break;

		case midi2::ChannelPressue:
		{
			const auto normalized = midi2::decodeController(msg).value;

			uint8_t msgout[] = {
				static_cast<uint8_t>((midi1::status_type::ChannelPressure << 4) | header.channel),
				gmpi::midi_utils::floatToU7(normalized)
			};

			sink({ msgout }, timestamp);
		}
		break;

		case midi2::ProgramChange:
		{
			if (msg[3] & 0x01) // option bit "Bank Select"
			{
				{
					uint8_t msgout[] = {
						static_cast<uint8_t>((midi1::status_type::ControlChange << 4) | header.channel),
						gmpi::midi::CC_Bank_msb,
						static_cast<uint8_t>(msg[6] & 0x7f),
					};
					sink({ msgout }, timestamp);
				}
				{
					uint8_t msgout[] = {
						static_cast<uint8_t>((midi1::status_type::ControlChange << 4) | header.channel),
						gmpi::midi::CC_Bank_lsb,
						static_cast<uint8_t>(msg[7] & 0x7f),
					};
					sink({ msgout }, timestamp);
				}
			}

			{
				uint8_t msgout[] = {
					static_cast<uint8_t>((midi1::status_type::ProgramChange << 4) | header.channel),
					static_cast<uint8_t>(msg[4] & 0x7f),
				};

				sink({ msgout }, timestamp);
			}
		}
		break;
		};
	}
};

#pragma warning( pop ) // midi2NoteToKey' is uninitialized

} // midi2
} // gmpi
