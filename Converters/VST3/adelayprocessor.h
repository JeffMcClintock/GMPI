#ifndef __adelayprocessor__
#define __adelayprocessor__

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "GmpiApiAudio.h"
#include "GmpiSdkCommon.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <array>
#include "mp_midi.h"
#include "se_types.h"
#include "lock_free_fifo.h"
#include "interThreadQue.h"
#include "xp_dynamic_linking.h"
//#include "RefCountMacros.h"

static const int MidiControllersParameterId = 10000;

enum class VoiceAllocationHint {Keyboard, MPE};

//namespace Steinberg {
//namespace Vst {

// to work arround Steinberg Interfaces having incompatible addRef etc
class GmpiBaseClass :public gmpi::api::IAudioPluginHost
{
public:
	GMPI_REFCOUNT_NO_DELETE;
};

template<int N>
class EventQue
{
	std::array<gmpi::api::Event, N> events;
	int tail = 0;
public:

	EventQue()
	{
		for (size_t i = 0 ; i < events.size() - 1; ++i)
		{
			events[i].next = &events[i + 1];
		}
	}

	void push(gmpi::api::Event event)
	{
		assert(tail != events.size()); // opps, filled event list up.

		if(tail < events.size() - 1)
			events[tail++] = event;
	}

	gmpi::api::Event* head()
	{
		if (tail == 0)
			return {};

		for (int i = 0; i < tail; ++i)
		{
			events[i].next = &events[i + 1];
		}

		events[tail - 1].next = {};
		return &events[0];
	}

	void clear()
	{
		tail = 0;
	}
};

//-----------------------------------------------------------------------------
class SeProcessor : public Steinberg::Vst::AudioEffect, public GmpiBaseClass //, public IShellServices, public IProcessorMessageQues
{
public:
	SeProcessor ();
	~SeProcessor ();
	
	Steinberg::tresult PLUGIN_API initialize (FUnknown* context) override;
	Steinberg::uint32 PLUGIN_API getLatencySamples() override;
	Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns, Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts) override;

	Steinberg::tresult PLUGIN_API setActive (Steinberg::TBool state) override;
	Steinberg::tresult PLUGIN_API process (Steinberg::Vst::ProcessData& data) override;

	Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) override;
	Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) override;

	Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage* message) override;

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

	// IAudioPluginHost
	gmpi::ReturnCode setPin(int32_t timestamp, int32_t pinId, int32_t size, const void* data) override;
	gmpi::ReturnCode setPinStreaming(int32_t timestamp, int32_t pinId, bool isStreaming) override;
	gmpi::ReturnCode setLatency(int32_t latency) override;
	gmpi::ReturnCode sleep() override;
	int32_t getBlockSize() override;
	int32_t getSampleRate() override;
	int32_t getHandle() override;

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
	gmpi::shared_ptr<gmpi::api::IAudioPlugin> plugin_;

	EventQue<1000> events;

	gmpi_dynamic_linking::DLL_HANDLE plugin_dllHandle = {};

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
	Steinberg::Vst::ProcessData* dataptr = {};

	std::vector<float> silence;

//	GMPI_QUERYINTERFACE(gmpi::api::IAudioPluginHost);
	gmpi::ReturnCode queryInterface(const gmpi::api::Guid* iid, void** returnInterface) override
	{
		*returnInterface = 0;
		if ((*iid) == gmpi::api::IAudioPluginHost::guid || (*iid) == gmpi::api::IUnknown::guid)
		{
			*returnInterface = static_cast<gmpi::api::IAudioPluginHost*>(this); GmpiBaseClass::addRef();
			return gmpi::ReturnCode::Ok;
		}
		return gmpi::ReturnCode::NoSupport;
	}
};

//}} // namespaces

#endif
