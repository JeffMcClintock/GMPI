#pragma once
/*
#include "Hosting/controller_holder.h"

gmpi::hosting::gmpi_processor plugin;
*/

#include <unordered_map>
#include "GmpiApiAudio.h"
#include "GmpiSdkCommon.h"
#include "Hosting/xml_spec_reader.h"
//#include "Hosting/message_queues.h"

namespace gmpi
{
namespace hosting
{


class factory
{
public:
	gmpi::shared_ptr<gmpi::api::IPluginFactory> plugin_factory;
	std::vector<gmpi::hosting::pluginInfo> plugins;

	factory();
	static factory& getInstance()
	{
		static factory instance;
		return instance;
	}

	gmpi::hosting::pluginInfo* getPluginInfo(int index = 0);

//	gmpi::shared_ptr<gmpi::api::IUnknown> createInstance(gmpi::api::PluginSubtype sub_type);
	gmpi::shared_ptr<gmpi::api::IUnknown> createInstance(const char* plugin_id, gmpi::api::PluginSubtype sub_type);

};

} // namespace hosting
} // namespace gmpi
