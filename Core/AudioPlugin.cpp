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

// MpBase2 - implements the IMpPlugin2 interface.
#if !defined(_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif

#include <assert.h>
#include "AudioPlugin.h"

using namespace gmpi;

template<typename INTERFACE>
INTERFACE* as(api::IUnknown* com_object)
{
	INTERFACE* result = {};
	com_object->queryInterface(&INTERFACE::guid, (void**)&result);
	return result;
}

namespace gmpi
{
ReturnCode AudioPlugin::open(IUnknown* phost)
{
#if defined(_DEBUG)
	debugIsOpen_ = true;
#endif

	return host.Init(phost);
}

void AudioPlugin::process(int32_t count, const api::Event* events)
{
	assert(count > 0);

#if defined(_DEBUG)
	blockPosExact_ = false;
#endif

	blockPos_ = 0;
	int remain = count;
	const api::Event* next_event = events;

	for (;;)
	{
		if (next_event == 0) // fast version, when no events on list.
		{
			(this->*(curSubProcess_))(remain);
			break;
		}

		assert(next_event->timeDelta < count); // Event will happen in this block

		int delta_time = next_event->timeDelta - blockPos_;

		if (delta_time > 0) // then process intermediate samples
		{
			eventsComplete_ = false;

			(this->*(curSubProcess_))(delta_time);
			remain -= delta_time;

			eventsComplete_ = true;

			assert(remain != 0); // BELOW NEEDED?, seems non sense. If we are here, there is a event to process. Don't want to exit!
			if (remain == 0) // done
			{
				return;
			}

			blockPos_ += delta_time;
		}

		//int cur_timeStamp = next_event->timeDelta;
#if defined(_DEBUG)
		blockPosExact_ = true;
#endif
		assert(blockPos_ == next_event->timeDelta);

		// PRE-PROCESS EVENT
		bool pins_set_flag = false;
		const api::Event* e = next_event;
		do
		{
			preProcessEvent(e); // updates all pins_ values
			pins_set_flag = pins_set_flag || e->eventType == api::EventType::PinSet || e->eventType == api::EventType::PinStreamingStart || e->eventType == api::EventType::PinStreamingStop;
			e = e->next;
		} while (e != 0 && e->timeDelta == blockPos_); // cur_timeStamp );

		// PROCESS EVENT
		e = next_event;
		do
		{
			processEvent(e); // notify all pins_ values
			e = e->next;
		} while (e != 0 && e->timeDelta == blockPos_); //cur_timeStamp );

		if (pins_set_flag)
		{
			onSetPins();
		}

		// POST-PROCESS EVENT
		do
		{
			postProcessEvent(next_event);
			next_event = next_event->next;
		} while (next_event != 0 && next_event->timeDelta == blockPos_); // cur_timeStamp );

#if defined(_DEBUG)
		blockPosExact_ = false;
#endif
	}
}

void AudioPlugin::processEvent(const api::Event* e)
{
	switch (e->eventType)
	{
	// pin events redirect to pin
	case api::EventType::PinSet:
	case api::EventType::PinStreamingStart:
	case api::EventType::PinStreamingStop:
	case api::EventType::Midi:
	{
		if (auto it = pins_.find(e->pinIdx); it != pins_.end())
		{
			(*it).second->processEvent(e);
		}
	}
	break;

	case api::EventType::GraphStart:
		onGraphStart();
		assert(debugGraphStartCalled_ && "Please call the base class's MpBase2::onGraphStart(); from your overloaded member.");
		break;

	default:
		break;
	};
}

void AudioPlugin::preProcessEvent(const api::Event* e)
{
	switch (e->eventType)
	{
	case api::EventType::PinStreamingStart:
	case api::EventType::PinStreamingStop:
	case api::EventType::PinSet:
	case api::EventType::Midi:
	{
		// pin events redirect to pin
		if (auto it = pins_.find(e->pinIdx); it != pins_.end())
		{
			(*it).second->preProcessEvent(e);
		}
	}

	break;
	case api::EventType::GraphStart:
	default:
		break;
	};
}

void AudioPlugin::postProcessEvent(const api::Event* e)
{
	switch (e->eventType)
	{
	// pin events redirect to pin
	case api::EventType::PinSet:
	case api::EventType::PinStreamingStart:
	case api::EventType::PinStreamingStop:
	case api::EventType::Midi:
	{
		if (auto it = pins_.find(e->pinIdx); it != pins_.end())
		{
			(*it).second->postProcessEvent(e);
		}
	}
	break;

	case api::EventType::GraphStart:
	default:
		break;
	};
}

void AudioPlugin::midiHelper(const api::Event* e)
{
	assert(e->eventType == api::EventType::Midi);

	onMidiMessage(
		e->pinIdx
		, e->data()
		, e->size());
}

AudioPlugin::AudioPlugin() :
	blockPos_(0)
	, sleepCount_(0)
	, streamingPinCount_(0)
	, canSleepManualOverride_(SLEEP_AUTO)
	, eventsComplete_(true)
{
}

// specialised for audio pins_
float MpAudioPinBase::getValue(int bufferPos) const
{
	if (bufferPos < 0)
	{
		assert(plugin_ != nullptr && "err: This pin was not initialized.");
		assert(plugin_->blockPosExact_ && "err: Please use - pin.getValue( someBufferPosition );");
		bufferPos = plugin_->getBlockPosition();
	}

	assert(bufferPos < plugin_->host.getBlockSize() && "err: Don't access past end of buffer");

	return *(getBuffer() + bufferPos);
}

void MpPinBase::initialize(AudioPlugin* plugin, int PinId, MpBaseMemberPtr handler)
{
	assert(id_ == -1 && "pin initialized twice?"); // check your constructor's calls to initializePin() for duplicates.

	id_ = PinId;
	plugin_ = plugin;
	eventHandler_ = handler;

	if (eventHandler_ == 0 && getDirection() == PinDirection::In)
	{
		eventHandler_ = getDefaultEventHandler();
	}
}

void MpPinBase::processEvent(const api::Event* e)
{
	if (eventHandler_)
		(plugin_->*eventHandler_)(e);
}

// Pins
void MpPinBase::sendPinUpdate(int32_t rawSize, const void* rawData, int32_t blockPosition)
{
	assert(plugin_ != nullptr && "err: Please don't forgot to call initializePin(pinWhatever) in contructor.");
	assert(plugin_->debugIsOpen_ && "err: Please don't update output pins in constructor or open().");

	if (blockPosition == -1)
	{
		assert(plugin_->blockPosExact_ && "err: Please use - pin.setValue( value, someBufferPosition );");
		blockPosition = plugin_->getBlockPosition();
	}
	plugin_->host.setPin(blockPosition, getId(), rawSize, rawData);
}

MpBaseMemberPtr MidiInPin::getDefaultEventHandler()
{
	return &AudioPlugin::midiHelper;
}

void AudioPlugin::initializePin(int PinId, MpPinBase& pin, MpBaseMemberPtr handler)
{
	pin.initialize(this, PinId, handler);

	[[maybe_unused]] auto r = pins_.insert({ PinId, &pin });

	assert(r.second && "Did you initializePin() with the same index twice?");
}

ReturnCode AudioPlugin::setBuffer(int32_t pinId, float* buffer)
{
	if (auto it = pins_.find(pinId); it != pins_.end())
	{
		(*it).second->setBuffer(buffer);
		return ReturnCode::Ok;
	}

	return ReturnCode::Fail;
}

void MidiOutPin::send(const unsigned char* data, int size, int blockPosition)
{
	assert(blockPosition >= -1 && "MIDI Out pin can't use negative timestamps");
	if (blockPosition == -1)
	{
		assert(plugin_->blockPosExact_ && "err: Please use - midiPin.send( data, size, someBufferPosition );");
		blockPosition = plugin_->getBlockPosition();
	}
	plugin_->host.setPin(blockPosition, getId(), size, data);
}

void AudioOutPin::setStreaming(bool isStreaming, int blockPosition)
{
	assert(plugin_ && "Don't forget initializePin() in your DSP class constructor for each pin");
	assert(plugin_->debugIsOpen_ && "Can't setStreaming() until after module open().");

	if (blockPosition == -1)
	{
		assert(plugin_->blockPosExact_ && "err: Please use - pin.setStreaming( isStreaming, someBufferPosition );");
		blockPosition = plugin_->getBlockPosition();
	}

	assert(blockPosition >= 0 && blockPosition < plugin_->host.getBlockSize() && "block posistion must be within current block");

	// note if streaming has changed (not just repeated one-offs).
	if (isStreaming_ != isStreaming)
	{
		isStreaming_ = isStreaming;
		plugin_->OnPinStreamingChange(isStreaming);
	}

	// Always reset sleep counter on streaming stop or one-off level changes.
	// Do this regardless of what pins are streaming. Else any later change in input pin streaming could switch to sleep mode without counter set.
	if (!isStreaming_)
	{
		plugin_->resetSleepCounter();
	}

	plugin_->host.setPinStreaming(blockPosition, getId(), (int)isStreaming_);
};

void AudioPlugin::setSleep(bool isOkToSleep)
{
	if (isOkToSleep)
	{
		canSleepManualOverride_ = SLEEP_ENABLE;
		curSubProcess_ = &AudioPlugin::subProcessPreSleep;
	}
	else
	{
		canSleepManualOverride_ = SLEEP_DISABLE;
	}
}

// module is OK to sleep, but first needs to output enough samples to clear the output buffers.
void AudioPlugin::subProcessPreSleep(int sampleFrames)
{
	// this routine used whenever we MAYBY need to sleep.
	bool canSleep;
	int sampleFramesRemain = sampleFrames;
	int saveBlockPos = blockPos_;

	do {
		// if sleep mode no longer appropriate, perform normal processing and exit.
		if (canSleepManualOverride_ == SLEEP_AUTO)
		{
			canSleep = streamingPinCount_ == 0;
		}
		else
		{
			canSleep = canSleepManualOverride_ == SLEEP_ENABLE;
		}

		if (!canSleep)
		{
			curSubProcess_ = saveSubProcess_;

			if (sampleFramesRemain > 0)
			{
				(this->*(saveSubProcess_))(sampleFramesRemain);
			}
			blockPos_ = saveBlockPos; // restore bufferOffset_, else may be incremented twice.
			return;
		}

		if (sleepCount_ <= 0)
		{
			if (eventsComplete_)
			{
				host.sleep();
				blockPos_ = saveBlockPos; // restore bufferOffset_, else may be incremented twice.
				return;
			}

			// we have to proces more than expected, at least till the next event.
			sleepCount_ = sampleFramesRemain;
		}

		if (sampleFramesRemain == 0)
		{
			blockPos_ = saveBlockPos; // restore bufferOffset_, else may be incremented twice.
			return;
		}

		// only process minimum samples.
		// determin how many samples remain to be processed.
		int toDo = sampleFramesRemain;
		if (sleepCount_ < toDo)
		{
			toDo = sleepCount_;
		}

		sleepCount_ -= toDo; // must be before sub-processs in case sub-process resets it.
		sampleFramesRemain -= toDo;

		(this->*(saveSubProcess_))(toDo);

		blockPos_ += toDo; // !! Messey, may screw up main process loop if not restored on exit.

	} while (true);
}

void AudioPlugin::onGraphStart()	// called on very first sample.
{
	// Send initial update on all output pins.
	for (auto& it : pins_)
	{
		auto pin = it.second;
		if (pin->getDirection() == PinDirection::Out)
		{
			pin->sendFirstUpdate();
		}
	}

#if defined(_DEBUG)
	debugGraphStartCalled_ = true;
#endif
}

void AudioPlugin::OnPinStreamingChange(bool isStreaming)
{
	if (isStreaming)
	{
		++streamingPinCount_;
	}
	else
	{
		--streamingPinCount_;
	}

	if (streamingPinCount_ == 0 || canSleepManualOverride_ == SLEEP_ENABLE)
	{
		curSubProcess_ = &AudioPlugin::subProcessPreSleep;
	}
	else // no need for sleep mode.
	{
		curSubProcess_ = saveSubProcess_;
	}
}

void AudioPlugin::resetSleepCounter()
{
	sleepCount_ = host.getBlockSize();
}

void AudioInPin::preProcessEvent(const api::Event* e)
{
	switch (e->eventType)
	{
	case api::EventType::PinStreamingStart:
	case api::EventType::PinStreamingStop:
		freshValue_ = true;
		const bool isStreaming = e->eventType == api::EventType::PinStreamingStart;
		if (isStreaming_ != isStreaming)
		{
			isStreaming_ = isStreaming;
			plugin_->OnPinStreamingChange(isStreaming_);
		}

		// For modules with no output pins, it's important to process that one last sample after processing the event,
		// else the module will sleep immediatly without processing the sample associated with the event.
		if (!isStreaming_)
		{
			plugin_->nudgeSleepCounter();
		}
		break;
	};
}

ReturnCode AudioPluginHostWrapper::Init(api::IUnknown* phost)
{
	host = as<api::IAudioPluginHost>(phost);
	return host ? ReturnCode::Ok : ReturnCode::NoSupport;
}

api::IAudioPluginHost* AudioPluginHostWrapper::get()
{
	return host.get();
}

ReturnCode AudioPluginHostWrapper::setPin(int32_t timestamp, int32_t pinId, int32_t size, const void* data)
{
	return host->setPin(timestamp, pinId, size, data);
}

ReturnCode AudioPluginHostWrapper::setPinStreaming(int32_t timestamp, int32_t pinId, bool isStreaming)
{
	return host->setPinStreaming(timestamp, pinId, isStreaming);
}
ReturnCode AudioPluginHostWrapper::setLatency(int32_t latency)
{
	return host->setLatency(latency);
}
ReturnCode AudioPluginHostWrapper::sleep()
{
	return host->sleep();
}
int32_t AudioPluginHostWrapper::getBlockSize() const
{
	return host->getBlockSize();
}
int32_t AudioPluginHostWrapper::getSampleRate() const
{
	return host->getSampleRate();
}
int32_t AudioPluginHostWrapper::getHandle() const
{
	return host->getHandle();
}

} // namespace