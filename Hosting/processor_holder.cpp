#include "Hosting/processor_holder.h"

#include <string>
#include <vector>
#include <algorithm>
#include <optional>
//#include "GmpiApiCommon.h"
//#include "GmpiApiEditor.h"
//#include "GmpiSdkCommon.h"
#include "Hosting/message_queues.h"

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

bool gmpi_processor::start_processor(gmpi::api::IProcessorHost* host, gmpi::hosting::pluginInfo const& pinfo)
{
	info = &pinfo;
	events.clear();
	processor = {};

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

	// initialise parameter pinss.
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

			if (pin.direction == gmpi::PinDirection::Out || pin.parameterId == -1)
				continue;

			auto param = patchManager.getParameter(pin.parameterId);
			if (!param)
				continue;

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
						copyValueToEvent(e, static_cast<float>(param->valueReal));
					}
					break;

					case gmpi::PinDatatype::Int32:
					{
						copyValueToEvent(e, static_cast<int32_t>(std::round(param->valueReal)));
					}
					break;

					case gmpi::PinDatatype::Bool:
					{
						copyValueToEvent(e, static_cast<bool>(std::round(param->valueReal)));
					}
					break;
					default:
						assert(false); // unsupported type.
					}
					events.push(e);
				}
				break;
				}
			}
		}
	}

	return true;
}

void gmpi_processor::setParameterNormalizedFromDaw(gmpi::hosting::pluginInfo const& info, int sampleOffset, int id, double valueNormalized)
{
	if (auto param = patchManager.setParameterNormalised(id, valueNormalized); param)
	{
		sendParameterToProcessor(info, param, sampleOffset);
	}
}

void gmpi_processor::setHostControlFromDaw(gmpi::hosting::HostControls hc, double value)
{
    const auto id = -2 - (int)hc;

    if (auto param = patchManager.setParameterReal(id, value); param)
    {
        pendingControllerQueueClients.AddWaiter(param);
    }
}

void gmpi_processor::sendParameterToProcessor(gmpi::hosting::pluginInfo const& info, DawParameter* param, int sampleOffset)
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
		if (pin.parameterId == param->id && pin.direction == gmpi::PinDirection::In)
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
					copyValueToEvent(e, static_cast<float>(param->valueReal));
				}
				break;

				case gmpi::PinDatatype::Int32:
				{
					copyValueToEvent(e, static_cast<int32_t>(std::round(param->valueReal)));
				}
				break;

				case gmpi::PinDatatype::Bool:
				{
					copyValueToEvent(e, static_cast<bool>(std::round(param->valueReal)));
				}
				break;
				default:
					assert(false); // unsupported type.
				}
				events.push(e);
				break;
			}
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
	};

	return false; // failed to consume message.
}

// data from Processor to host (eg MIDI out).
gmpi::ReturnCode gmpi_processor::setPin(int32_t timestamp, int32_t pinId, int32_t size, const uint8_t* data)
{
	for (auto& pin : info->dspPins)
	{
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
			default:
				assert(false); // unsupported type.
			}
		}
		break;
		}
	}

	return gmpi::ReturnCode::Ok;
}


} // namespace hosting
} // namespace gmpi
