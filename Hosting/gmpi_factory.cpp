#include "Hosting/gmpi_factory.h"

#include <string>
#include <vector>
#include <algorithm>
#include <optional>

extern "C"
gmpi::ReturnCode MP_GetFactory(void** returnInterface); // assuming a statically linked factory

namespace gmpi
{
namespace hosting
{

factory::factory()
{
	//auto r = dll_entry_point(factoryBase.asIMpUnknownPtr());

	gmpi::shared_ptr<gmpi::api::IUnknown> factoryBase;
	auto r = MP_GetFactory(factoryBase.put_void());

//	gmpi::shared_ptr<gmpi::api::IPluginFactory> factory;
	auto r2 = factoryBase->queryInterface(&gmpi::api::IPluginFactory::guid, plugin_factory.put_void());

	if (!plugin_factory || r != gmpi::ReturnCode::Ok)
	{
		return;
	}

	int index = 0;
	while (true)
	{
		gmpi::ReturnString xml;
		const auto r = plugin_factory->getPluginInformation(index++, &xml); // FULL XML

		if (r != gmpi::ReturnCode::Ok)
			break;

		// RegisterXml(pluginPath, s.c_str());

		gmpi::hosting::readpluginXml(xml.c_str(), plugins);
	}
}

gmpi::hosting::pluginInfo* factory::getPluginInfo(int index)
{
	if (index < 0 || index >= static_cast<int>(plugins.size()))
	{
		return nullptr;
	}
	return &plugins[index];
}

// return the first plugin
//gmpi::shared_ptr<gmpi::api::IUnknown> factory::createInstance(gmpi::api::PluginSubtype sub_type)
//{
//	if(plugins.empty())
//	{
//		return {};
//	}
//
//	return createInstance(plugins[0].id.c_str(), sub_type);
//}

// return a specific plugin, if there are more than one.
gmpi::shared_ptr<gmpi::api::IUnknown> factory::createInstance(const char* plugin_id, gmpi::api::PluginSubtype sub_type)
{
	gmpi::shared_ptr<gmpi::api::IUnknown> pluginUnknown;
	auto r = plugin_factory->createInstance(plugin_id, sub_type, pluginUnknown.put_void());
	if (!pluginUnknown || r != gmpi::ReturnCode::Ok)
	{
		return {};
	}

	return pluginUnknown;
}

} // namespace hosting
} // namespace gmpi
