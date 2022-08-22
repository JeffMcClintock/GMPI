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
#include "GmpiApiAudio.h"
#include "GmpiSdkCommon.h"
#include "RefCountMacros.h"

namespace gmpi
{

// Pointer to sound processing member function.
class AudioPlugin;
typedef void (AudioPlugin::* SubProcessPtr)(int sampleFrames);

// Pointer to event handler member function.
typedef void (AudioPlugin::* MpBaseMemberPtr)(const api::Event*);

class MpPinBase
{
public:
	MpPinBase(){}
	virtual ~MpPinBase(){}
	void initialize( AudioPlugin* plugin, int PinId, MpBaseMemberPtr handler = 0 );

	// overrides for audio pins_
	virtual void setBuffer( float* buffer ) = 0;
	virtual void preProcessEvent( const api::Event* ){}

	virtual void processEvent( const api::Event* e );
	virtual void postProcessEvent( const api::Event* ){}

	int getId(){return id_;}
	virtual PinDatatype getDatatype() const = 0;
	virtual PinDirection getDirection() const = 0;
	virtual MpBaseMemberPtr getDefaultEventHandler() = 0;
	virtual void sendFirstUpdate() = 0;

protected:
	void sendPinUpdate( int32_t rawSize, const void* rawData, int32_t blockPosition = - 1 );
	int id_ = -1;
	class AudioPlugin* plugin_ = {};
	MpBaseMemberPtr eventHandler_ = {};
};

template
<typename T, int pinDatatype = PinTypeTraits<T>::PinDataType>
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
	virtual const void* rawData()
	{
		return variableRawData<T>(value_);
	}
	PinDatatype getDatatype() const override
	{
		return (PinDatatype) pinDatatype;
	}
	void preProcessEvent( const api::Event* e ) override
	{
		if(e->eventType == api::EventType::PinSet)
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
	void postProcessEvent( const api::Event* e ) override
	{
		if(e->eventType == api::EventType::PinSet)
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
<typename T, int pinDirection_, int pinDatatype = PinTypeTraits<T>::PinDataType>
class MpControlPin : public MpControlPinBase< T, pinDatatype >
{
public:
	MpControlPin() : MpControlPinBase< T, pinDatatype >()
	{
	}
	MpControlPin( T initialValue ) : MpControlPinBase< T, pinDatatype >( initialValue )
	{
	}
	PinDirection getDirection() const override
	{
		return (PinDirection) pinDirection_;
	}
	const T& operator=(const T &value)
	{
		// GCC don't like using plugin_ in this scope. assert( plugin_ != 0 && "Don't forget initializePin() on each pin in your constructor." );
		return MpControlPinBase< T, pinDatatype> ::operator=(value);
	}
	// todo: specialise for value_ vs ref types !!!

	const T& operator=(const MpControlPin<T, static_cast<int>(PinDirection::In), pinDatatype> &other)
	{
		return operator=(other.getValue());
	}

	const T& operator=(const MpControlPin<T, static_cast<int>(PinDirection::Out), pinDatatype> &other)
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
	PinDatatype getDatatype() const override
	{
		return PinDatatype::Audio;
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
	PinDirection getDirection() const override
	{
		return (PinDirection) pinDirection_;
	}
};

class AudioInPin final : public MpAudioPinBaseB<static_cast<int>(PinDirection::In)>
{
public:
	AudioInPin(){}
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
	bool freshValue_ = true; // true = value_ has been updated on current sample_clock
};

class AudioOutPin final : public MpAudioPinBaseB<static_cast<int>(PinDirection::Out)>
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

typedef MpControlPin<int, static_cast<int>(PinDirection::In)>				IntInPin;
typedef MpControlPin<int, static_cast<int>(PinDirection::Out)>			IntOutPin;
typedef MpControlPin<float, static_cast<int>(PinDirection::In)>			FloatInPin;
typedef MpControlPin<float, static_cast<int>(PinDirection::Out)>			FloatOutPin;
typedef MpControlPin<Blob, static_cast<int>(PinDirection::In)>		BlobInPin;
typedef MpControlPin<Blob, static_cast<int>(PinDirection::Out)>		BlobOutPin;
typedef MpControlPin<std::string, static_cast<int>(PinDirection::In)>		StringInPin;
typedef MpControlPin<std::string, static_cast<int>(PinDirection::Out)>	StringOutPin;

typedef MpControlPin<bool, static_cast<int>(PinDirection::In)>			BoolInPin;
typedef MpControlPin<bool, static_cast<int>(PinDirection::Out)>			BoolOutPin;

// enum (List) pin based on Int Pin
typedef MpControlPin<int, static_cast<int>(PinDirection::In), static_cast<int>(PinDatatype::Enum)>	EnumInPin;
typedef MpControlPin<int, static_cast<int>(PinDirection::Out), static_cast<int>(PinDatatype::Enum)>	EnumOutPin;

class MidiInPin : public MpPinBase
{
public:
	PinDatatype getDatatype() const override
	{
		return PinDatatype::Midi;
	}
	PinDirection getDirection() const override
	{
		return PinDirection::In;
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
	PinDirection getDirection() const override
	{
		return PinDirection::Out;
	}
	MpBaseMemberPtr getDefaultEventHandler() override
	{
		assert( false && "output pins don't need event handler" );
		return 0;
	}

	virtual void send(const unsigned char* data, int size, int blockPosition = -1);
};

class TempBlockPositionSetter;

class AudioPlugin : public api::IAudioPlugin
{
	friend class TempBlockPositionSetter;

public:
	AudioPlugin();
	virtual ~AudioPlugin() {}

	// IAudioPlugin methods
	gmpi::ReturnCode open(api::IUnknown* phost) override;
	gmpi::ReturnCode setBuffer(int32_t pinId, float* buffer) override;
	void process(int32_t count, const api::Event* events) override;

	// overridables
	virtual void onGraphStart();	// called on very first sample.
	virtual void onSetPins() {}  // one or more pins_ updated.  Check pin update flags to determine which ones.
	virtual void onMidiMessage(int pin, const uint8_t* midiMessage, int size) {}

	// access to the DAW
	gmpi2_sdk::shared_ptr<api::IAudioPluginHost> host;

	// Communication with pins.
	int getBlockPosition() const
	{
		return blockPos_;
	}
	void midiHelper( const api::Event* e );
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

	void preProcessEvent( const api::Event* e );
	void processEvent( const api::Event* e );
	void postProcessEvent( const api::Event* e );

	// identification and reference countin
	GMPI_QUERYINTERFACE(api::IAudioPlugin::guid, IAudioPlugin);
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
