/* Copyright (c) 2007-2021 SynthEdit Ltd
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name SEM, nor SynthEdit, nor 'Music Plugin Interface' nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY SynthEdit Ltd ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL SynthEdit Ltd BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

#include <map>
#include <vector>
#include <algorithm>
#include "mp_interface_wrapper.h"

#include <assert.h>
#include "mp_sdk_common.h"
namespace gmpi
{
	// Timestamped events.
	enum MpEventType { EVENT_PIN_SET = 100, EVENT_PIN_STREAMING_START, EVENT_PIN_STREAMING_STOP, EVENT_MIDI, EVENT_GRAPH_START };

	struct MpEvent
	{
		int32_t timeDelta;	// Relative to block.
		int32_t eventType;	// See MpEventType enumeration.
		int32_t parm1;		// Pin index if needed.
		int32_t parm2;		// Sizeof additional data. >4 implies extraData is a pointer to the value.
		int32_t parm3;		// Pin value (if 4 bytes or less).
		int32_t parm4;		// Voice ID.
		char* extraData;	// Pointer to additional data which is too large to fit.
		MpEvent* next;		// Next event in list.
	};

	// Music plugin audio processing interface.
	class IMpAudioPlugin : public IMpUnknown
	{
	public:
		// Establish connection to host.
		virtual int32_t setHost(IMpUnknown* host) = 0;

		// Processing about to start.  Allocate resources here.
		virtual int32_t open() = 0;

		// Notify plugin of audio buffer address, one pin at a time. Address may change between process() calls.
		virtual int32_t setBuffer(int32_t pinId, float* buffer) = 0;

		// Process a time slice. No return code, must always succeed.
		virtual void process(int32_t count, const MpEvent* events) = 0;
	};

	// GUID for IMpAudioPlugin
	// {23835D7E-DCEB-4B08-A9E7-B43F8465939E}
	static const MpGuid MP_IID_AUDIO_PLUGIN = // MP_IID_PLUGIN2 =
	{ 0x23835d7e, 0xdceb, 0x4b08, { 0xa9, 0xe7, 0xb4, 0x3f, 0x84, 0x65, 0x93, 0x9e } };


	class IMpAudioPluginHost : public IMpUnknown
	{
	public:
		// Plugin sending out control data.
		virtual int32_t setPin(int32_t blockRelativeTimestamp, int32_t pinId, int32_t size, const void* data) = 0;

		// Plugin audio output start/stop (silence detection).
		virtual int32_t setPinStreaming(int32_t blockRelativeTimestamp, int32_t pinId, bool isStreaming) = 0;

		// PDC (Plugin Delay Compensation) support.
		virtual int32_t setLatency(int32_t latency) = 0;

		// Plugin indicates no processing needed until input state changes.
		virtual int32_t sleep() = 0;

		// Query audio buffer size.
		virtual int32_t getBlockSize() = 0;

		// Query sample-rate.
		virtual float getSampleRate() = 0;

		// Each plugin instance has a host-assigned unique handle shared by UI and Audio class.
		virtual int32_t getHandle() = 0;
	};

	// GUID for IMpAudioPluginHost.
	// {87CCD426-71D7-414E-A9A6-5ADCA81C7420}
	static const MpGuid MP_AUDIO_PLUGIN_HOST = // !!! should this be MP_IID_AUDIO_PLUGIN_HOST ???!!! (and others)
	{ 0x87ccd426, 0x71d7, 0x414e, { 0xa9, 0xa6, 0x5a, 0xdc, 0xa8, 0x1c, 0x74, 0x20 } };
}

namespace GmpiSdk
{

class AudioPlugin;

// Pointer to sound processing member function.
typedef void (AudioPlugin::* SubProcess_ptr2)(int sampleFrames);

// Pointer to event handler member function.
typedef void (AudioPlugin::* MpBaseMemberPtr)(const gmpi::MpEvent*);

class ProcessorHost
{
	gmpi::IMpAudioPluginHost* host = {};

public:
	int32_t Init(gmpi::IMpUnknown* phost)
	{
		return phost->queryInterface(gmpi::MP_AUDIO_PLUGIN_HOST, (void**) &host);
	}

	// Plugin sending out control data.
	int32_t setPin(int32_t blockRelativeTimestamp, int32_t pinId, int32_t size, const void* data)
	{
		assert(host);
		return host->setPin(blockRelativeTimestamp, pinId, size, data);
	}

	// Plugin audio output start/stop (silence detection).
	int32_t setPinStreaming(int32_t blockRelativeTimestamp, int32_t pinId, int32_t isStreaming)
	{
		assert(host);
		return host->setPinStreaming(blockRelativeTimestamp, pinId, isStreaming);
	}

	// Plugin indicates no processing needed until input state changes.
	int32_t sleep()
	{
		assert(host);
		return host->sleep();
	}

	// Query audio buffer size.
	int32_t getBlockSize()
	{
		assert(host);
		return host->getBlockSize();
	}

	int32_t getHandle()
	{
		assert(host);
		return host->getHandle();
	}

	void SetLatency(int32_t latency)
	{
		assert(host);
		host->setLatency(latency);
	}

	gmpi::IMpAudioPluginHost* Get()
	{
		return host;
	}
};

class MpPinBase
{
public:
	MpPinBase() : id_(-1), plugin_(0), eventHandler_(0) {};
	virtual ~MpPinBase(){};
	void initialize( AudioPlugin* plugin, int PinId, MpBaseMemberPtr handler = 0 );

	// overrides for audio pins_
	virtual void setBuffer( float* buffer ) = 0;
	virtual void preProcessEvent( const gmpi::MpEvent* ){}

	virtual void processEvent( const gmpi::MpEvent* e );
	virtual void postProcessEvent( const gmpi::MpEvent* ){}

	int getId(){return id_;};
	virtual int getDatatype() const = 0;
	virtual int getDirection() const = 0;
	virtual MpBaseMemberPtr getDefaultEventHandler() = 0;
	virtual void sendFirstUpdate() = 0;

protected:
	void sendPinUpdate( int32_t rawSize, void* rawData, int32_t blockPosition = - 1 );
	int id_;
	class AudioPlugin* plugin_;
	MpBaseMemberPtr eventHandler_;
};

template
<typename T, int pinDatatype = MpTypeTraits<T>::PinDataType>
class MpControlPinBase : public MpPinBase
{
public:
	MpControlPinBase()
	{
	}
	MpControlPinBase( T initialValue ) : value_( initialValue )
	{
	}
	void sendPinUpdate(int blockPosition = -1)
	{
		MpPinBase::sendPinUpdate( rawSize(), rawData(), blockPosition );
	}
	void setBuffer( float* /*buffer*/ ) override
	{
		assert(false && "Control-rate pins_ don't have a buffer");
	}
	const T& getValue() const
	{
		assert( id_ != -1 && "remember initializePin() in constructor?" );
		return value_;
	}
	operator T()
	{
		assert( id_ != -1 && "remember initializePin() in constructor?" );
		return value_;
	}
	void setValue( const T &value, int blockPosition = -1 )
	{
		if( !variablesAreEqual<T>(value, value_) ) // avoid unnesc updates
		{
			value_ = value;
			sendPinUpdate( blockPosition );
		}
	}
	const T& operator=(const T &value)
	{
		if( !variablesAreEqual<T>(value, value_) ) // avoid unnesc updates
		{
			value_ = value;
			sendPinUpdate();
		}
		return value_;
	}
	bool operator==(T other)
	{
		assert( plugin_ != 0 && "Don't forget initializePin() on each pin in your constructor." );
		return variablesAreEqual<T>(other, value_);
	}
	virtual void setValueRaw(int size, const void* data)
	{
		VariableFromRaw<T>(size, data, value_);
	}
	virtual void setValueRaw(size_t size, const void* data)
	{
		VariableFromRaw<T>(static_cast<int>(size), data, value_);
	}
	virtual int rawSize() const
	{
		return variableRawSize<T>(value_);
	}
	virtual void* rawData()
	{
		return variableRawData<T>(value_);
	}
	int getDatatype() const override
	{
		return pinDatatype;
	}
	void preProcessEvent( const gmpi::MpEvent* e ) override
	{
		switch(e->eventType)
		{
			case gmpi::EVENT_PIN_SET:
				if(e->extraData != 0)
				{
					setValueRaw(e->parm2, e->extraData);
				}
				else
				{
					setValueRaw(e->parm2, &(e->parm3));
				}
				freshValue_ = true;
				break;
		};
	}
	void postProcessEvent( const gmpi::MpEvent* e ) override
	{
		switch(e->eventType)
		{
			case gmpi::EVENT_PIN_SET:
				freshValue_ = false;
				break;
		};
	}
	MpBaseMemberPtr getDefaultEventHandler() override
	{
		return 0;
	}
	bool isUpdated() const
	{
		return freshValue_;
	}
	void sendFirstUpdate() override
	{
		sendPinUpdate();
	}

