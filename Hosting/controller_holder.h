#pragma once
/*
#include "Hosting/controller_holder.h"

gmpi::hosting::gmpi_processor plugin;
*/

#include <unordered_map>
#include <span>
#include <variant>
#include <functional>
#include "GmpiApiAudio.h"
#include "GmpiSdkCommon.h"
#include "Hosting/xml_spec_reader.h"
#include "Hosting/message_queues.h"

namespace gmpi
{
namespace hosting
{

constexpr bool is_scalar(gmpi::PinDatatype dt)
{
	switch (dt)
	{
	case gmpi::PinDatatype::String:
	case gmpi::PinDatatype::Blob:
	case gmpi::PinDatatype::Midi:
		return false;
	default:
		return true;
	}
}

struct GmpiParameter : public QueClient // also host-controls, might need to rename it.
{
	const paramInfo* info{};

	// Holds either a numeric value (double) or a textual/blob value (std::vector<uint8_t>).
	std::variant<double, std::vector<uint8_t>> value_{};

	bool isGrabbed{};

	GmpiParameter() = default;
	GmpiParameter(const paramInfo* i)
		: info(i)
	{
		assert(info->minimum <= info->maximum);

		setToDefault();
	}

	void setToDefault()
	{
		if (is_scalar(info->datatype))
		{
			value_ = atof(info->default_value_s.c_str());
		}
		else
		{
			value_ = std::vector<uint8_t>{}; // TODO: proper default for string/blob
		}
	}

	double valueReal() const
	{
		if (auto pval = std::get_if<double>(&value_); pval)
			return *pval;

		return 0.0;
	}

	double real2Normalized(double r) const
	{
		if (info->maximum == info->minimum)
			return 0.0; // avoid divide by zero.
		return (r - info->minimum) / (info->maximum - info->minimum);
	}

	double normalized2Real(double n) const
	{
		return info->minimum + n * (info->maximum - info->minimum);
	}

	bool setNormalised(double value)
	{
		return setReal(normalized2Real(value));
	}
	double normalisedValue() const
	{
		return real2Normalized(valueReal());
	}

	bool setReal(double value)
	{
		const bool r = value != valueReal();

		value_ = value;

		return r;
	}

	// BLOB based parameters
	bool setBlob(std::span<const uint8_t> data)
	{
		if (auto* v = std::get_if<std::vector<uint8_t>>(&value_))
		{
			const bool changed = v->size() != data.size() || !std::equal(v->begin(), v->end(), data.begin());

			// Reuse existing storage if possible.
			v->assign(data.begin(), data.end());

			return changed;
		}

		// Construct the vector in-place inside the variant (no temporary).
		value_.emplace<std::vector<uint8_t>>(data.begin(), data.end());
		return true;
	}

	int32_t queryQueMessageLength(int availableBytes) override
	{
		if(std::holds_alternative<double>(value_))
			return sizeof(double);
		else
			return sizeof(int32_t) + static_cast<int32_t>(std::get<std::vector<uint8_t>>(value_).size());
	}

	void getQueMessage(class my_output_stream& outStream, int messageLength) override
	{
		const bool hostNeedsParameterUpdate{};
		const int32_t voice{};

		outStream << info->id;

		if (std::holds_alternative<double>(value_))
		{
			outStream << id_to_long("ppc2");
			outStream << messageLength;

			outStream << valueReal();
		}
		else
		{
			auto& v = std::get<std::vector<uint8_t>>(value_);

			outStream << id_to_long("ppc3");
			outStream << messageLength;

			outStream << static_cast<int32_t>(v.size());
			outStream.Write(v.data(), static_cast<unsigned int>(v.size()));
		}
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
					//, paramInfo.default_value // atof(paramInfo.default_value.c_str())
					//, paramInfo.minimum
					//, paramInfo.maximum
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

		if (!param.setReal(value))
			return {};

		return &param;
	}

	GmpiParameter* setParameterBlob(int id, std::span<const uint8_t> data)
	{
		auto it = parameters.find(id);
		if (it == parameters.end())
			return {};

		auto& param = it->second;

		if (!param.setBlob(data))
			return {};

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
public:
	gmpi::hosting::pluginInfo* info{};

	gmpi_controller_holder() :
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
//?	QueuedUsers pendingControllerQueueClients; // parameters waiting to be sent to GUI

	std::function<void(GmpiParameter*)> notifyDaw = [](GmpiParameter*) {};

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

	std::string getPreset();
	void setPresetXmlFromDaw(const std::string& xml);

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
