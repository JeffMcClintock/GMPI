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
	const paramInfo* info{};

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

	double real2Normalized(double r) const
	{
		if (valueHi == valueLo)
			return 0.0; // avoid divide by zero.
		return (r - valueLo) / (valueHi - valueLo);
	}

	double normalized2Real(double n) const
	{
		return valueLo + n * (valueHi - valueLo);
	}

	int queryQueMessageLength(int availableBytes) override
	{
		return sizeof(double);
	}

	void getQueMessage(class my_output_stream& outStream, int messageLength) override
	{
		const bool hostNeedsParameterUpdate{};
		const int32_t voice{};

		outStream << info->id;
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
			//assert(param.id != -1 || param.hostConnect != gmpi::hosting::HostControls::None);
			//		if (param.id >= 0)
			{
				gmpi::hosting::DawParameter p;
//				p.id = param.id; // > -1 ? param.id : (-2 - (int)param.hostConnect);
				p.info = &param;
				p.valueReal = param.default_value; // atof(param.default_value.c_str());
				p.valueLo = param.minimum;
				p.valueHi = param.maximum;

				parameters[param.id] = p;
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

struct gmpi_processor : public gmpi::hosting::interThreadQueUser // _holder ??
	, public gmpi::api::IProcessorHost
{
	gmpi::hosting::pluginInfo const* info{};

	gmpi::shared_ptr<gmpi::api::IProcessor> processor;
	EventQue events;
	PatchManager patchManager;
	QueuedUsers pendingControllerQueueClients; // parameters waiting to be sent to GUI
	int MidiInputPinIdx = -1;
    std::vector<gmpi::hosting::DawParameter*> nativeParams;

	int32_t blockSize = 512;
	float sampleRate = 44100.0f;

    void init(gmpi::hosting::pluginInfo const& info);
	bool start_processor(gmpi::api::IProcessorHost* host, gmpi::hosting::pluginInfo const& info, int32_t blockSize, float sampleRate);
	void setParameterNormalizedFromDaw(gmpi::hosting::pluginInfo const& info, int sampleOffset, int id, double value);
    void setHostControlFromDaw(gmpi::hosting::HostControls hc, double value);
	void sendParameterToProcessor(gmpi::hosting::pluginInfo const& info, DawParameter* param, int sampleOffset);

	bool onQueMessageReady(int handle, int msg_id, gmpi::hosting::my_input_stream& p_stream) override;

//	gmpi::ReturnCode setPin(int32_t timestamp, int32_t pinId, int32_t size, const uint8_t* data);

	void setPresetUnsafe(std::string& chunk);
	std::string getPresetUnsafe();

	// IAudioPluginHost
	gmpi::ReturnCode setPin(int32_t timestamp, int32_t pinId, int32_t size, const uint8_t* data) override;
	gmpi::ReturnCode setPinStreaming(int32_t timestamp, int32_t pinId, bool isStreaming) override;
	gmpi::ReturnCode setLatency(int32_t latency) override;
	gmpi::ReturnCode sleep() override;
	int32_t getBlockSize() override;
	float getSampleRate() override;
	int32_t getHandle() override;

	GMPI_QUERYINTERFACE_METHOD(gmpi::api::IProcessorHost);
	GMPI_REFCOUNT_NO_DELETE;
};

} // namespace hosting
} // namespace gmpi