protected:
	T value_ = {};
	bool freshValue_ = false; // true = value_ has been updated by host on current sample_clock
};

template
<typename T, int pinDirection_, int pinDatatype = gmpi::MpTypeTraits<T>::PinDataType>
class MpControlPin : public MpControlPinBase< T, pinDatatype >
{
public:
	MpControlPin() : MpControlPinBase< T, pinDatatype >()
	{
	}
	MpControlPin( T initialValue ) : MpControlPinBase< T, pinDatatype >( initialValue )
	{
	}
	int getDirection() const override
	{
		return pinDirection_;
	}
	const T& operator=(const T &value)
	{
		// GCC don't like using plugin_ in this scope. assert( plugin_ != 0 && "Don't forget initializePin() on each pin in your constructor." );
		return MpControlPinBase< T, pinDatatype> ::operator=(value);
	}
	// todo: specialise for value_ vs ref types !!!
	const T& operator=(const MpControlPin<T, gmpi::MP_IN, pinDatatype> &other)
	{
		return operator=(other.getValue());
	}
	const T& operator=(const MpControlPin<T, gmpi::MP_OUT, pinDatatype> &other)
	{
		return operator=(other.getValue());
	}
};

class MpAudioPinBase : public MpPinBase
{
public:
	MpAudioPinBase(){}
	void setBuffer( float* buffer ) override
	{
		buffer_ = buffer;
	}
	float* getBuffer( void ) const
	{
		return buffer_;
	}
	float getValue(int bufferPos = -1) const;
	operator float() const
	{
		return getValue();
	}
	bool operator==(float other)
	{
		return other == getValue();
	}
	virtual void setValueRaw(int /*size*/, void* /*data*/)
	{
		assert(false && "Audio-rate pins_ don't support setValueRaw");
	}
	int getDatatype() const override
	{
		return gmpi::MP_AUDIO;
	}
	MpBaseMemberPtr getDefaultEventHandler() override
	{
		return 0;
	}
	bool isStreaming()
	{
		return isStreaming_;
	}

protected:
	float* buffer_ = {};
	bool isStreaming_ = false; // true = active audio, false = silence or constant input level
};

