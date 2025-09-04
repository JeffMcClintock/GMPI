#pragma once
/*
#include "Hosting/processor_holder.h"

gmpi::hosting::gmpi_processor plugin;
*/

#include <unordered_map>
#include "GmpiApiAudio.h"
#include "GmpiSdkCommon.h"
#include "Hosting/xml_spec_reader.h"
#include "Hosting/message_queues.h"

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

struct DawParameter : public QueClient // also host-controls, might need to rename it.
{
	int32_t id{};
	double valueReal = 0.0;
	double valueLo = 0.0;
	double valueHi = 1.0;

	bool setNormalised(double value)
	{
		const auto newValueReal = valueLo + value * (valueHi - valueLo);

		const bool r = newValueReal != valueReal;

		valueReal = newValueReal;

		return r;
	}

	bool setReal(double value)
	{
		const bool r = value != valueReal;

		valueReal = value;

		return r;
	}

	double normalisedValue() const
	{
		if (valueHi == valueLo)
			return 0.0; // avoid divide by zero.
		return (valueReal - valueLo) / (valueHi - valueLo);
	}

	int queryQueMessageLength(int availableBytes) override
	{
		return sizeof(double);
	}

	void getQueMessage(class my_output_stream& outStream, int messageLength) override
	{
		const bool hostNeedsParameterUpdate{};
		const int32_t voice{};

		outStream << id;
		outStream << id_to_long("ppc2");
		outStream << messageLength;

		outStream << valueReal;
	}
};

class PatchManager
{
public:
	std::unordered_map<int, DawParameter> parameters;

	PatchManager() = default;

	void init(gmpi::hosting::pluginInfo const& info)
	{
		for (const auto& param : info.parameters)
		{
			assert(param.id != -1 || param.hostConnect != gmpi::hosting::HostControls::None);
			//		if (param.id >= 0)
			{
				gmpi::hosting::DawParameter p;
				p.id = param.id > -1 ? param.id : (-2 - (int)param.hostConnect);
				p.valueReal = atof(param.default_value.c_str());
				p.valueLo = param.minimum;
				p.valueHi = param.maximum;

				parameters[p.id] = p;
			}
		}
	}

	DawParameter* getParameter(int id)
	{
		if (auto it = parameters.find(id); it != parameters.end())
			return &(it->second);

		return {};
	}

	// return the parameter only if it changed.
	DawParameter* setParameterNormalised(int id, double value)
	{
		auto it = parameters.find(id);
		if (it == parameters.end())
			return {};

		auto& param = it->second;

		if (param.setNormalised(value))
			return &param;

		return {};
	}

	// return the parameter only if it changed.
	DawParameter* setParameterReal(int id, double value)
	{
		auto it = parameters.find(id);
		if (it == parameters.end())
			return {};

		auto& param = it->second;

		if (value == param.valueReal)
			return {};

		param.valueReal = value;
		return &param;
	}
};

struct gmpi_processor // _holder ??
{
	gmpi::shared_ptr<gmpi::api::IProcessor> processor;
	EventQue events;
	PatchManager patchManager;
	QueuedUsers pendingControllerQueueClients; // parameters waiting to be sent to GUI
	int MidiInputPinIdx = -1;

	bool start_processor(gmpi::api::IProcessorHost* host, gmpi::hosting::pluginInfo const& info);
	void setParameterNormalizedFromDaw(gmpi::hosting::pluginInfo const& info, int sampleOffset, int id, double value);
};

//gmpi::shared_ptr<gmpi::api::IProcessor> gmpi_instansiate_processor();

} // namespace hosting
} // namespace gmpi
