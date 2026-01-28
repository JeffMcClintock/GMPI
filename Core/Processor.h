#pragma once

/*
  GMPI - Generalized Music Plugin Interface specification.
  Copyright 2007-2025 Jeff McClintock.

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "Common.h"
#include "GmpiApiAudio.h"
#include "GmpiSdkCommon.h"
#include "RefCountMacros.h"
#include "Core/GmpiMidi.h"

namespace gmpi
{

// Pointer to sound processing member function.
class Processor;
typedef void (Processor::* SubProcessPtr)(int sampleFrames);

// Pointer to event handler member function.
typedef void (Processor::* ProcessorMemberPtr)(const api::Event*);

class PinBase
{
public:
	PinBase(){}
	virtual ~PinBase(){}
	void initialize(Processor* plugin, int PinId, ProcessorMemberPtr handler = {});

	// overrides for audio pins
	virtual void setBuffer( float* buffer ) = 0;
	virtual void preProcessEvent( const api::Event* ){}

	virtual void processEvent( const api::Event* e );
	virtual void postProcessEvent( const api::Event* ){}

	int getIndex() const {return idx_;}
	virtual PinDatatype getDatatype() const = 0;
	virtual PinDirection getDirection() const = 0;
	virtual ProcessorMemberPtr getDefaultEventHandler() = 0;
	virtual void sendFirstUpdate() = 0;

protected:
	void sendPinUpdate( int32_t rawSize, const uint8_t* rawData, int32_t blockPosition = - 1 );
	int idx_ = -1;
	class Processor* plugin_ = {};
	ProcessorMemberPtr eventHandler_ = {};
};

template
<typename T, PinDatatype pinDatatype = PinTypeTraits<T>::PinDataType>
class ControlPinBase : public PinBase
{
public:
	ControlPinBase() = default;
	void sendPinUpdate(int blockPosition = -1)
	{
		PinBase::sendPinUpdate( rawSize(), rawData(), blockPosition );
	}
	void setBuffer( float* /*buffer*/ ) override
	{
		assert(false && "Control-rate pins_ don't have a buffer");
	}
	const T& getValue() const
	{
		assert( idx_ != -1 && "remember init() in constructor?" );
		return value_;
	}
	operator T()
	{
		assert( idx_ != -1 && "remember init() in constructor?" );
		return value_;
	}
	void setValue( const T &value, int blockPosition = -1 )
	{
		if(value != value_) // avoid unnesc updates
		{
			value_ = value;
			sendPinUpdate( blockPosition );
		}
	}
	const T& operator=(const T &value)
	{
		if( value != value_ ) // avoid unnesc updates
		{
			value_ = value;
			sendPinUpdate();
		}
		return value_;
	}
	bool operator==(T other)
	{
		assert( plugin_ != nullptr && "Don't forget init() on each pin in your constructor." );
		return other == value_;
	}

	void setValueRaw(std::span<const uint8_t> bytes)
	{
		valueFromData<T>(bytes, value_);
	}
	/*
	virtual void setValueRaw(int size, const uint8_t* data)
	{
		valueFromData<T>(size, data, value_);
	}
	virtual void setValueRaw(size_t size, const uint8_t* data)
	{
		valueFromData<T>(static_cast<int>(size), data, value_);
	}
	*/
	/*virtual*/ int rawSize() const
	{
		return dataSize<T>(value_);
	}
	/*virtual*/ const uint8_t* rawData()
	{
		return dataPtr<T>(value_);
	}
	PinDatatype getDatatype() const override
	{
		return pinDatatype;
	}
	void preProcessEvent( const api::Event* e ) override
	{
		if(e->eventType == api::EventType::PinSet)
		{
			setValueRaw(e->payload());
			freshValue_ = true;
		}
	}
	void postProcessEvent( const api::Event* e ) override
	{
		if(e->eventType == api::EventType::PinSet)
		{
			freshValue_ = false;
		}
	}
	ProcessorMemberPtr getDefaultEventHandler() override
	{
		return nullptr;
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

class AudioPinBase : public PinBase
{
public:
	void setBuffer( float* buffer ) override
	{
		buffer_ = buffer;
	}
	float* begin() const
	{
		return buffer_;
	}
	float getValue(int bufferPos = -1) const;
	operator float() const
	{
		return getValue();
	}
	bool operator==(float other) const
	{
		return other == getValue();
	}
	virtual void setValueRaw(int /*size*/, const uint8_t* /*data*/)
	{
		assert(false && "Audio-rate pins_ don't support setValueRaw");
	}
	PinDatatype getDatatype() const override
	{
		return PinDatatype::Audio;
	}
	ProcessorMemberPtr getDefaultEventHandler() override
	{
		return {};
	}
	bool isStreaming() const
	{
		return isStreaming_;
	}

protected:
	float* buffer_ = {};
	bool isStreaming_ = false; // true = active audio, false = silence or constant input level
};

template
<PinDirection pinDirection_>
class AudioPinBaseB : public AudioPinBase
{
public:
	PinDirection getDirection() const override
	{
		return pinDirection_;
	}
};

class AudioInPin final : public AudioPinBaseB<PinDirection::In>
{
public:
    AudioInPin();
	void preProcessEvent( const api::Event* e ) override;
	void postProcessEvent( const api::Event* e ) override
	{
		switch(e->eventType)
		{
			case api::EventType::PinStreamingStart:
			case api::EventType::PinStreamingStop:
				freshValue_ = false;
				break;

		default:
			break;
		};
	}

	bool isUpdated() const
	{
		return freshValue_;
	}
	void sendFirstUpdate() override {} // N/A.

private:
	bool freshValue_ = true; // has value_been updated on current sample_clock?
};

class AudioOutPin final : public AudioPinBaseB<PinDirection::Out>
{
public:
    AudioOutPin();
    
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

class MidiInPin : public PinBase
{
public:
	MidiInPin();

	PinDatatype getDatatype() const override
	{
		return PinDatatype::Midi;
	}
	PinDirection getDirection() const override
	{
		return PinDirection::In;
	}
	// This member not needed for MIDI.
	void setBuffer(float* /*buffer*/) override
	{
		assert( false && "MIDI pins_ don't have a buffer" );
	}
	ProcessorMemberPtr getDefaultEventHandler() override;
	void sendFirstUpdate() override {}
};

class MidiOutPin final : public PinBase
{
public:
	MidiOutPin();

	PinDatatype getDatatype() const override
	{
		return PinDatatype::Midi;
	}
	PinDirection getDirection() const override
	{
		return PinDirection::Out;
	}
	ProcessorMemberPtr getDefaultEventHandler() override
	{
		assert( false && "output pins don't need event handler" );
		return {};
	}
	// This member not needed for MIDI.
	void setBuffer(float* /*buffer*/) override
	{
		assert(false && "MIDI pins_ don't have a buffer");
	}

	void send(gmpi::midi2::message_view msg, int blockPosition = -1);
	void send(const unsigned char* data, int size, int blockPosition = -1);
	void sendFirstUpdate() override {} // N/A.
};

class TempBlockPositionSetter;

class Processor : public api::IProcessor
{
	friend class TempBlockPositionSetter;

public:
	inline static thread_local Processor* constructingInstance{};

	Processor();
	virtual ~Processor() {}

	// IAudioPlugin methods
	gmpi::ReturnCode open(api::IUnknown* phost) override;
	gmpi::ReturnCode setBuffer(int32_t PinIndex, float* buffer) override;
	void process(int32_t count, const api::Event* events) override;

	// overrides
	virtual void onGraphStart();	// called on very first sample.
	virtual void onSetPins() {}  // one or more pins_ updated.  Check pin update flags to determine which ones.
	virtual void onMidiMessage(int pin, std::span<const uint8_t> midiMessage) {}

	// access to the DAW
	gmpi::shared_ptr<api::IProcessorHost> host;

	// Communication with pins.
	int getBlockPosition() const
	{
		return blockPos_;
	}
	void midiHelper( const api::Event* e );
	void onPinStreamingChange(bool isStreaming);
	void resetSleepCounter();
	void nudgeSleepCounter()
	{
		sleepCount_ = (std::max)(sleepCount_, 1);
	}

	void init(int PinIndex, PinBase& pin, ProcessorMemberPtr handler = {});
	void init(PinBase& pin, ProcessorMemberPtr handler = {})
	{
		init(static_cast<int32_t>(pins_.size()), pin, handler); // Automatic indexing.
	}

protected:
	const float* getBuffer(const AudioInPin& pin) const
	{
		return blockPos_ + pin.begin();
	}
	float* getBuffer(const AudioOutPin& pin) const
	{
		return blockPos_ + pin.begin();
	}

	template <typename PointerToMember>
	void setSubProcess(PointerToMember functionPointer)
	{
		// under normal use, curSubProcess_ and saveSubProcess_ are the same.
		if (curSubProcess_ == saveSubProcess_)
		{
			curSubProcess_ = saveSubProcess_ = static_cast <SubProcessPtr> (functionPointer);
		}
		else // in sleep mode.
		{
			saveSubProcess_ = static_cast <SubProcessPtr> (functionPointer);
			assert(saveSubProcess_ != &Processor::subProcessPreSleep);
		}
	}

	SubProcessPtr getSubProcess() const
	{
		return saveSubProcess_;
	}
	void setSleep(bool isOkToSleep);

	// the default routine, if none set by module.
	void subProcessNothing(int /*sampleFrames*/) {}
	void subProcessPreSleep(int sampleFrames);

	void preProcessEvent( const api::Event* e );
	void processEvent( const api::Event* e );
	void postProcessEvent( const api::Event* e );

	// identification and reference counting
	GMPI_QUERYINTERFACE_METHOD(IProcessor);
	GMPI_REFCOUNT

protected:
	SubProcessPtr curSubProcess_ = &Processor::subProcessPreSleep;
	SubProcessPtr saveSubProcess_ = &Processor::subProcessNothing; // saves curSubProcess_ while sleeping
	std::vector<PinBase*> pins_;

	int blockPos_{};				// valid only during processEvent()
	int sleepCount_{};			// sleep countdown timer.
	int streamingPinCount_{};		// tracks how many pins streaming.
	enum { SLEEP_AUTO = -1, SLEEP_DISABLE, SLEEP_ENABLE } canSleepManualOverride_{ SLEEP_AUTO };
	bool eventsComplete_{ true };		// Flag indicates all events have been dealt with, and module is safe to sleep.

#if defined(_DEBUG)
public:
	bool debugIsOpen_ = false;
	bool blockPosExact_ = true;
	bool debugGraphStartCalled_ = false;
#endif
};

template
<typename T, PinDirection pinDirection_, PinDatatype pinDatatype = (PinDatatype)PinTypeTraits<T>::PinDataType>
class ControlPin : public ControlPinBase< T, pinDatatype >
{
public:
	ControlPin() : ControlPinBase< T, pinDatatype >()
	{
		// register with the plugin.
		if (Processor::constructingInstance)
			Processor::constructingInstance->init(*this);
	}
#if 0
	ControlPin(T initialValue) : ControlPinBase< T, pinDatatype >(initialValue)
	{
		// register with the plugin.
		if (Processor::constructingInstance)
			Processor::constructingInstance->init(*this);
	}
#endif
	PinDirection getDirection() const override
	{
		return pinDirection_;
	}
	const T& operator=(const T& value)
	{
		// GCC don't like using plugin_ in this scope. assert( plugin_ != nullptr && "Don't forget init() on each pin in your constructor." );
		return ControlPinBase< T, pinDatatype> ::operator=(value);
	}
	// todo: specialise for value_ vs ref types !!!

	const T& operator=(const ControlPin<T, PinDirection::In, pinDatatype>& other)
	{
		return operator=(other.getValue());
	}

	const T& operator=(const ControlPin<T, PinDirection::Out, pinDatatype>& other)
	{
		return operator=(other.getValue());
	}
};

typedef ControlPin<int, PinDirection::In>			IntInPin;
typedef ControlPin<int, PinDirection::Out>			IntOutPin;
typedef ControlPin<Blob, PinDirection::In>			BlobInPin;
typedef ControlPin<bool, PinDirection::In>			BoolInPin;
typedef ControlPin<bool, PinDirection::Out>			BoolOutPin;
typedef ControlPin<Blob, PinDirection::Out>			BlobOutPin;
typedef ControlPin<float, PinDirection::In>			FloatInPin;
typedef ControlPin<float, PinDirection::Out>		FloatOutPin;
typedef ControlPin<std::string, PinDirection::In>	StringInPin;
typedef ControlPin<std::string, PinDirection::Out>	StringOutPin;

// enum (List) pin based on Int Pin
typedef ControlPin<int, PinDirection::In, PinDatatype::Enum>	EnumInPin;
typedef ControlPin<int, PinDirection::Out, PinDatatype::Enum>	EnumOutPin;

// When sending values out a pin, it's cleaner if the block-position is known,
// however in subProcess(), we can't usually identify a specific block position automatically.
// This helper class lets you set the block-position manually in able to use shorthand pin setting syntax. e.g. pinOut = 3;
class TempBlockPositionSetter
{
#if defined(_DEBUG)
	bool saveBlockPosExact_;
#endif
	int saveBlockPosition_;
	Processor* module_;

public:
	TempBlockPositionSetter(Processor* module, int currentBlockPosition)
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