class AudioInPin : public MpAudioPinBase
{
public:
	AudioInPin() : freshValue_(true){}
	int getDirection() const override
	{
		return gmpi::MP_IN;
	}
	void preProcessEvent( const gmpi::MpEvent* e ) override;
	void postProcessEvent( const gmpi::MpEvent* e ) override
	{
		switch(e->eventType)
		{
			case gmpi::EVENT_PIN_STREAMING_START:
			case gmpi::EVENT_PIN_STREAMING_STOP:
				freshValue_ = false;
				break;
		};
	}

	bool isUpdated() const
	{
		return freshValue_;
	}
	void sendFirstUpdate() override {} // N/A.

private:
	bool freshValue_; // true = value_ has been updated on current sample_clock
};

class AudioOutPin : public MpAudioPinBase
{
public:
	int getDirection() const override
	{
		return gmpi::MP_OUT;
	}

	// Indicate output pin's value changed, but it's not streaming (a 'one-off' change).
	void setUpdated(int blockPosition = -1)
	{
		setStreaming( false, blockPosition );
	}
	void setStreaming(bool isStreaming, int blockPosition = -1);
	void sendFirstUpdate() override
	{
		setStreaming( true );
	}
};

typedef MpControlPin<int, gmpi::MP_IN>			IntInPin;
typedef MpControlPin<int, gmpi::MP_OUT>			IntOutPin;
typedef MpControlPin<float, gmpi::MP_IN>		FloatInPin;
typedef MpControlPin<float, gmpi::MP_OUT>		FloatOutPin;
typedef MpControlPin<gmpi::MpBlob, gmpi::MP_IN>		BlobInPin;
typedef MpControlPin<gmpi::MpBlob, gmpi::MP_OUT>		BlobOutPin;
typedef MpControlPin<std::wstring, gmpi::MP_OUT>StringOutPin;

