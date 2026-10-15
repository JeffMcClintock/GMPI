#include "Hosting/processor_holder.h"

#include <string>
#include <vector>
#include <algorithm>
#include <optional>
#include <unordered_set>

//#include "GmpiApiCommon.h"
//#include "GmpiApiEditor.h"
//#include "GmpiSdkCommon.h"
#include "Hosting/message_queues.h"
#include "tinyXml2/tinyxml2.h"

extern "C"
gmpi::ReturnCode MP_GetFactory(void** returnInterface);

namespace gmpi
{
namespace hosting
{

template<typename T>
void copyValueToEvent(gmpi::api::Event& e, T value)
{
	const auto src = reinterpret_cast<const uint8_t*>(&value);
	e.size_ = static_cast<int32_t>(sizeof(value));
	std::copy(src, src + sizeof(value), e.data_);
}

void gmpi_processor::init(gmpi::hosting::pluginInfo const& info)
{
    // init PatchManager parameters
    patchManager.init(info);
    
    // create a list of native params. the index must line up strictly with the parameter DAW tag
    nativeParams.clear();
    for (auto& paramInfo : info.parameters)
    {
        if (paramInfo.dawTag > -1)
        {
            assert(paramInfo.dawTag == nativeParams.size());
            nativeParams.push_back(&patchManager.parameters[paramInfo.id]);
        }
    }
}

bool gmpi_processor::start_processor(gmpi::api::IProcessorHost* host, gmpi::hosting::pluginInfo const& pinfo, int32_t pblockSize, float psampleRate)
{
	info = &pinfo;
	blockSize = pblockSize;
	sampleRate = psampleRate;
    
	events.clear();
	processor = {};
	latency.store(0, std::memory_order_relaxed); // a new instance that never reports has none

	gmpi::shared_ptr<gmpi::api::IUnknown> factoryBase;
	auto r = MP_GetFactory(factoryBase.put_void());

	gmpi::shared_ptr<gmpi::api::IPluginFactory> factory;
	auto r2 = factoryBase->queryInterface(&gmpi::api::IPluginFactory::guid, factory.put_void());

	if (!factory || r != gmpi::ReturnCode::Ok)
	{
		return false;
	}

	gmpi::shared_ptr<gmpi::api::IUnknown> pluginUnknown;
	r2 = factory->createInstance(info->id.c_str(), gmpi::api::PluginSubtype::Audio, pluginUnknown.put_void());
	if (!pluginUnknown || r != gmpi::ReturnCode::Ok)
	{
		return false;
	}

	processor = pluginUnknown.as<gmpi::api::IProcessor>();

	if(!processor)
	{
		return false;
	}

	processor->open(host);

	events.push(
		{
			{},            // next (populated later)
			0,             // timeDelta
			gmpi::api::EventType::GraphStart,
			{},            // pinIdx
			{},            // size_
			{}             // data_/oversizeData_
		}
	);

	// initialise silence flags.
	for (auto& pin : info->dspPins)
	{
		if (pin.datatype != gmpi::PinDatatype::Audio || pin.direction != gmpi::PinDirection::In)
			continue;

		events.push(
			{
				{},            // next (populated later)
				0,             // timeDelta
				gmpi::api::EventType::PinStreamingStart,
				pin.id,        // pinIdx
				{},            // size_
				{}             // data_/oversizeData_
			}
		);
	}

	// initialise pins.
	{
		gmpi::api::Event e
		{
			{},            // next (populated later)
			0,				// timeDelta
			gmpi::api::EventType::PinSet,
			0,				// pinIdx
			0,             // size_
			{}             // data_/oversizeData_
		};

		MidiInputPinIdx = -1;

		for (auto& pin : info->dspPins)
		{
			if (pin.datatype == gmpi::PinDatatype::Midi && pin.direction == gmpi::PinDirection::In && -1 == MidiInputPinIdx)
			{
				MidiInputPinIdx = pin.id;
			}

			if (pin.direction == gmpi::PinDirection::Out)
				continue;

			e.pinIdx = pin.id;

			if (pin.parameterId == -1) // non-parameter pins. Just set to thier default.
			{
				const auto def = atof(pin.default_value.c_str());
				bool hasDefault = true;

				switch (pin.datatype)
				{
				case gmpi::PinDatatype::Float32:
				{
					copyValueToEvent(e, static_cast<float>(def));
				}
				break;

				case gmpi::PinDatatype::Int32:
				case gmpi::PinDatatype::Enum:
				{
					copyValueToEvent(e, static_cast<int32_t>(std::round(def)));
				}
				break;

				case gmpi::PinDatatype::Bool:
				{
					copyValueToEvent(e, static_cast<bool>(std::round(def)));
				}
				break;

				case gmpi::PinDatatype::Midi: // Audio/MIDI pins can't have default in most plugin APIs.
				case gmpi::PinDatatype::Audio:
					hasDefault = false;
					break;

				default:
					hasDefault = false;
					assert(false); // unsupported type.
				}

				if(hasDefault)
					events.push(e);
			}
			else
			{
				auto param = patchManager.getParameter(pin.parameterId);
				if (!param)
					continue;

				switch (pin.parameterFieldType)
				{
				case gmpi::Field::Normalized:
				{
					copyValueToEvent(e, static_cast<float>(param->normalisedValue()));
					events.push(e);
				}
				break;

				case gmpi::Field::Value:
				{
					switch (pin.datatype)
					{
					case gmpi::PinDatatype::Float32:
					{
						copyValueToEvent(e, static_cast<float>(param->valueReal()));
					}
					break;

					case gmpi::PinDatatype::Int32:
					{
						copyValueToEvent(e, static_cast<int32_t>(std::round(param->valueReal())));
					}
					break;

					case gmpi::PinDatatype::Bool:
					{
						copyValueToEvent(e, static_cast<bool>(std::round(param->valueReal())));
					}
					break;

					case gmpi::PinDatatype::Blob:
					{
						// Seed the pin with the parameter's CURRENT bytes, not a
						// default. A processor can be created at any time - after
						// restartComponent, for offline rendering, or on state
						// restore - and without this it would start with an empty
						// blob and never be told otherwise, since blobs only reach
						// it when they CHANGE. The parameter owns the storage and
						// outlives this event queue.
						auto* v = std::get_if<std::vector<uint8_t>>(&param->value_);
						if (!v || v->empty())
							continue; // nothing stored yet; a later change will deliver it

						e.size_ = static_cast<int32_t>(v->size());
						if (v->size() > 8)
						{
							e.oversizeData_ = v->data();
						}
						else
						{
							std::fill(std::begin(e.data_), std::end(e.data_), uint8_t{});
							std::copy(v->begin(), v->end(), e.data_);
						}
					}
					break;

					default:
						assert(false); // unsupported type.
					}
					events.push(e);
				}
				break;

				default:
					assert(false); // unsupported
					break;
				}
			}
		}
	}

	return true;
}

void gmpi_processor::setParameterNormalizedFromDaw(gmpi::hosting::pluginInfo const& info, int sampleOffset, int id, double valueNormalized, bool sendToEditor)
{
	if (auto param = patchManager.setParameterNormalised(id, valueNormalized); param)
	{
		sendParameterToProcessor(info, param, sampleOffset);

		// For CLAP queue it for the GUI too (drained to the dsp-to-ui queue after process()),
		// so DAW automation is reflected in the editor.
		if(sendToEditor)
			pendingControllerQueueClients.AddWaiter(param);
	}
}

void gmpi_processor::setHostControlFromDaw(gmpi::hosting::HostControls hc, double value)
{
    const auto id = -2 - (int)hc;

    if (auto param = patchManager.setParameterReal(id, value); param)
    {
        // Audio thread, before process(): deliver at offset 0 like a "ppc2" change. On the first block this
        // replaces start_processor's seed (same pin, same time), so the DSP never starts on a stale value.
        if (info)
            sendParameterToProcessor(*info, param, 0);

        pendingControllerQueueClients.AddWaiter(param);
    }
}

void gmpi_processor::sendParameterToProcessor(gmpi::hosting::pluginInfo const& info, GmpiParameter* param, int sampleOffset)
{
	gmpi::api::Event e
	{
		{},            // next (populated later)
		sampleOffset,  // timeDelta
		gmpi::api::EventType::PinSet,
		0,				// pinIdx
		0,             // size_
		{}             // data_/oversizeData_
	};

	for (auto& pin : info.dspPins)
	{
		if (pin.parameterId == param->info->id && pin.direction == gmpi::PinDirection::In)
		{
			e.pinIdx = pin.id;

			switch (pin.parameterFieldType)
			{
			case gmpi::Field::Normalized:
			{
				copyValueToEvent(e, static_cast<float>(param->normalisedValue()));
				events.push(e);
			}
			break;

			case gmpi::Field::Value:
			{
				switch (pin.datatype)
				{
				case gmpi::PinDatatype::Float32:
				{
					copyValueToEvent(e, static_cast<float>(param->valueReal()));
				}
				break;

				case gmpi::PinDatatype::Int32:
				{
					copyValueToEvent(e, static_cast<int32_t>(std::round(param->valueReal())));
				}
				break;

			case gmpi::PinDatatype::Bool:
			{
				copyValueToEvent(e, static_cast<bool>(std::round(param->valueReal())));
			}
			break;

			case gmpi::PinDatatype::Blob:
			{
				// The parameter owns the bytes; the event points into that
				// storage for payloads over the 8 inline bytes. Safe because a
				// later update to this pin replaces this event (EventQue::push)
				// before the drain, so a moved vector is never read.
				const auto& v = std::get<std::vector<uint8_t>>(param->value_);
				e.size_ = static_cast<int32_t>(v.size());
				if (v.size() > 8)
				{
					e.oversizeData_ = v.data();
				}
				else
				{
					std::fill(std::begin(e.data_), std::end(e.data_), uint8_t{});
					std::copy(v.begin(), v.end(), e.data_);
				}
			}
			break;
			default:
				assert(false); // unsupported type.
			}
			events.push(e);
		}
		break;

		default:
			assert(false); // unsupported
			break;
		}
		}
	}
}

bool gmpi_processor::onQueMessageReady(int handle, int msg_id, gmpi::hosting::my_input_stream& strm)
{
	switch (msg_id)
	{
	case id_to_long("ppc2"): // "ppc2" Patch parameter change, always sent as a double
	{
		double val{};
		strm >> val;

		if (auto param = patchManager.setParameterReal(handle, val); param)
		{
			constexpr int sampleOffset{};
			sendParameterToProcessor(*info, param, sampleOffset);
		}

		return true;
	}
	break;

	case id_to_long("ppc3"): // Patch parameter change, blob payload. See
	                         // GmpiParameter::getQueMessage for the framing and
	                         // gmpi_controller_holder::setParameter for the sender.
	{
		int32_t size{};
		strm >> size;

		blobScratch.resize(static_cast<size_t>(size)); // NO!!!!! setBlob to take stream directly, or at least a span, so we don't have to allocate a vector on every message.
		if (size > 0)
			strm.Read(blobScratch.data(), static_cast<unsigned int>(size));

		if (auto* param = patchManager.getParameter(handle); param)
		{
			if (param->setBlob(blobScratch))
			{
				constexpr int sampleOffset{};
				sendParameterToProcessor(*info, param, sampleOffset);
			}
		}

		return true;
	}
	break;
	};

	return false; // failed to consume message.
}

void gmpi_processor::readPreset(PatchManager& store, const std::string& chunk)
{
	// One reader for both stores: readPresetXml (controller_holder.cpp), the inverse of
	// writePresetXml, which the controller's setPresetXmlFromDaw also calls. This used to be a
	// second copy of that loop, and the two had drifted (see the reader's comment).
	readPresetXml(store.parameters, chunk);
}

void gmpi_processor::setPresetUnsafe(std::string& chunk)
{
	readPreset(patchManager, chunk);
}

std::string gmpi_processor::getPresetUnsafe()
{
	// The whole body moved to writePresetXml (controller_holder.cpp), which the
	// controller's store now shares. The two used to be copies of one loop and
	// had drifted: the blob/string arms and the non-stateful filter were added
	// here and never there, so which half of the plug-in a host asked decided
	// whether a blob survived.
	//
	// "Unsafe" still means what it always did - patchManager is the PROCESSOR's
	// store, written from the audio thread by pollMessage, so reading it is only
	// safe while no audio callback can run.
	return writePresetXml(patchManager.parameters);
}

// IAudioPluginHost
// 
// data from Processor to host (eg MIDI out).
//gmpi::ReturnCode gmpi_processor::setPin(int32_t timestamp, int32_t pinId, int32_t size, const uint8_t* data)
gmpi::ReturnCode gmpi_processor::setPin(int32_t timestamp, int32_t pinId, int32_t size, const uint8_t* data)
{
	int32_t midiOutPort = 0;
	for (auto& pin : info->dspPins)
	{
		if (pin.datatype == gmpi::PinDatatype::Midi && pin.direction == gmpi::PinDirection::Out)
		{
			if (pinId == pin.id)
			{
				if (midiOutputSink && size > 0 && data)
					midiOutputSink->onMidiOut(midiOutPort, timestamp, { data, static_cast<size_t>(size) });

				return gmpi::ReturnCode::Ok;
			}
			++midiOutPort;
			continue;
		}

		// only output parameter pins.
		if (pinId != pin.id || pin.direction != gmpi::PinDirection::Out || pin.parameterId == -1)
			continue;

		auto param = patchManager.getParameter(pin.parameterId);
		if (!param)
			continue;

		switch (pin.parameterFieldType)
		{
		case gmpi::Field::Normalized:
		{
			assert(size == sizeof(float));

			if (param->setNormalised(static_cast<double>(*reinterpret_cast<const float*>(data))))
				pendingControllerQueueClients.AddWaiter(param);
		}
		break;

		case gmpi::Field::Value:
		{
			switch (pin.datatype)
			{
			case gmpi::PinDatatype::Float32:
			{
				if (param->setReal(static_cast<double>(*reinterpret_cast<const float*>(data))))
					pendingControllerQueueClients.AddWaiter(param);
			}
			break;
			case gmpi::PinDatatype::Int32:
			{
				if (param->setReal(static_cast<double>(*reinterpret_cast<const int32_t*>(data))))
					pendingControllerQueueClients.AddWaiter(param);
			}
			break;
			case gmpi::PinDatatype::Bool:
			{
				if (param->setReal(static_cast<double>(*reinterpret_cast<const bool*>(data))))
					pendingControllerQueueClients.AddWaiter(param);
			}
			break;
			case gmpi::PinDatatype::Blob:
			{
				// UNCONDITIONAL, unlike the scalar cases above: a blob output
				// parameter is a STREAM, not a value. Scope captures and
				// TIDE's rack-feedback channel send each update explicitly
				// (sendPinUpdate), and a payload that happens to be
				// byte-identical to the previous one is still a send - with
				// the change-compare here, a value the DSP wrote back to its
				// previous state was silently never shipped, and the compare
				// itself is a memcmp of the whole blob per block for nothing.
				param->setBlob({ data, (size_t) size });
				pendingControllerQueueClients.AddWaiter(param);
			}
			break;
			default:
				assert(false); // unsupported type.
			}
		}
		break;

		default:
			assert(false); // unsupported
			break;
		}
	}

	return gmpi::ReturnCode::Ok;
}

gmpi::ReturnCode gmpi_processor::setPinStreaming(int32_t timestamp, int32_t pinId, bool isStreaming)
{
	return gmpi::ReturnCode::Ok;
}

gmpi::ReturnCode gmpi_processor::setLatency(int32_t samples)
{
	latency.store((std::max)(samples, 0), std::memory_order_relaxed);
	return gmpi::ReturnCode::Ok;
}

gmpi::ReturnCode gmpi_processor::sleep()
{
	return gmpi::ReturnCode::Ok;
}

int32_t gmpi_processor::getBlockSize()
{
	return blockSize;
}

float gmpi_processor::getSampleRate()
{
	return sampleRate;
}

int32_t gmpi_processor::getHandle()
{
	return 0; // only one plugin, can have handle zero.
}

} // namespace hosting
} // namespace gmpi
