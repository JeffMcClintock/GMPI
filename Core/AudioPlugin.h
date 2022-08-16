#pragma once

/*
  GMPI - Generalized Music Plugin Interface specification.
  Copyright 2007-2022 Jeff McClintock.

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

#include <map>
#include "Common.h"

// Platform specific definitions.
#if defined __BORLANDC__
#pragma -a8
#elif defined(_WIN32) || defined(__FLAT__) || defined (CBUILDER)
#pragma pack(push,8)
#endif

#ifndef DECLSPEC_NOVTABLE
#if defined(__cplusplus)
#define DECLSPEC_NOVTABLE   __declspec(novtable)
#else
#define DECLSPEC_NOVTABLE
#endif
#endif

namespace gmpi
{

enum class EventType : int32_t
{
    PinSet            = 100, // A parameter has changed value.
    PinStreamingStart = 101, // An input is not silent.
    PinStreamingStop  = 102, // An input is silent.
    Midi              = 103, // A MIDI message.
    GraphStart        = 104, // Plugin is about to process the very first sample.
};

struct Event
{
    int32_t timeDelta;
    EventType eventType;
    int32_t parm1;
    int32_t parm2;
    int32_t parm3;
    int32_t parm4;
    char* extraData;
    Event* next;
};

// INTERFACE 'IAudioPlugin'
struct DECLSPEC_NOVTABLE IAudioPlugin : public IUnknown
{
    virtual ReturnCode open(IUnknown* host) = 0;
    virtual ReturnCode setBuffer(int32_t pinId, float* buffer) = 0;
    virtual void process(int32_t count, const Event* events) = 0;

    // {23835D7E-DCEB-4B08-A9E7-B43F8465939E}
    inline static const Guid guid =
    { 0x23835D7E, 0xDCEB, 0x4B08, { 0xA9, 0xE7, 0xB4, 0x3F, 0x84, 0x65, 0x93, 0x9E} };
};

// INTERFACE 'IAudioPluginHost'
struct DECLSPEC_NOVTABLE IAudioPluginHost : public IUnknown
{
    virtual ReturnCode setPin(int32_t timestamp, int32_t pinId, int32_t size, const void* data) = 0;
    virtual ReturnCode setPinStreaming(int32_t timestamp, int32_t pinId, bool isStreaming) = 0;
    virtual ReturnCode setLatency(int32_t latency) = 0;
    virtual ReturnCode sleep() = 0;
    virtual int32_t getBlockSize() = 0;
    virtual int32_t getSampleRate() = 0;
    virtual int32_t getHandle() = 0;

    // {87CCD426-71D7-414E-A9A6-5ADCA81C7420}
    inline static const Guid guid =
    { 0x87CCD426, 0x71D7, 0x414E, { 0xA9, 0xA6, 0x5A, 0xDC, 0xA8, 0x1C, 0x74, 0x20} };
};


// Platform specific definitions.
#if defined __BORLANDC__
#pragma -a-
#elif defined(_WIN32) || defined(__FLAT__) || defined (CBUILDER)
#pragma pack(pop)
#endif

// Pointer to sound processing member function.
class AudioPlugin;
typedef void (AudioPlugin::* SubProcessPtr)(int sampleFrames);

// Pointer to event handler member function.
typedef void (AudioPlugin::* MpBaseMemberPtr)(const gmpi::Event*);

class MpPinBase
{
public:
	MpPinBase(){}
	virtual ~MpPinBase(){}
	void initialize( AudioPlugin* plugin, int PinId, MpBaseMemberPtr handler = 0 );

	// overrides for audio pins_
	virtual void setBuffer( float* buffer ) = 0;
	virtual void preProcessEvent( const gmpi::Event* ){}

	virtual void processEvent( const gmpi::Event* e );
	virtual void postProcessEvent( const gmpi::Event* ){}

	int getId(){return id_;}
	virtual gmpi::PinDatatype getDatatype() const = 0;
	virtual gmpi::PinDirection getDirection() const = 0;
	virtual MpBaseMemberPtr getDefaultEventHandler() = 0;
	virtual void sendFirstUpdate() = 0;

protected:
	void sendPinUpdate( int32_t rawSize, const void* rawData, int32_t blockPosition = - 1 );
	int id_ = -1;
	class AudioPlugin* plugin_ = {};
	MpBaseMemberPtr eventHandler_ = {};
};

template
<typename T, int pinDatatype = gmpi_sdk::PinTypeTraits<T>::PinDataType>
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
		assert( plugin_ != 0 && "Don't forget initializePin() on each pin in your constructor." );
		return other == value_;
	}
	virtual void setValueRaw(int size, const void* data)
	{
		gmpi_sdk::VariableFromRaw<T>(size, data, value_);
	}
	virtual void setValueRaw(size_t size, const void* data)
	{
		gmpi_sdk::VariableFromRaw<T>(static_cast<int>(size), data, value_);
	}
	virtual int rawSize() const
	{
		return gmpi_sdk::variableRawSize<T>(value_);
	}
	virtual const void* rawData()
	{
		return gmpi_sdk::variableRawData<T>(value_);
	}
	gmpi::PinDatatype getDatatype() const override
	{
		return (gmpi::PinDatatype) pinDatatype;
	}
	void preProcessEvent( const gmpi::Event* e ) override
	{
		if(e->eventType == gmpi::EventType::PinSet)
		{
			if(e->extraData != 0)
			{
				setValueRaw(e->parm2, e->extraData);
			}
			else
			{
				setValueRaw(e->parm2, &(e->parm3));
			}
			freshValue_ = true;
		};
	}
	void postProcessEvent( const gmpi::Event* e ) override
	{
		if(e->eventType == gmpi::EventType::PinSet)
		{
			freshValue_ = false;
		};
	}
	MpBaseMemberPtr getDefaultEventHandler() override
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

template
<typename T, int pinDirection_, int pinDatatype = gmpi_sdk::PinTypeTraits<T>::PinDataType>
class MpControlPin : public MpControlPinBase< T, pinDatatype >
{
public:
	MpControlPin() : MpControlPinBase< T, pinDatatype >()
	{
	}
	MpControlPin( T initialValue ) : MpControlPinBase< T, pinDatatype >( initialValue )
	{
	}
	gmpi::PinDirection getDirection() const override
	{
		return (gmpi::PinDirection) pinDirection_;
	}
	const T& operator=(const T &value)
	{
		// GCC don't like using plugin_ in this scope. assert( plugin_ != 0 && "Don't forget initializePin() on each pin in your constructor." );
		return MpControlPinBase< T, pinDatatype> ::operator=(value);
	}
	// todo: specialise for value_ vs ref types !!!

	const T& operator=(const MpControlPin<T, static_cast<int>(gmpi::PinDirection::In), pinDatatype> &other)
	{
		return operator=(other.getValue());
	}

	const T& operator=(const MpControlPin<T, static_cast<int>(gmpi::PinDirection::Out), pinDatatype> &other)
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
	gmpi::PinDatatype getDatatype() const override
	{
		return gmpi::PinDatatype::Audio;
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

template
<int pinDirection_>
class MpAudioPinBaseB : public MpAudioPinBase
{
public:
	gmpi::PinDirection getDirection() const override
	{
		return (gmpi::PinDirection) pinDirection_;
	}
};

class AudioInPin final : public MpAudioPinBaseB<static_cast<int>(gmpi::PinDirection::In)>
{
public:
	AudioInPin(){}
	void preProcessEvent( const gmpi::Event* e ) override;
	void postProcessEvent( const gmpi::Event* e ) override
	{
		switch(e->eventType)
		{
			case gmpi::EventType::PinStreamingStart:
			case gmpi::EventType::PinStreamingStop:
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
	bool freshValue_ = true; // true = value_ has been updated on current sample_clock
};

class AudioOutPin final : public MpAudioPinBaseB<static_cast<int>(gmpi::PinDirection::Out)>
{
public:
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

typedef MpControlPin<int, static_cast<int>(gmpi::PinDirection::In)>				IntInPin;
typedef MpControlPin<int, static_cast<int>(gmpi::PinDirection::Out)>			IntOutPin;
typedef MpControlPin<float, static_cast<int>(gmpi::PinDirection::In)>			FloatInPin;
typedef MpControlPin<float, static_cast<int>(gmpi::PinDirection::Out)>			FloatOutPin;
typedef MpControlPin<gmpi::Blob, static_cast<int>(gmpi::PinDirection::In)>		BlobInPin;
typedef MpControlPin<gmpi::Blob, static_cast<int>(gmpi::PinDirection::Out)>		BlobOutPin;
typedef MpControlPin<std::string, static_cast<int>(gmpi::PinDirection::In)>		StringInPin;
typedef MpControlPin<std::string, static_cast<int>(gmpi::PinDirection::Out)>	StringOutPin;

typedef MpControlPin<bool, static_cast<int>(gmpi::PinDirection::In)>			BoolInPin;
typedef MpControlPin<bool, static_cast<int>(gmpi::PinDirection::Out)>			BoolOutPin;

// enum (List) pin based on Int Pin
typedef MpControlPin<int, static_cast<int>(gmpi::PinDirection::In), static_cast<int>(gmpi::PinDatatype::Enum)>	EnumInPin;
typedef MpControlPin<int, static_cast<int>(gmpi::PinDirection::Out), static_cast<int>(gmpi::PinDatatype::Enum)>	EnumOutPin;

class MidiInPin : public MpPinBase
{
public:
	gmpi::PinDatatype getDatatype() const override
	{
		return gmpi::PinDatatype::Midi;
	}
	gmpi::PinDirection getDirection() const override
	{
		return gmpi::PinDirection::In;
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
	gmpi::PinDirection getDirection() const override
	{
		return gmpi::PinDirection::Out;
	}
	MpBaseMemberPtr getDefaultEventHandler() override
	{
		assert( false && "output pins don't need event handler" );
		return 0;
	}

	virtual void send(const unsigned char* data, int size, int blockPosition = -1);
};

class TempBlockPositionSetter;

class AudioPlugin : public gmpi::IAudioPlugin
{
	friend class TempBlockPositionSetter;

public:
	AudioPlugin();
	virtual ~AudioPlugin() {}

	// IAudioPlugin methods
	gmpi::ReturnCode open(IUnknown* phost) override;
	gmpi::ReturnCode setBuffer(int32_t pinId, float* buffer) override;
	void process(int32_t count, const gmpi::Event* events) override;

	// overridables
	virtual void onGraphStart();	// called on very first sample.
	virtual void onSetPins() {}  // one or more pins_ updated.  Check pin update flags to determine which ones.
	virtual void onMidiMessage(int pin, const uint8_t* midiMessage, int size) {}

	// access to the DAW
	gmpi_sdk::shared_ptr<gmpi::IAudioPluginHost> host;

	// Communication with pins.
	int getBlockPosition() const
	{
		return blockPos_;
	}
	void midiHelper( const gmpi::Event* e );
	void OnPinStreamingChange(bool isStreaming);
	void resetSleepCounter();
	void nudgeSleepCounter()
	{
		sleepCount_ = (std::max)(sleepCount_, 1);
	}

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
			curSubProcess_ = saveSubProcess_ = static_cast <SubProcessPtr> (functionPointer);
		}
		else // in sleep mode.
		{
			saveSubProcess_ = static_cast <SubProcessPtr> (functionPointer);
			assert(saveSubProcess_ != &AudioPlugin::subProcessPreSleep);
		}
	}

	SubProcessPtr getSubProcess()
	{
		return saveSubProcess_;
	}
	void setSleep(bool isOkToSleep);

	// the default routine, if none set by module.
	void subProcessNothing(int /*sampleFrames*/) {}
	void subProcessPreSleep(int sampleFrames);

	void preProcessEvent( const gmpi::Event* e );
	void processEvent( const gmpi::Event* e );
	void postProcessEvent( const gmpi::Event* e );

	// identification and reference countin
	GMPI_QUERYINTERFACE(gmpi::IAudioPlugin::guid, IAudioPlugin);
	GMPI_REFCOUNT;

protected:
	SubProcessPtr curSubProcess_ = &AudioPlugin::subProcessPreSleep;
	SubProcessPtr saveSubProcess_ = &AudioPlugin::subProcessNothing; // saves curSubProcess_ while sleeping
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
