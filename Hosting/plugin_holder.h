#pragma once
/*
#include "Hosting/plugin_holder.h"

gmpi::hosting::gmpi_processor plugin;
*/

//#include "GmpiApiCommon.h"
#include "GmpiApiAudio.h"
#include "GmpiSdkCommon.h"
#include "Hosting/xml_spec_reader.h"

namespace gmpi
{
namespace hosting
{

class EventQue
{
	std::vector<gmpi::api::Event> events;

public:
	EventQue(size_t capacity = 1000)
	{
		events.reserve(capacity);
	}

	void push(gmpi::api::Event event)
	{
		// insert in sorted order.
		auto it = std::lower_bound(events.begin(), events.end(), event, [](const gmpi::api::Event& a, const gmpi::api::Event& b)
			{ return a.timeDelta < b.timeDelta;	}
		);
		events.insert(it, event);
	}

	gmpi::api::Event* head()
	{
		if (events.empty())
			return {};

		// create linked list.
		for (int i = 1; i < events.size(); ++i)
		{
			events[i - 1].next = &events[i];
		}

		events.back().next = {};

		return &events[0];
	}

	void clear()
	{
		events.clear();
	}
};

struct gmpi_processor
{
	gmpi::shared_ptr<gmpi::api::IProcessor> processor;
	EventQue events;

	bool start_processor(gmpi::api::IProcessorHost* host, gmpi::hosting::pluginInfo const& info);
};

//gmpi::shared_ptr<gmpi::api::IProcessor> gmpi_instansiate_processor();

} // namespace hosting
} // namespace gmpi