typedef MpControlPin<bool, gmpi::MP_IN>			BoolInPin;
typedef MpControlPin<bool, gmpi::MP_OUT>		BoolOutPin;

// enum (List) pin based on Int Pin
typedef MpControlPin<int, gmpi::MP_IN, gmpi::MP_ENUM>	EnumInPin;
typedef MpControlPin<int, gmpi::MP_OUT, gmpi::MP_ENUM>	EnumOutPin;

class StringInPin : public MpControlPin<std::wstring, gmpi::MP_IN>
{
public:
	explicit operator std::string(); // convert to UTF8 encoded.
};

class MidiInPin : public MpPinBase
{
public:
	int getDatatype() const override
	{
		return gmpi::MP_MIDI;
	}
	int getDirection() const override
	{
		return gmpi::MP_IN;
	}

	// These members not needed for MIDI.
	void setBuffer(float* /*buffer*/) override
	{
		assert( false && "MIDI pins_ don't have a buffer" );
	}
	MpBaseMemberPtr getDefaultEventHandler() override;
	void sendFirstUpdate() override {}
};

class MidiOutPin : public MidiInPin
{
public:
	int getDirection() const override
	{
		return gmpi::MP_OUT;
	}
	MpBaseMemberPtr getDefaultEventHandler() override
	{
		assert( false && "output pins don't need event handler" );
		return 0;
	}

	virtual void send(const unsigned char* data, int size, int blockPosition = -1);
};


class AudioPlugin : public gmpi::IMpAudioPlugin
{
	friend class TempBlockPositionSetter;
	friend class MpAudioPinBase;
	friend class AudioOutPin;
	friend class AudioInPin;
	friend class MpPinBase;
	friend class MidiInPin;
	friend class MidiOutPin;
	
public:
	AudioPlugin();
	virtual ~AudioPlugin() {}

	// IMpAudioPlugin methods
	int32_t setHost(IMpUnknown* host) override;
	int32_t open() override;
	int32_t setBuffer(int32_t pinId, float* buffer) override;
	void    process(int32_t count, const gmpi::MpEvent* events) override;

	// overridables
	virtual void onGraphStart();	// called on very first sample.
	virtual void onSetPins() {}  // one or more pins_ updated.  Check pin update flags to determine which ones.
	virtual void onMidiMessage(int pin, const unsigned char* midiMessage, int size) {}

	// access to the DAW
	GmpiSdk::ProcessorHost host;

protected:
	void initializePin(int PinId, MpPinBase& pin, MpBaseMemberPtr handler = 0);
	void initializePin(MpPinBase& pin, MpBaseMemberPtr handler = 0)
	{
		int idx = 0;
		if (!pins_.empty())
		{
			idx = pins_.rbegin()->first + 1;
		}
		initializePin(idx, pin, handler); // Automatic indexing.
	}

