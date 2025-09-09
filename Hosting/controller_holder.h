#pragma once
/*
#include "Hosting/controller_holder.h"

gmpi::hosting::gmpi_processor plugin;
*/

#include <unordered_map>
#include <span>
#include <functional>
#include "GmpiApiAudio.h"
#include "GmpiSdkCommon.h"
#include "Hosting/xml_spec_reader.h"
#include "Hosting/message_queues.h"

namespace gmpi
{
namespace hosting
{

struct GmpiParameter : public QueClient // also host-controls, might need to rename it.
{
	const paramInfo* info{};
//	int32_t id{};

	double valueReal = 0.0;
	double valueLo = 0.0;
	double valueHi = 1.0;

	bool isGrabbed{};

	GmpiParameter() = default;
	GmpiParameter(const paramInfo* i, double defaultValue, double lo, double hi)
		: info(i)
		, valueReal(defaultValue)
		, valueLo(lo)
		, valueHi(hi)
	{
		assert(valueLo < valueHi);
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

	bool setNormalised(double value)
	{
		return setReal(normalized2Real(value));
	}
	double normalisedValue() const
	{
		return real2Normalized(valueReal);
	}

	bool setReal(double value)
	{
		const bool r = value != valueReal;

		valueReal = value;

		return r;
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

class ControllerPatchManager
{
public:
	std::unordered_map<int, GmpiParameter> parameters;

	ControllerPatchManager() = default;

	void init(gmpi::hosting::pluginInfo const& info)
	{
		for (const auto& paramInfo : info.parameters)
		{
			assert(paramInfo.id != -1 || paramInfo.hostConnect != gmpi::hosting::HostControls::None);
			//		if (param.id >= 0)
			{
//				auto parameterId = paramInfo.id > -1 ? paramInfo.id : (-2 - (int)paramInfo.hostConnect);

				GmpiParameter p(
					  &paramInfo
					, atof(paramInfo.default_value.c_str())
					, paramInfo.minimum
					, paramInfo.maximum
				);

				parameters[paramInfo.id] = p;
			}
		}
	}

	GmpiParameter* getParameter(int id)
	{
		if (auto it = parameters.find(id); it != parameters.end())
			return &(it->second);

		return {};
	}

	// return the parameter only if it changed.
	GmpiParameter* setParameterNormalised(int id, double value)
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
	GmpiParameter* setParameterReal(int id, double value)
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

	/*
	GmpiParameter* setParameterRaw(int id, std::span<const std::byte> data)
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
	*/


};

class gmpi_controller_holder : public gmpi::api::IEditorHost, public gmpi::api::IParameterObserver, public gmpi::hosting::interThreadQueUser
{
//	IWriteableQue* queueToProcessor{};

public:
	gmpi::hosting::pluginInfo* info{};

	gmpi_controller_holder() : //IWriteableQue* processorQueue) :
		message_que_dsp_to_ui(0x500000) // 5MB. see also AUDIO_MESSAGE_QUE_SIZE
//		queueToProcessor(processorQueue)
	{
	}

#if 0 // not used currently
	std::vector<gmpi::api::IParameterObserver*> m_guis;

	gmpi::ReturnCode registerGui(gmpi::api::IParameterObserver* gui)
	{
		m_guis.push_back(gui);
		return gmpi::ReturnCode::Ok;
	}
	gmpi::ReturnCode unRegisterGui(gmpi::api::IParameterObserver* gui)
	{
#if _HAS_CXX20
		std::erase(m_guis, gui);
#else
		if (auto it = find(m_guis.begin(), m_guis.end(), gui); it != m_guis.end())
			m_guis.erase(it);
#endif
		return gmpi::ReturnCode::Ok;
	}
#endif

	ControllerPatchManager patchManager;
	std::vector<gmpi::hosting::GmpiParameter*> nativeParams;

	std::vector<gmpi::api::IEditor*> m_editors;
	gmpi::hosting::interThreadQue message_que_dsp_to_ui;
	QueuedUsers pendingControllerQueueClients; // parameters waiting to be sent to GUI

	std::function<void(GmpiParameter const*)> notifyDaw = [](GmpiParameter const*) {};

	void init(gmpi::hosting::pluginInfo& info);

	// send initial value of all parameters to GUI
	void initUi(gmpi::api::IParameterObserver* gui); // old, needs aditional translation params->pins

	void initUi(gmpi::api::IEditor* gui);
	gmpi::ReturnCode unRegisterGui(gmpi::api::IEditor* gui);
	
	void setPinFromUi(int32_t pinId, int32_t voice, std::span<const std::byte> data);

	// IEditorHost
	gmpi::ReturnCode setPin(int32_t PinIndex, int32_t voice, int32_t size, const uint8_t* data) override
	{
        setPinFromUi(PinIndex, voice, { (std::byte*) data, (std::byte*) data + size });
		return gmpi::ReturnCode::Ok;
	}

	int32_t getHandle() override {
		return 0;
	}

	void notifyGui(GmpiParameter* param);

	// IParameterObserver
	gmpi::ReturnCode setParameter(int32_t parameterHandle, gmpi::Field fieldId, int32_t voice, int32_t size, const uint8_t* data) override
	{
		return gmpi::ReturnCode::Ok;
	}

	// interThreadQueUser
	bool onQueMessageReady(int handle, int msg_id, gmpi::hosting::my_input_stream& p_stream) override;

	gmpi::ReturnCode queryInterface(const gmpi::api::Guid* iid, void** returnInterface) override
	{
		GMPI_QUERYINTERFACE(gmpi::api::IEditorHost);
		GMPI_QUERYINTERFACE(gmpi::api::IParameterObserver);
		return gmpi::ReturnCode::NoSupport;
	}
	GMPI_REFCOUNT_NO_DELETE;
};

} // namespace hosting
} // namespace gmpi
