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

void gmpi_processor::setPresetUnsafe(std::string& chunk)
{
	std::string name, category;

	tinyxml2::XMLDocument doc;
	doc.Parse(chunk.c_str());

	auto* parentXml = &doc;

	auto presetXml = parentXml->FirstChildElement("Preset");

	if (!presetXml)
		return;

	tinyxml2::XMLNode* parametersE{};
	parametersE = presetXml;

//	auto presetXmlElement = presetXml->ToElement();

	// Query plugin's 4-char code. Presence Indicates also that preset format supports MIDI learn.
	int32_t fourCC = -1;
	int formatVersion = 0;
	{
		const char* hexcode{};
		if (tinyxml2::XML_SUCCESS == presetXml->QueryStringAttribute("pluginId", &hexcode))
		{
			formatVersion = 1;
			try
			{
				fourCC = std::stoul(hexcode, nullptr, 16);
			}
			catch (...)
			{
				// who gives a f*ck
			}
		}
	}

	// !!! TODO: Check 4-char ID correct.

	const char* temp{};
	if (tinyxml2::XML_SUCCESS == presetXml->QueryStringAttribute("category", &temp))
	{
		category = temp;
	}
	//presetXmlElement->QueryStringAttribute("name", &name);
	if (tinyxml2::XML_SUCCESS == presetXml->QueryStringAttribute("name", &temp))
	{
		name = temp;
	}

	std::unordered_set<int32_t> parametersInPreset;

	// assuming we are passed "Preset.Parameters" node.
	for (auto ParamElement = presetXml->FirstChildElement("Param"); ParamElement; ParamElement = ParamElement->NextSiblingElement("Param"))
	{
		int paramHandle = -1;
		ParamElement->QueryIntAttribute("id", &paramHandle);

		assert(paramHandle != -1);

		// ignore unrecognised parameters
		auto it = patchManager.parameters.find(paramHandle);
		if (it == patchManager.parameters.end())
			continue;

		auto& param = (*it).second;

		parametersInPreset.insert(paramHandle);

		//??			if (info.ignoreProgramChange)
		//				continue;

		//auto& values = params[paramHandle];
		/* no stateful parameters in preset we assume
					if (!parameter.stateful_) // For VST2 wrapper aeffect ptr. prevents it being inadvertently zeroed.
						continue;

					// possibly need to handle elsewhere
					if (parameter.ignorePc_ && ignoreProgramChange) // a short time period after plugin loads, ignore-PC parameters will no longer overwrite existing value.
						continue;

		*/
		auto v = ParamElement->Attribute("val"); // todo instead of constructing a pointless string, what about just passing the char* to ParseToRaw()?

		if(!v)
			continue;

		//		values.dataType = param.info->datatype;
		//		values.rawValues_.push_back(ParseToRaw(param.dataType, v));

		const double value = std::stod(v);
		param.setReal(value);

		// This block seems messy. Should updating a parameter be a single function call?
					// (would need to pass 'updateProcessor')
		{
			// calls controller_->updateGuis(this, voice)
//				parameter->setParameterRaw(gmpi::Field::Value, (int32_t)raw.size(), raw.data(), voiceId);

			/* todo elsewhere
			// updated cached value.
			parameter->upDateImmediateValue();
			*/
		}

		// MIDI learn. part of a preset?????
		if (formatVersion > 0)
		{
			// TODO
			int32_t midiController = -1;
			ParamElement->QueryIntAttribute("MIDI", &midiController);

			const char* sysexU{};
			ParamElement->QueryStringAttribute("MIDI_SYSEX", &sysexU);
		}
	}

	// set any parameters missing from the preset to their default
	// except for preset name and category
	for (auto& [paramHandle, param] : patchManager.parameters)
	{
#if 0
		if (info.hostControl == HC_PROGRAM_NAME || HC_PROGRAM_CATEGORY == info.hostControl)
			continue;
#endif

		if (parametersInPreset.find(paramHandle) == parametersInPreset.end())
		{
			//			auto& values = params[paramHandle];

			//			values.dataType = info.dataType;
			//			for (const auto& v : info.defaultRaw)
			{
				//				values.rawValues_.push_back(v);

				param.setToDefault();
			}
		}
	}
}

std::string gmpi_processor::getPresetUnsafe()
{
	tinyxml2::XMLDocument doc;
	// TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "", "");
	doc.LinkEndChild(doc.NewDeclaration());

	auto element = doc.NewElement("Preset");
	doc.LinkEndChild(element);

#if 0 // TODO
	{
		char txt[20];
#if defined(_MSC_VER)
		sprintf_s(txt, "%08x", pluginId);
#else
		snprintf(txt, std::size(txt), "%08x", pluginId);
#endif
		element->SetAttribute("pluginId", txt);
	}

	if (!presetNameOverride.empty())
	{
		element->SetAttribute("name", presetNameOverride.c_str());
	}
	else if (!name.empty())
	{
		element->SetAttribute("name", name.c_str());
	}

	if (!category.empty())
		element->SetAttribute("category", category.c_str());
#endif

	for (auto& [handle, parameter] : patchManager.parameters)
	{
		auto paramElement = doc.NewElement("Param");
		element->LinkEndChild(paramElement);
		paramElement->SetAttribute("id", handle);

		//const int voice = 0;
		//auto& raw = parameter.rawValues_[voice];

		//const auto val = RawToUtf8B(parameter.dataType, raw.data(), raw.size());

		paramElement->SetAttribute("val", parameter.valueReal());

#if 0  // TODO??
		// MIDI learn.
		if (parameter->MidiAutomation != -1)
		{
			paramElement->SetAttribute("MIDI", parameter->MidiAutomation);

			if (!parameter->MidiAutomationSysex.empty())
				paramElement->SetAttribute("MIDI_SYSEX", WStringToUtf8(parameter->MidiAutomationSysex));
		}
#endif
	}

	tinyxml2::XMLPrinter printer;
	// printer.SetIndent(" ");
	doc.Accept(&printer);

	return printer.CStr();
}

// IAudioPluginHost
// 
// data from Processor to host (eg MIDI out).
//gmpi::ReturnCode gmpi_processor::setPin(int32_t timestamp, int32_t pinId, int32_t size, const uint8_t* data)
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
			case gmpi::PinDatatype::Blob:
			{
				if (param->setBlob({ data, (size_t) size }))
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

gmpi::ReturnCode gmpi_processor::setPinStreaming(int32_t timestamp, int32_t pinId, bool isStreaming)
{
	return gmpi::ReturnCode::Ok;
}

gmpi::ReturnCode gmpi_processor::setLatency(int32_t latency)
{
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
