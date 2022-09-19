//-----------------------------------------------------------------------------
// Project     : VST SDK
// Version     : 3.5.2
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/adelay/source/adelayprocessor.cpp
// Created by  : Steinberg, 06/2009
// Description : 
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2012, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#ifndef __adelayprocessor__
#define __adelayprocessor__

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/vst/ivstevents.h"
//#include "SynthRuntime.h"
#include <thread>
#include <mutex>
#include <atomic>
#include "mp_midi.h"
#include "se_types.h"
#include "lock_free_fifo.h"
#include "interThreadQue.h"

static const int MidiControllersParameterId = 10000;

enum class VoiceAllocationHint {Keyboard, MPE};

namespace Steinberg {
namespace Vst {

//-----------------------------------------------------------------------------
class SeProcessor : public AudioEffect //, public IShellServices, public IProcessorMessageQues
{
public:
	SeProcessor ();
	~SeProcessor ();
	
	tresult PLUGIN_API initialize (FUnknown* context) override;
	uint32 PLUGIN_API getLatencySamples() override;
	tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts) override;

	tresult PLUGIN_API setActive (TBool state) override;
	tresult PLUGIN_API process (ProcessData& data) override;

	tresult PLUGIN_API setState (IBStream* state) override;
	tresult PLUGIN_API getState (IBStream* state) override;

	tresult PLUGIN_API notify(IMessage* message) override;

	static FUnknown* createInstance (void*) { return (IAudioProcessor*)new SeProcessor (); }

	void reInitialise();
#if 0 // TODO
	// IShellServices
	void onQueDataAvailable() override;
	void flushPendingParameterUpdates() override {}

	// IProcessorMessageQues
	IWriteableQue* MessageQueToGui() override
	{
		return &m_message_que_dsp_to_ui;
	}
	interThreadQue* ControllerToProcessorQue() override
	{
		return &m_message_que_ui_to_dsp;
	}
	void Service() override
	{
		// If any data waiting, either from ServiceWaiter or any other queue user, send it via VST3 binary message.
		if (m_message_que_dsp_to_ui.readyBytes())
		{
			onQueDataAvailable();
		}
	}
#endif

protected:
	void CommunicationProc();
	void DoNoteOff(int channel, int32_t noteId, float velocity, int sampleOffset);

	struct vstNoteInfo
	{
		int32_t noteId;		// VST3 note ID
		float pitch2;		// in MIDI semitones
		uint8_t channel;
		uint8_t MidiKeyNumber;
		bool held;
	};

	vstNoteInfo* findKey(uint8_t channel, int noteId);

	SeProcessor::vstNoteInfo& allocateKey(const Steinberg::Vst::NoteOnEvent& note);

//TODO	SynthRuntime synthEditProject;
	bool active_;
//TODO	my_VstTimeInfo timeInfo;

	std::vector<float*> inputBuffers;
	std::vector<float*> outputBuffers;
	bool outputsAsStereoPairs;

	// Background communication thread.
    std::thread background;
    std::mutex backgroundMutex;
    std::condition_variable backgroundSignal;
	std::atomic<bool> killBackgroundthread = {};

	// MIDI 2.0 allocation
	vstNoteInfo noteIds[16][128] = {};
	int noteIdsRoundRobin = -1;

    lock_free_fifo m_message_que_dsp_to_ui;
	interThreadQue m_message_que_ui_to_dsp;

	// MIDI output
	void MidiToHost(class MidiBuffer3* mb, timestamp_t SeStartClock, int numSamples);
	gmpi::midi_2_0::MidiConverter2 midiConverter;
	float midi2NoteTune[256];
	uint8_t midi2NoteToKey[256];
	struct avoidRepeatedCCs
	{
		float unquantized;
		uint8_t quantized;
	};
	avoidRepeatedCCs ControlChangeValue[128];
	ProcessData* dataptr = {};

	std::vector<float> silence;
};

}} // namespaces

#endif