	const float* getBuffer(const AudioInPin& pin) const
	{
		return blockPos_ + pin.getBuffer();
	}
	float* getBuffer(const AudioOutPin& pin) const
	{
		return blockPos_ + pin.getBuffer();
	}

	template <typename PointerToMember>
	void setSubProcess(PointerToMember functionPointer)
	{
		// under normal use, curSubProcess_ and saveSubProcess_ are the same.
		if (curSubProcess_ == saveSubProcess_)
		{
			curSubProcess_ = saveSubProcess_ = static_cast <SubProcess_ptr2> (functionPointer);
		}
		else // in sleep mode.
		{
			saveSubProcess_ = static_cast <SubProcess_ptr2> (functionPointer);
			assert(saveSubProcess_ != &AudioPlugin::subProcessPreSleep);
		}
	}

	SubProcess_ptr2 getSubProcess()
	{
		return saveSubProcess_;
	}
	int getBlockPosition()
	{
		return blockPos_;
	}
	void setSleep(bool isOkToSleep);

	void OnPinStreamingChange(bool isStreaming);

	// the default routine, if none set by module.
	void subProcessNothing(int /*sampleFrames*/) {}
	void subProcessPreSleep(int sampleFrames);

	void preProcessEvent( const gmpi::MpEvent* e );
	void processEvent( const gmpi::MpEvent* e );
	void postProcessEvent( const gmpi::MpEvent* e );
	void midiHelper( const gmpi::MpEvent* e );

	void resetSleepCounter();
	void nudgeSleepCounter()
	{
		sleepCount_ = (std::max)(sleepCount_, 1);
	}

	// identification and reference countin
	GMPI_QUERYINTERFACE1(gmpi::MP_IID_AUDIO_PLUGIN, IMpAudioPlugin);
	GMPI_REFCOUNT;

protected:
	SubProcess_ptr2 curSubProcess_ = &AudioPlugin::subProcessPreSleep;
	SubProcess_ptr2 saveSubProcess_ = &AudioPlugin::subProcessNothing; // saves curSubProcess_ while sleeping
	std::map<int, MpPinBase*> pins_;

	int blockPos_;				// valid only during processEvent()
	int sleepCount_;			// sleep countdown timer.
	int streamingPinCount_;		// tracks how many pins streaming.
	enum { SLEEP_AUTO = -1, SLEEP_DISABLE, SLEEP_ENABLE } canSleepManualOverride_;
	bool eventsComplete_;		// Flag indicates all events have been delt with, and module is safe to sleep.

#if defined(_DEBUG)
public:
	bool debugIsOpen_ = false;
	bool blockPosExact_ = true;
	bool debugGraphStartCalled_ = false;
#endif
};

// When sending values out a pin, it's cleaner if the block-position is known,
// however in subProcess(), we can't usually identify a specific block position automatically.
// This helper class lets you set the block-position manually in able to use shorthand pin setting syntax. e.g. pinOut = 3;
class TempBlockPositionSetter
{
	bool saveBlockPosExact_;
	int saveBlockPosition_;
	AudioPlugin* module_;

public:
	TempBlockPositionSetter(AudioPlugin* module, int currentBlockPosition)
	{
		module_ = module;
		saveBlockPosition_ = module_->getBlockPosition();
		module_->blockPos_ = currentBlockPosition;

#if defined(_DEBUG)
		saveBlockPosExact_ = module->blockPosExact_;
		module_->blockPosExact_ = true;
#endif
	}

	~TempBlockPositionSetter()
	{
		module_->blockPos_ = saveBlockPosition_;
#if defined(_DEBUG)
		module_->blockPosExact_ = saveBlockPosExact_;
#endif
	}
};

} // namespace
