#include "adelayprocessor.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/base/ibstream.h"
//#include "midi_defs.h"
#include <algorithm>
#include <math.h>
#include "MyVstPluginFactory.h"
#include "unicode_conversion.h"
#include "mp_midi.h"
#include "Controller.h"
#include "mp_midi.h"
//#include "BundleInfo.h"
//#include "UMidiBuffer2.h"

using namespace JmUnicodeConversions;

#if defined( _WIN32)
extern HINSTANCE ghInst;
#endif


namespace Steinberg {
namespace Vst {

//-----------------------------------------------------------------------------
SeProcessor::SeProcessor ()
: active_(false)
, outputsAsStereoPairs(true)
,m_message_que_dsp_to_ui(0x500000)//AUDIO_MESSAGE_QUE_SIZE) //TODO
,m_message_que_ui_to_dsp(0x500000)//AUDIO_MESSAGE_QUE_SIZE)
, midiConverter(
	// provide a lambda to accept converted MIDI 2.0 messages
	[this](const gmpi::midi::message_view& msg, int offset)
	{
		const auto header = gmpi::midi_2_0::decodeHeader(msg);

		// only 8-byte messages supported. only 16 channels supported
		if (header.messageType != gmpi::midi_2_0::ChannelVoice64 || header.channel > 15)
			return;

		auto outputEvents = dataptr->outputEvents;

		// Prepare a VST3 event
		const uint16_t eventNotSet = 12345;
		Steinberg::Vst::Event event{};
		event.type = eventNotSet;
		event.sampleOffset = offset;

		// Parse MIDI 2.0 into a VST3 event
		switch (header.status)
		{
		case gmpi::midi_2_0::NoteOn:
		{
			const auto note = gmpi::midi_2_0::decodeNote(msg);
#if 0
			uint8_t keyNumber = note.noteNumber & 0x7f; // clamp to range [0 - 127] 

			// derive keynumber from pitch, since noteNumber may have no relation to pitch.
			if (gmpi::midi_2_0::attribute_type::Pitch == note.attributeType)
			{
			}
#endif
			const auto keyNumber = static_cast<uint8_t>(static_cast<int>(floorf(0.5f + midi2NoteTune[note.noteNumber])) & 0x7f);

			midi2NoteToKey[note.noteNumber] = keyNumber;

			event.type = Steinberg::Vst::Event::kNoteOnEvent;
			event.noteOn.noteId = keyNumber;
			event.noteOn.pitch = keyNumber;
			event.noteOn.velocity = note.velocity;
			event.noteOn.channel = header.channel & 0x0f;
		}
		break;

		case gmpi::midi_2_0::NoteOff:
		{
			// TODO: !!! lookup nearest key number based on pitch, store it for note-off.
			const auto note = gmpi::midi_2_0::decodeNote(msg);

			event.type = Steinberg::Vst::Event::kNoteOffEvent;
			event.noteOff.pitch = midi2NoteToKey[note.noteNumber];
			event.noteOff.noteId = midi2NoteToKey[note.noteNumber];
			event.noteOff.velocity = note.velocity;
			event.noteOff.channel = header.channel & 0x0f;

			// reset pitch of note number.
			midi2NoteToKey[note.noteNumber] = note.noteNumber;
		}
		break;

		case gmpi::midi_2_0::ControlChange:
		{
			const auto controller = gmpi::midi_2_0::decodeController(msg);
			if (controller.type < 128)
			{
				const auto newQuantizedValue = gmpi::midi::utils::floatToU7(controller.value);

				// 'thin' repeated 7-bit values. Unless it apears to be on purpose (e.g. all notes off)
				// Send exact (deliberate) repeats, but not 'close' (different but close unquantized) repeats
				if(ControlChangeValue[controller.type].quantized != newQuantizedValue || controller.value == ControlChangeValue[controller.type].unquantized)
				{
					ControlChangeValue[controller.type].quantized = newQuantizedValue;
					ControlChangeValue[controller.type].unquantized = controller.value;

					event.type = Steinberg::Vst::Event::kLegacyMIDICCOutEvent;
					event.midiCCOut.controlNumber = controller.type;
					event.midiCCOut.value = gmpi::midi::utils::floatToU7(controller.value);
					event.midiCCOut.channel = header.channel & 0x0f;
				}
			}
		}
		break;

		// track pitch changes
		case gmpi::midi_2_0::PolyControlChange:
		{
			const auto polyController = gmpi::midi_2_0::decodePolyController(msg);

			if (polyController.type == gmpi::midi_2_0::PolyPitch)
			{
				const auto semitones = gmpi::midi_2_0::decodeNotePitch(msg);
				midi2NoteTune[polyController.noteNumber] = semitones;
			}
		}
		break;

		case gmpi::midi_2_0::ChannelPressue:
		{
			const auto normalized = gmpi::midi_2_0::decodeController(msg).value;

			event.type = Steinberg::Vst::Event::kLegacyMIDICCOutEvent;
			event.midiCCOut.controlNumber = Steinberg::Vst::kAfterTouch;
			event.midiCCOut.value = gmpi::midi::utils::floatToU7(normalized);
			event.midiCCOut.channel = header.channel & 0x0f;
		}
		break;

		case gmpi::midi_2_0::PitchBend:
		{
			const auto normalized = gmpi::midi_2_0::decodeController(msg).value;

			event.type = Steinberg::Vst::Event::kLegacyMIDICCOutEvent;
			event.midiCCOut.controlNumber = Steinberg::Vst::kPitchBend;
			event.midiCCOut.channel = header.channel & 0x0f;
			gmpi::midi::utils::normalizedToBipoler14bit(
				normalized
				, (uint8_t&)event.midiCCOut.value
				, (uint8_t&)event.midiCCOut.value2
			);
		}
		break;

		case gmpi::midi_2_0::PolyAfterTouch:
		{
			const auto aftertouch = gmpi::midi_2_0::decodePolyController(msg);

#if 0
			// plain CC
			event.type = Steinberg::Vst::Event::kLegacyMIDICCOutEvent;
			event.midiCCOut.controlNumber = kCtrlPolyPressure;
			event.midiCCOut.value = aftertouch.noteNumber;
			event.midiCCOut.value2 = gmpi::midi::utils::floatToU7(aftertouch.value);
#else
			// VST3 poly pressure event
			event.type = Steinberg::Vst::Event::kPolyPressureEvent;
			event.polyPressure.pitch = aftertouch.noteNumber;
			event.polyPressure.noteId = aftertouch.noteNumber;
			event.polyPressure.pressure = aftertouch.value;
			event.polyPressure.channel = header.channel & 0x0f;
#endif
		}
		break;

		case gmpi::midi_2_0::System: // SYSTEM_EXCLUSIVE:
		{
#if 0 // TODO
			event.data.type = Steinberg::Vst::DataEvent::kMidiSysEx;

			// bit of a hack. the sysex data is technically 'freed' and *might* be overwritten at some future point when buffer wraps.
			event.data.bytes = midi_msg->data;
			event.data.size = midi_msg->size;
#endif
		}
		break;

		default:
			break;
		};

		// pass event to DAW
		if (event.type != eventNotSet)
		{
			outputEvents->addEvent(event);
		}
	}
)
{
	killBackgroundthread.store(false);
//TODO	memset( &timeInfo, 0, sizeof( timeInfo ) );

	for (int c = 0; c < std::size(noteIds); ++c)
	{
		for (int n = 0; n < std::size(noteIds[0]); ++n)
		{
			noteIds[c][n].MidiKeyNumber = n;
			noteIds[c][n].pitch2 = n;
			noteIds[c][n].channel = c;
		}
	}

	for (size_t i = 0; i < std::size(midi2NoteToKey); ++i)
	{
		midi2NoteToKey[i] = static_cast<uint8_t>(i);
		midi2NoteTune[i] = static_cast<float>(i);
	}

	for (auto& q : ControlChangeValue)
	{
		// out-of-range value to trigger initial update.
		q.quantized = 255;
		q.unquantized = -1.f;
	}

	//TODO	synthEditProject.connectPeer(this);
}

SeProcessor::~SeProcessor ()
{
	if (background.joinable())
	{
		{
			std::unique_lock<std::mutex> lk(backgroundMutex);
			killBackgroundthread.store(true);
		}
		backgroundSignal.notify_one();

		background.join();
	}
}

void SeProcessor::reInitialise()
{
#if 0 // TODO
	synthEditProject.prepareToPlay(
		this,
		processSetup.sampleRate,
		processSetup.maxSamplesPerBlock,
		processSetup.processMode != kOffline
	);
#endif

	silence.assign(processSetup.maxSamplesPerBlock, 0.0f);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API SeProcessor::initialize (FUnknown* context)
{
	auto factory = MyVstPluginFactory::GetInstance();
	outputsAsStereoPairs = factory->GetOutputsAsStereoPairs();

	//const auto& info = BundleInfo::instance()->getPluginInfo();
	//supportedChannels = info.support16MidiChannels ? 16 : 1;

	reInitialise();

	tresult result = AudioEffect::initialize (context);

	if (result == kResultTrue)
	{
//		int numInputs = synthEditProject.getNumInputs();
int numInputs = 2;
		inputBuffers.assign(numInputs, nullptr);

		if( outputsAsStereoPairs )
		{
			while( numInputs > 1 )
			{
				addAudioInput (STR16 ("AudioInput"), SpeakerArr::kStereo);
				numInputs -= 2;
			}
		}

		while( numInputs > 0 )
		{
			addAudioInput( STR16( "AudioInput" ), SpeakerArr::kMono );
			numInputs -= 1;
		}

		//int numOutputs = synthEditProject.getNumOutputs();
int numOutputs = 2;
		outputBuffers.assign(numOutputs, nullptr);
		int pinIndex = 0;
		if( outputsAsStereoPairs )
		{
			while( numOutputs > 1 )
			{
#ifdef MAC
                auto name = JmUnicodeConversions::ToUtf16( factory->GetOutputsName(pinIndex) );
#else
				auto name = factory->GetOutputsName(pinIndex);
#endif
				addAudioOutput(name.c_str(), SpeakerArr::kStereo);
				numOutputs -= 2;
				pinIndex += 2;
			}
		}
		while( numOutputs > 0 )
		{
#ifdef MAC
            auto name = JmUnicodeConversions::ToUtf16( factory->GetOutputsName(pinIndex) );
#else
            auto name = factory->GetOutputsName(pinIndex);
#endif
			addAudioOutput(name.c_str(), SpeakerArr::kMono);
			numOutputs -= 1;
			pinIndex += 1;
		}

#if 0 // TODO
		if(synthEditProject.wantsMidi())
		{
			addEventInput(STR16 ("Event Input"), 16);
		}

		if (synthEditProject.sendsMidi())
		{
			addEventOutput(STR16("Event Output"), 16);
		}
#endif
	}

	factory->release();

	if (!background.joinable()) // protect against double initialize (Steinberg validator)
	{
		background = std::thread(
			[this]
			{
				CommunicationProc();
			}
			);
	}
	return result;
	return true;
}

uint32 SeProcessor::getLatencySamples()
{
	// see also kLatencyChanged and componentHandler->restartComponent (Vst::kLatencyChanged & Vst::kParamValuesChanged);
	// _RPT1(_CRT_WARN, "SeProcessor::getLatencySamples() -> %d\n", synthEditProject.getLatencySamples());

	return 0;// synthEditProject.getLatencySamples();
}

//-----------------------------------------------------------------------------
// Not called?
tresult PLUGIN_API SeProcessor::setBusArrangements (SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts)
{
	// we only support one in and output bus and these buses must have the same number of channels
	if (numIns == 1 && numOuts == 1 && inputs[0] == outputs[0])
		return AudioEffect::setBusArrangements (inputs, numIns, outputs, numOuts);

	return kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API SeProcessor::setActive (TBool state)
{
/* Seems to be checking number of output channels. SE does not require any particular number.
	SpeakerArrangement arr;
	if (getBusArrangement (kOutput, 0, arr) != kResultTrue)
		return kResultFalse;
	int32 numChannels = SpeakerArr::getChannelCount (arr);
	if (numChannels == 0)
		return kResultFalse;
*/

//	_RPT1( _CRT_WARN, "SeProcessor::setActive (%d)n", state );

	if (state)
	{
		reInitialise();
	}

	active_ = state != 0;

	return AudioEffect::setActive (state);
}

#if 0
// VST3 just uses successive incrementing id for notes, this maps those values to a key.
// by default we use the natural key for that pitch, else we find a spare key.
int SeProcessor::allocateKey(int noteId, int pitchSemiTone)
{
	const int naturalKey = 0x7f & pitchSemiTone;

	int key = naturalKey;

	if (heldNotes[naturalKey])
	{
		for (int i = 0; i < 128; ++i)
		{
			key = (key + 1) & 0x7f;

			if (!heldNotes[key])
				break;
		}
	}
	
	heldNotes[key] = true;
	noteIds[key] = noteId;

	return key;
}
#endif
SeProcessor::vstNoteInfo* SeProcessor::findKey(uint8_t channel, int noteId)
{
	for (auto& keyInfo : noteIds[channel])
	{
		if (keyInfo.noteId == noteId)
		{
			return &keyInfo;
		}
	}

	return {};
}

SeProcessor::vstNoteInfo& SeProcessor::allocateKey(const NoteOnEvent& note)
{
	auto& ret = noteIds[note.channel][note.pitch];

	ret.held = true;

	if(note.noteId < 0)
		ret.noteId = note.pitch;
	else
		ret.noteId = note.noteId;

	return ret;

#if 0
	// TODO!! use allocation mode here base on host-control HC_MPE_MODE:
	// 1. assume source is a standard keyboard, and that two different noteIds with same pitch means same key/voice (to enable soft/hard steal to work)
	// 2. MPE assume two different noteIds with same pitch are independent voices. and only same noteID means same key/voice.
	const auto hint = VoiceAllocationHint::Keyboard;

	if (e.noteOn.noteId == -1) // note id increments indefinitely each note.
		e.noteOn.noteId = e.noteOn.pitch;


	// prefer to allocate voice as per MIDI 1.0 (semitone = key)
	if (VoiceAllocationHint::Keyboard == hint)
	{
		return noteIds[semitone];
	}
	// prefer to allocate voice with same noteId.
	for (int i = 0; i < 128; ++i)
	{
		if (noteIds[i].noteId == noteId)
		{
			return noteIds[i];
		}
	}

	// allocate in a round robin fashion.
	for (int i = 0; i < 128; ++i)
	{
		noteIdsRoundRobin = (noteIdsRoundRobin + 1) & 0x7f;

		if (!noteIds[noteIdsRoundRobin].held)
		{
			break;
		}
	}

	return noteIds[noteIdsRoundRobin];
#endif
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API SeProcessor::process (ProcessData& data)
{
#if 0
	if (synthEditProject.reinitializeFlag)
	{
		reInitialise();
	}

	if (data.inputParameterChanges)
	{
		int32 paramChangeCount = data.inputParameterChanges->getParameterCount ();
		for (int32 index = 0; index < paramChangeCount; index++)
		{
			IParamValueQueue* queue = data.inputParameterChanges->getParameterData (index);
			if (queue && queue->getParameterId () >= 0 )
			{
				int32 valueChangeCount = queue->getPointCount ();
				ParamValue value;
				int32 sampleOffset;

				if (queue->getPoint (valueChangeCount-1, sampleOffset, value) == kResultTrue)
				{
					int id = queue->getParameterId();

					if( id < MidiControllersParameterId)
					{
						//_RPTW1( _CRT_WARN, L"Processor : %f\n", value );
						assert(sampleOffset >=0 && sampleOffset < data.numSamples);
//						_RPT1(_CRT_WARN, "                     PRESETS-DSP: P%d Set in Process()\n", id);

						synthEditProject.setParameterNormalizedDsp( sampleOffset, id, value );
					}
					else
					{
						// TODO: CC74 (Brightness) and 128 (Channel Pressure) on all channels
						id = id - MidiControllersParameterId;
						if ( id >=0 )
						{
							constexpr int ccsPerChannel = 130;
							const auto channel = id / ccsPerChannel;
							const auto cc = id % ccsPerChannel;

							gmpi::midi_2_0::rawMessage64 msgout;

							if(cc < 128 )
							{
								msgout = gmpi::midi_2_0::makeController(
									static_cast<uint8_t>(cc)
									, value
									, channel
								);
							}
							else
							{
								switch(cc)
								{
									case 128: // Channel Pressure.
									{
										msgout = gmpi::midi_2_0::makeChannelPressure(
											value
											, channel
										);
									}
									break;

									case 129: // Bender.
									{
										_RPTN(0, "C%2d BDR %f\n", channel, value);
										msgout = gmpi::midi_2_0::makeBender(
											value // use normalized bender
											, channel
										);
									}
									break;
								}
							}

							synthEditProject.MidiIn(
								sampleOffset
								, msgout.m
								, sizeof(msgout.m)
							);
						}
					}
				}
			}
		}
	}

	if (data.numSamples <= 0)
		return kResultTrue;

	const int32 numEvents = data.inputEvents ? data.inputEvents->getEventCount() : 0;
	
	for (int32 eventIndex = 0; eventIndex < numEvents ; ++eventIndex)
	{
		Event e;
		if (data.inputEvents->getEvent(eventIndex, e) != kResultTrue)
		{
			break;
		}

		/* Steinberg seem to use smaller sub-blocks
		// if the event is not in the current processing block then adapt offset for next block
		if (e.sampleOffset > samplesToProcess)
		{
			e.sampleOffset -= samplesToProcess;
			break;
		}
		*/
		switch (e.type)
		{
			case Event::kNoteOnEvent:
			{
				// handle hosts that don't pass a velocity=0 note-on as a note-off
				if (e.noteOn.velocity == 0.0f)
				{
					if (e.noteOn.noteId == -1)
						e.noteOn.noteId = e.noteOn.pitch;

					DoNoteOff(e.noteOn.channel, e.noteOn.noteId, e.noteOn.velocity, e.sampleOffset);
				}
				else
				{
					auto& keyInfo = allocateKey(e.noteOn);

					// In the case a non natural key was allocated, send pitch information also. (else SE will play wrong pitch)
					const float pitch = e.noteOn.pitch + 0.01f * e.noteOn.tuning; // microtunable pitch
					if (keyInfo.pitch2 != pitch)
					{
						keyInfo.pitch2 = pitch;

						const auto out = gmpi::midi_2_0::makeNotePitchMessage(
							keyInfo.MidiKeyNumber,
							keyInfo.pitch2,
							keyInfo.channel
						);

						synthEditProject.MidiIn(e.sampleOffset, (const unsigned char*)&out, sizeof(out));
					}

					// TODO : DON"T SEND UNLESS WE HAVE TO (MAYBE RESET THEM AUTOMATICALLY IN PM on note on
					// reset note expression controllers.
					{
						for (auto controllerId : { gmpi::midi_2_0::PolyVolume, gmpi::midi_2_0::PolyPan, gmpi::midi_2_0::PolySoundController5 })
						{
							const auto msg = gmpi::midi_2_0::makePolyController(
								keyInfo.MidiKeyNumber,
								controllerId,
								0.0f,
								keyInfo.channel
							);

							synthEditProject.MidiIn(e.sampleOffset, (const unsigned char*)&msg, sizeof(msg));
						}
						// per-note bender
						{
							const auto msg = gmpi::midi_2_0::makePolyBender(
								keyInfo.MidiKeyNumber,
								0.5f,
								keyInfo.channel
							);

							synthEditProject.MidiIn(e.sampleOffset, (const unsigned char*)&msg, sizeof(msg));
						}
					}

					{
						_RPTN(0, "C%2d NON %3d\n", e.noteOn.channel, e.noteOn.pitch);

						const auto out = gmpi::midi_2_0::makeNoteOnMessage(
							keyInfo.MidiKeyNumber,
							e.noteOn.velocity,
							keyInfo.channel
						);

						synthEditProject.MidiIn(e.sampleOffset, (const unsigned char*)&out, sizeof(out));
					}
				}
			}
			break;

			case Event::kNoteOffEvent:
			{
				if (e.noteOff.noteId == -1)
					e.noteOff.noteId = e.noteOff.pitch;

				DoNoteOff(e.noteOff.channel, e.noteOff.noteId, e.noteOff.velocity, e.sampleOffset);
			}
			break;

			case Event::kPolyPressureEvent:
			{
				// noteid is always -1, and pitch 0
				if (e.polyPressure.noteId == -1)
					e.polyPressure.noteId = e.polyPressure.pitch;

				if (auto keyInfo = findKey(e.polyPressure.channel, e.polyPressure.noteId); keyInfo)
				{
					const auto out = gmpi::midi_2_0::makePolyPressure(
						keyInfo->MidiKeyNumber,
						e.polyPressure.pressure,
						keyInfo->channel
					);
//						_RPTN(0, "makePolyPressure %d %f\n", keyInfo->MidiKeyNumber, e.polyPressure.pressure);

					synthEditProject.MidiIn(
						e.sampleOffset,
						(const unsigned char*)&out,
						sizeof(out)
					);
				}
			}
			break;				

			case Event::kNoteExpressionValueEvent:
			{
				const int channel = 0; // ??? how is this sposed to find notes on other channels?
				auto keyInfo = findKey(channel, e.noteExpressionValue.noteId);
					
				if (!keyInfo)
					break;

				if (kTuningTypeID == e.noteExpressionValue.typeId)
				{				
					const double pitchBendRangeExpr = 120.0; // +/- 10 octaves
					constexpr double pitchBendRangeMidi = 24.0; // assumed
					constexpr double pitchBendRangeMidiInv = 1.0 / pitchBendRangeMidi;

					const double semitones = (e.noteExpressionValue.value - 0.5) * 2.0 * pitchBendRangeExpr;
					const double normalized = (std::max)(0.0, std::min(1.0, 0.5 + 0.5 * semitones * pitchBendRangeMidiInv));

//						_RPTN(0, "makePolyBender %f => %f semitones, %f normal\n", e.noteExpressionValue.value, semitones, normalized);

					// Send MIDI HD-Protocol Note Expression message.
					const auto msg = gmpi::midi_2_0::makePolyBender(
						keyInfo->MidiKeyNumber,
						static_cast<float>(normalized),
						channel
					);

					synthEditProject.MidiIn(e.sampleOffset, (const unsigned char*)&msg, sizeof(msg));
				}
				else
				{
					// PolyModulation and PolyBreath not accounted for, probley gonna be pretty common.

					// Volume, Pan, Tuning is all Cubase supports by default. (plus poly pressure)
					// probly need brightness too for MPE

					int controllerId = -1;
					switch (e.noteExpressionValue.typeId)
					{
					case kVolumeTypeID:
						controllerId = gmpi::midi_2_0::PolyVolume;
						break;
					case kPanTypeID:
						controllerId = gmpi::midi_2_0::PolyPan;
						break;
/*
					case kVibratoTypeID:
						controllerId = gmpi::midi_2_0::PolySoundController8; // Vibrato Depth
						break;
					case kExpressionTypeID:
						controllerId = gmpi::midi_2_0::PolyExpression;
						break;
*/
					case kBrightnessTypeID:
						controllerId = gmpi::midi_2_0::PolySoundController5; // Brightness. MPE Vertical (Y)
						break;

					default:
						break;
					}

					_RPTN(0, "EXPR: key %3d type %2d value %f\n", keyInfo->MidiKeyNumber, e.noteExpressionValue.typeId, e.noteExpressionValue.value);

					if (controllerId >= 0)
					{
						// Bitwig sends brightness as bipolar (-0.5 -> +0.5), Cubase as normalized (0 -> 1.0).
						// Not sure which is right.
						const auto safeValue = (std::min)(1.0, (std::max)(0.0, e.noteExpressionValue.value));

						// Send MIDI HD-Protocol Note Expression message.
						const auto msg = gmpi::midi_2_0::makePolyController(
							keyInfo->MidiKeyNumber,
							controllerId,
							safeValue
						);

						synthEditProject.MidiIn(e.sampleOffset, (const unsigned char*)&msg, sizeof(msg));
					}
				}
			}
			break;

			case Event::kDataEvent:
			{
				if (e.data.type == DataEvent::kMidiSysEx)
				{
					bool isFirst = true;
					int remain = e.data.size - 2; // skip leading F0 and trailing F7
					const uint8_t* src = e.data.bytes + 1; // skip leading F0
					while (remain > 0)
					{
						const auto msg = gmpi::midi_2_0::makeSysex(src, remain, isFirst);
						synthEditProject.MidiIn(e.sampleOffset, (const unsigned char*)&msg, sizeof(msg));
					}
				}
			}
			break;
		}
	}

	// Tempo
	if( synthEditProject.NeedsTempo( ) )
	{
		if (data.processContext)
		{
			auto& vst3Time = *data.processContext;

			timeInfo.timeSigNumerator = vst3Time.timeSigNumerator;
			timeInfo.timeSigDenominator = vst3Time.timeSigDenominator;
			timeInfo.ppqPos = vst3Time.projectTimeMusic;
			timeInfo.barStartPos = vst3Time.barPositionMusic;
			timeInfo.sampleRate = processSetup.sampleRate;
			timeInfo.tempo = vst3Time.tempo;
			timeInfo.cycleStartPos = vst3Time.cycleStartMusic;
			timeInfo.cycleEndPos = vst3Time.cycleEndMusic;

			timeInfo.flags = 0;

			if ((vst3Time.state & ProcessContext::kTimeSigValid) != 0)
			{
				timeInfo.flags |= my_VstTimeInfo::kVstTimeSigValid;
			}
			if ((vst3Time.state & ProcessContext::kBarPositionValid) != 0)
			{
				timeInfo.flags |= my_VstTimeInfo::kVstBarsValid;
			}
			if ((vst3Time.state & ProcessContext::kTempoValid) != 0)
			{
				timeInfo.flags |= my_VstTimeInfo::kVstTempoValid;
			}
			if ((vst3Time.state & ProcessContext::kProjectTimeMusicValid) != 0)
			{
				timeInfo.flags |= my_VstTimeInfo::kVstPpqPosValid;
			}
			if ((vst3Time.state & ProcessContext::kCycleValid) != 0)
			{
				timeInfo.flags |= my_VstTimeInfo::kVstCyclePosValid;
			}
			if ((vst3Time.state & ProcessContext::kCycleActive) != 0)
			{
				timeInfo.flags |= my_VstTimeInfo::kVstTransportCycleActive;
			}
			if (((vst3Time.state & ProcessContext::kPlaying) == 0) != ((timeInfo.flags & my_VstTimeInfo::kVstTransportPlaying) == 0))
			{
				timeInfo.flags |= my_VstTimeInfo::kVstTransportChanged;
			}
			if ((vst3Time.state & ProcessContext::kPlaying) != 0)
			{
				timeInfo.flags |= my_VstTimeInfo::kVstTransportPlaying;
			}
#if 0
			if ((vst3Time.state & ProcessContext::kSmpteValid) != 0)
			{
				timeInfo.flags |= my_VstTimeInfo::kVstSmpteValid;
			}
			if ((vst3Time.state & ProcessContext::kClockValid) != 0)
			{
				timeInfo.flags |= my_VstTimeInfo::kVstClockValid;
			}
#endif
			synthEditProject.UpdateTempo(&timeInfo);
		}
	}

	// Setup audio buffers.
	int numChannelsIn = 0;
	int n = 0;
	for (int bus = 0; bus < data.numInputs; ++bus)
	{
		auto silenceFlags = data.inputs[bus].silenceFlags;
		numChannelsIn += data.inputs[bus].numChannels;
		const auto channels = (std::min)((size_t) data.inputs[bus].numChannels, inputBuffers.size());

		for (int channel = 0; channel < channels; ++channel)
		{
			const bool isSilent = 0 != (silenceFlags & 1);

			synthEditProject.setInputSilent(n, isSilent);

			if (isSilent)
			{
				// Cubase sends non-silent buffers with silent flag set. In this case. substitute a actually silent buffer.
				inputBuffers[n] = silence.data();
			}
			else
			{
	//			assert(0 == (silenceFlags & 1) || *inputBuffers[n] == 0.0f); // check 'silent' buffers are zeroed
				inputBuffers[n] = data.inputs[bus].channelBuffers32[channel];
			}

			++n;
			silenceFlags = silenceFlags >> 1;
		}
	}

	int numChannelsOut = 0;
	n = 0;
	for (int bus = 0; bus < data.numOutputs; ++bus)
	{
		numChannelsOut += data.outputs[bus].numChannels;
		for (int channel = 0; channel < data.outputs[bus].numChannels && n < outputBuffers.size(); ++channel)
		{
			outputBuffers[n++] = data.outputs[bus].channelBuffers32[channel];
		}
	}

	timestamp_t SeStartClock = synthEditProject.getSampleClock();

	synthEditProject.process(data.numSamples, (const float**) inputBuffers.data(), outputBuffers.data(), numChannelsIn, numChannelsOut);

	// silence flags
	n = 0;
	for (int bus = 0; bus < data.numOutputs; ++bus)
	{
		const auto channels = (std::min)((size_t)data.outputs[bus].numChannels, outputBuffers.size());
		data.outputs[bus].silenceFlags = synthEditProject.updateSilenceFlags(n, channels);
	}

	// MIDI output.
	auto mb = synthEditProject.getMidiOutputBuffer();

#if 0 // send MIDI CCs as parameter changes
	IParameterChanges* paramChanges = data.outputParameterChanges;
	if (!mb->IsEmpty() && paramChanges)
	{
		IParamValueQueue* paramQueue[128];
		memset(paramQueue, 0, sizeof(paramQueue));

		while (!mb->IsEmpty())
		{
			int due_time_delta = (int)(mb->PeekNextTimestamp() - SeStartClock);
			assert(due_time_delta >= 0);

			// not due till next block?
			if (due_time_delta >= data.numSamples)
			{
				break;
			}
			due_time_delta = (std::max)(due_time_delta, 0); // should never be nesc, but is safer to do

			auto midi_msg = mb->Current();

			// Handle MIDI messages.
			{
				unsigned char* midiData = (unsigned char*)midi_msg->data;

				// normal short MIDI message?
				int b1 = midiData[0];
				if (b1 == CONTROL_CHANGE)
				{
					int controllerNumber = midiData[1];
					Steinberg::Vst::ParamID ParameterId = MidiControllersParameterId + controllerNumber;

					Steinberg::int32 returnIndex; // not used.
					if (paramQueue[controllerNumber] == 0)
					{
						paramQueue[controllerNumber] = paramChanges->addParameterData(ParameterId, returnIndex);
					}

					if (paramQueue)
					{
						const double byteToNormal = 1.0 / 127.0;
						double v = midiData[2] * byteToNormal;
						paramQueue[controllerNumber]->addPoint(due_time_delta, v, returnIndex);
					}
				}
			}

			mb->UpdateReadPos();
		}
	}
#else
	// Send MIDI CC as legacy events.
	dataptr = &data; // hacky back-channel
	MidiToHost(mb, SeStartClock, data.numSamples);
	dataptr = {};
#endif
#endif
	return kResultTrue;
}

void SeProcessor::DoNoteOff(int channel, int32_t noteId, float velocity, int sampleOffset)
{
	if (auto keyInfo = findKey(channel, noteId); keyInfo)
	{
		keyInfo->held = false;

		const auto out = gmpi::midi_2_0::makeNoteOffMessage(
			keyInfo->MidiKeyNumber,
			velocity,
			keyInfo->channel
		);

//		synthEditProject.MidiIn(sampleOffset, (const unsigned char*)&out, sizeof(out));
	}
}

tresult SeProcessor::setState (IBStream* state)
{
//	_RPT1(_CRT_WARN, "SeProcessor::setState (T%x)\n", GetCurrentThreadId());

	int32 bytesRead;
	int32 chunkSize = 0;
	state->read( &chunkSize, sizeof(chunkSize), &bytesRead );

	char* s = new char[chunkSize];
	state->read( s, chunkSize, &bytesRead );
	std::string chunk(s, bytesRead);
	delete [] s;

//	synthEditProject.setPresetStateFromUiThread( chunk, active_ );

	return kResultTrue;
}

// Seems to be called before audio starts to set GUI up correctly.
tresult SeProcessor::getState (IBStream* state)
{
	std::string chunk;
//	synthEditProject.getPresetState( chunk, active_ );

	int32 chunkSize = (int32) chunk.size();
	int32 bytesWritten;

	state->write( &chunkSize, sizeof(chunkSize), &bytesWritten );
	state->write( (void*) chunk.data(), chunkSize, &bytesWritten );

	return kResultTrue;
}

tresult PLUGIN_API SeProcessor::notify( IMessage* message )
{
	// WARNING: CALLED FROM GUI THREAD (In Ableton Live).
	if( !message )
		return kInvalidArgument;

	if( !strcmp( message->getMessageID(), "BinaryMessage" ) )
	{
		const void* data;
		uint32 size;
		if( message->getAttributes()->getBinary( "MyData", data, size ) == kResultOk )
		{
			//auto que = synthEditProject.ControllerToProcessorQue();

			m_message_que_ui_to_dsp.pushString(size, (unsigned char*) data);
			m_message_que_ui_to_dsp.Send();

			return kResultOk;
		}
	}

	return AudioEffect::notify( message );
}

#if 0
void SeProcessor::onQueDataAvailable()
{
	// TODO!!! This is INCORRECT. The que write pointer must be updated under the lock, else waiting thread can miss it.
	std::unique_lock<std::mutex> lk(backgroundMutex);
	backgroundSignal.notify_one();
}
#endif
// background thread
void SeProcessor::CommunicationProc()
{
//	auto que = synthEditProject.MessageQueToGuiRaw();

	while(!killBackgroundthread)
	{
		auto readyBytes = m_message_que_dsp_to_ui.readyBytes();
		while(readyBytes > 0) // handle case of fifo wrap-arround
		{
			IMessage* message = allocateMessage();
			if(!message)
			{
				break;
			}

			Steinberg::FReleaser msgReleaser(message);
			message->setMessageID("BinaryMessage");

			void* data{};
			const auto size = m_message_que_dsp_to_ui.siphon2(&data);
			assert(size > 0);

			message->getAttributes()->setBinary("MyData", data, size);
			sendMessage(message);

			m_message_que_dsp_to_ui.siphon2_advance(size);

			readyBytes -= size;
		}
		// wait to be signalled.
		{
			std::unique_lock<std::mutex> lk(backgroundMutex);
			backgroundSignal.wait(lk);
		}
	}
}

void SeProcessor::MidiToHost(MidiBuffer3* mb, timestamp_t SeStartClock, int numSamples)
{
#if 0
	while (!mb->IsEmpty())
	{
		auto due_time_delta = static_cast<int64_t>(mb->PeekNextTimestamp()) - static_cast<int64_t>(SeStartClock);
		assert(due_time_delta >= 0);
		due_time_delta = (std::max)(due_time_delta, int64_t(0)); // should never be nesc, but is safer to do

		// not due till next block?
		if (due_time_delta >= numSamples)
		{
			break;
		}

		const auto midi_msg = mb->Current();

		// Handle MIDI messages.
		midiConverter.processMidi({ midi_msg->data, midi_msg->size }, due_time_delta);
#if 0
//		const auto midiData = midi_msg->data;
		Steinberg::Vst::Event event{};
		event.sampleOffset = due_time_delta;
		event.midiCCOut.channel = midiData[0] & 0x0f;

		// normal short MIDI message?
		auto status = midiData[0] & 0xf0;

		// Note offs can be note_on vel=0
		if (status == NOTE_ON && midiData[2] == 0)
		{
			status = NOTE_OFF;
		}

		constexpr float recip_127 = 1.0f / 127.0f;

		switch (status)
		{
		case NOTE_ON:
		{
			event.type = Steinberg::Vst::Event::kNoteOnEvent;
			event.noteOn.noteId = midiData[1];
			event.noteOn.pitch = midiData[1];
			event.noteOn.velocity = recip_127 * (float)midiData[2];

			outputEvents->addEvent(event);
		}
		break;

		case NOTE_OFF:
		{
			event.type = Steinberg::Vst::Event::kNoteOffEvent;
			event.noteOff.pitch = midiData[1];
			event.noteOff.noteId = midiData[1];
			event.noteOff.velocity = recip_127 * (float)midiData[2];

			outputEvents->addEvent(event);
		}
		break;

		case CONTROL_CHANGE:
		{
			event.type = Steinberg::Vst::Event::kLegacyMIDICCOutEvent;
			event.midiCCOut.controlNumber = midiData[1];
			event.midiCCOut.value = midiData[2];

			outputEvents->addEvent(event);
		}
		break;

		case PITCHBEND:
		{
			event.type = Steinberg::Vst::Event::kLegacyMIDICCOutEvent;
			event.midiCCOut.controlNumber = Steinberg::Vst::kPitchBend;
			event.midiCCOut.value = midiData[2];
			event.midiCCOut.value2 = midiData[3];

			outputEvents->addEvent(event);
		}
		break;

		case CHANNEL_PRESSURE:
		{
			event.type = Steinberg::Vst::Event::kLegacyMIDICCOutEvent;
			event.midiCCOut.controlNumber = Steinberg::Vst::kAfterTouch;
			event.midiCCOut.value = midiData[2];

			outputEvents->addEvent(event);
		}
		break;

		case POLY_AFTERTOUCH:
		{
#if 0
			// plain CC
			event.type = Steinberg::Vst::Event::kLegacyMIDICCOutEvent;
			event.midiCCOut.controlNumber = kCtrlPolyPressure;
			event.midiCCOut.value = midiData[2];
			event.midiCCOut.value2 = midiData[3];
#else
			// VST3 poly pressure event
			event.type = Steinberg::Vst::Event::kPolyPressureEvent;
			event.polyPressure.pitch = midiData[1];
			event.polyPressure.noteId = midiData[1];
			event.polyPressure.pressure = recip_127 * (float)midiData[2];
#endif
			outputEvents->addEvent(event);
		}
		break;

		case SYSTEM_EXCLUSIVE:
		{
			event.data.type = Steinberg::Vst::DataEvent::kMidiSysEx;

			// bit of a hack. the sysex data is technically 'freed' and *might* be overwritten at some future point when buffer wraps.
			event.data.bytes = midi_msg->data;
			event.data.size = midi_msg->size;

			outputEvents->addEvent(event);
		}
		break;

		default:
			break;
		}
#endif

		mb->UpdateReadPos();
	}
#endif
}

}} // namespaces
