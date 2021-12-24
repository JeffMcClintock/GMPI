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
*/// MpBase2 - implements the IMpPlugin2 interface.
#if !defined(_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif

#include "mp_sdk_audio.h"
#include <assert.h>
#include <codecvt>
#include <locale>

using namespace gmpi;

namespace GmpiSdk
{
	int32_t AudioPlugin::open()
	{
#if defined(_DEBUG)
		debugIsOpen_ = true;
#endif

		return gmpi::MP_OK;
	}

	void AudioPlugin::process(int32_t count, const MpEvent* events)
	{
		assert(count > 0);

#if defined(_DEBUG)
		blockPosExact_ = false;
#endif

		blockPos_ = 0;
		int remain = count;
		const MpEvent* next_event = events;

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
			const MpEvent* e = next_event;
			do
			{
				preProcessEvent(e); // updates all pins_ values
				pins_set_flag = pins_set_flag || e->eventType == EVENT_PIN_SET || e->eventType == EVENT_PIN_STREAMING_START || e->eventType == EVENT_PIN_STREAMING_STOP;
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

	void AudioPlugin::processEvent(const MpEvent* e)
	{
		switch (e->eventType)
		{
			// pin events redirect to pin
		case EVENT_PIN_SET:
		case EVENT_PIN_STREAMING_START:
		case EVENT_PIN_STREAMING_STOP:
		case EVENT_MIDI:
		{
			auto it = pins_.find(e->parm1);
			if (it != pins_.end())
			{
				(*it).second->processEvent(e);
			}
		}
		break;

		case EVENT_GRAPH_START:
			onGraphStart();
			assert(debugGraphStartCalled_ && "Please call the base class's MpBase2::onGraphStart(); from your overloaded member.");
			break;

		default:
			break;
		};
	}

	void AudioPlugin::preProcessEvent(const MpEvent* e)
	{
		switch (e->eventType)
		{
		case EVENT_PIN_STREAMING_START:
		case EVENT_PIN_STREAMING_STOP:
		case EVENT_PIN_SET:
		case EVENT_MIDI:
		{
			// pin events redirect to pin
			auto it = pins_.find(e->parm1);
			if (it != pins_.end())
			{
				(*it).second->preProcessEvent(e);
			}
		}

		break;
		case EVENT_GRAPH_START:
		default:
			break;
		};
	}

	void AudioPlugin::postProcessEvent(const MpEvent* e)
	{
		switch (e->eventType)
		{
		// pin events redirect to pin
		case EVENT_PIN_SET:
		case EVENT_PIN_STREAMING_START:
		case EVENT_PIN_STREAMING_STOP:
		case EVENT_MIDI:
		{
			auto it = pins_.find(e->parm1);
			if (it != pins_.end())
			{
				(*it).second->postProcessEvent(e);
			}
		}
		break;

		case EVENT_GRAPH_START:
		default:
			break;
		};
	}

	void AudioPlugin::midiHelper(const MpEvent* e)
	{
		assert(e->eventType == EVENT_MIDI);

		if (e->extraData == 0) // short msg
		{
			onMidiMessage(e->parm1 // pin
				, (const unsigned char*)&(e->parm3), e->parm2); // midi bytes (short msg)
		}
		else
		{
			onMidiMessage(e->parm1 // pin
				, (const unsigned char*)e->extraData, e->parm2); // midi bytes (sysex)
		}
	}

	AudioPlugin::AudioPlugin() :
		blockPos_(0)
		, sleepCount_(0)
		, streamingPinCount_(0)
		, canSleepManualOverride_(SLEEP_AUTO)
		, eventsComplete_(true)
	{
	}

	int32_t AudioPlugin::setHost(gmpi::IMpUnknown* phost)
	{
		return host.Init(phost);
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

		if (eventHandler_ == 0 && getDirection() == MP_IN)
		{
			eventHandler_ = getDefaultEventHandler();
		}
	}

	void MpPinBase::processEvent(const MpEvent* e)
	{
		if (eventHandler_)
			(plugin_->*eventHandler_)(e);
	}

	// Pins
	void MpPinBase::sendPinUpdate(int32_t rawSize, void* rawData, int32_t blockPosition)
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

	StringInPin::operator std::string()
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t> > convert;
		return convert.to_bytes(getValue());
	}

	void AudioPlugin::initializePin(int PinId, MpPinBase& pin, MpBaseMemberPtr handler)
	{
		pin.initialize(this, PinId, handler);

		[[maybe_unused]] auto r = pins_.insert(std::pair<int, MpPinBase*>(PinId, &pin));

		assert(r.second && "Did you initializePin() with the same index twice?");
	}

	int32_t AudioPlugin::setBuffer(int32_t pinId, float* buffer)
	{
		auto it = pins_.find(pinId);
		if (it != pins_.end())
		{
			(*it).second->setBuffer(buffer);
			return gmpi::MP_OK;
		}

		return MP_FAIL;
	}

	//int32_t MpBase2::receiveMessageFromGui(int32_t id, int32_t size, const void* messageData)
	//{
	//	return MP_OK;
	//}

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
			if (pin->getDirection() == MP_OUT)
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

	void AudioInPin::preProcessEvent(const gmpi::MpEvent* e)
	{
		switch (e->eventType)
		{
		case gmpi::EVENT_PIN_STREAMING_START:
		case gmpi::EVENT_PIN_STREAMING_STOP:
			freshValue_ = true;
			const bool isStreaming = e->eventType == gmpi::EVENT_PIN_STREAMING_START;
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
} // namespace