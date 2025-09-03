#pragma once
#include "Hosting/plugin_holder.h"

#include <string>
#include <vector>
#include <algorithm>
#include <optional>
//#include "GmpiApiCommon.h"
//#include "GmpiApiEditor.h"
//#include "GmpiSdkCommon.h"

extern "C"
gmpi::ReturnCode MP_GetFactory(void** returnInterface);

namespace gmpi
{
namespace hosting
{

bool gmpi_processor::start_processor(gmpi::api::IProcessorHost* host, gmpi::hosting::pluginInfo const& info)
{
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
	r2 = factory->createInstance(info.id.c_str(), gmpi::api::PluginSubtype::Audio, pluginUnknown.put_void());
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
	for (auto& pin : info.dspPins)
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

	return true;
}

} // namespace hosting
} // namespace gmpi
