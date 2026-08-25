#pragma once
/*
#include "Hosting/controller_holder.h"

gmpi::hosting::gmpi_processor plugin;
*/

#include <unordered_map>
#include <span>
#include "Core/base64.h"
#include <variant>
#include <functional>
#include <cassert> // setFromXml's datatype-mismatch assert
#include <cstdlib> // strtod / atof, ditto
#include "GmpiApiAudio.h"
#include "GmpiSdkCommon.h"
#include "Hosting/xml_spec_reader.h"
#include "Hosting/message_queues.h"
#include "helpers/IController.h"

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

// True if the whole string parses as a number; trailing whitespace tolerated.
// Only used to assert that a scalar pin's preset text really is scalar - see
// setFromXml. Deliberately not a parser: atof() still does the conversion.
inline bool isNumericText(const char* s)
{
	if (!s)
		return false;

	char* end{};
	(void)std::strtod(s, &end);

	if (end == s)
		return false; // no conversion at all

	while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')
		++end;

	return *end == '\0'; // nothing but the number
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

	// Whether setFromXml would STORE this text rather than assert on it - the
	// same question its two asserts ask, asked before the fact instead of after.
	// The scalar one is literally written in terms of this call; the other - the
	// switch's `default:` - fires on exactly the datatypes the last line here
	// excludes. So a caller that tests here and a debug build that trips there
	// cannot come to disagree.
	//
	// Who needs it: setFromXml's asserts mean "the WRITER put the wrong thing in
	// this preset", which is a defect report about input that came from code.
	// Text read out of a FILE is not that - a file on disk is editable by
	// anybody, so garbage in one is an ordinary event and not a bug. Whoever
	// decided to trust a file is the one that has to ask this first, and then
	// decline the whole document rather than reach a value that will abort a
	// debug build and silently become 0.0 in a release one. The standalone
	// wrapper's SessionState::restore is the caller this was written for.
	bool acceptsXmlText(const char* textValue) const
	{
		if (is_scalar(info->datatype))
		{
			// An empty attribute is tolerated - "no value" is a legitimate
			// encoding, not a datatype mismatch.
			return isNumericText(textValue) || (textValue && !*textValue);
		}

		// The only two non-scalar datatypes setFromXml knows how to store; the
		// switch below asserts on anything else (Midi reaches here, and there
		// is no preset encoding for one).
		return info->datatype == gmpi::PinDatatype::String
			|| info->datatype == gmpi::PinDatatype::Blob;
	}

	bool setFromXml(const char* textValue)
	{
		if (is_scalar(info->datatype))
		{
			// Each pin has a fixed datatype, so a scalar pin whose preset text
			// is not a number means the WRITER put the wrong thing in the
			// preset. Say so rather than silently storing atof()'s 0.0, which
			// would surface the corruption somewhere quieter and further away.
			assert(acceptsXmlText(textValue));
			return setReal(atof(textValue));
		}
		else
		{
			switch (info->datatype)
			{
			// Base64, both of them: presets are XML, and a string parameter is
			// a byte vector rather than text - an embedded NUL truncated it and
			// a control character made the document ill-formed. The codec lives
			// in GMPI/Core so both ends of the round trip share one
			// implementation; writePresetXml is the other end and carries the
			// full argument.
			case gmpi::PinDatatype::String:
			case gmpi::PinDatatype::Blob:
				return setBlob(gmpi::base64Decode(textValue));
                    
            default:
                assert(false); // not supported
                break;
			}
		}
		return false;
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

// Writes a parameter store out as a <Preset> document - the format
// GmpiParameter::setFromXml reads back, and the one every wrapper hands its
// host through getState/setState.
//
// ONE writer, because there are two stores of the identical type: the
// processor's PatchManager and the controller's ControllerPatchManager both
// hold std::unordered_map<int, GmpiParameter>. They used to have a preset
// writer each, and the two had already drifted - the processor's learned to
// base64 a blob and to skip non-stateful parameters, the controller's went on
// writing valueReal() for every datatype, so which of them a host asked
// decided whether a blob survived. Anything a caller wants added to the format
// (a name, a per-parameter datatype tag) belongs here, where both get it.
//
// Takes the map rather than either holder so neither side has to know the
// other exists.
std::string writePresetXml(const std::unordered_map<int, GmpiParameter>& parameters);

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

class gmpi_controller_holder :
	public gmpi::api::IEditorHost
	, public gmpi::api::IControllerHost
	, public gmpi::api::IParameterObserver
	, public gmpi::hosting::interThreadQueUser
	, public gmpi::hosting::IController
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

	// Scratch for onQueMessageReady's blob arm. resize() keeps the high-water
	// capacity, so the steady 30Hz display-state stream (65KB/frame) allocates
	// once ever instead of once per message. Single-threaded use: the queue is
	// only polled from the GUI/timer thread.
	std::vector<uint8_t> blobScratch;

	std::vector<gmpi::api::IEditor*> m_editors;
	gmpi::hosting::interThreadQue message_que_dsp_to_ui;

	std::function<void(GmpiParameter*)> notifyDaw = [](GmpiParameter*) {};

	// Transport for parameter types the DAW's own parameter mechanism cannot
	// carry (currently blobs). Installed by each wrapper; default no-op keeps
	// hosts that never need it working unchanged. The parameter's
	// getQueMessage() already frames these ("ppc3"); the wrapper only moves
	// the framed bytes to the processor's ui->dsp queue.
	std::function<void(GmpiParameter*)> sendNonNativeParameterToProcessor = [](GmpiParameter*) {};

	void init(gmpi::hosting::pluginInfo& info);

	// send initial value of all parameters to GUI
	void initUi(gmpi::api::IParameterObserver* gui); // old, needs aditional translation params->pins

	// IController interface
	void initUi(gmpi::api::IUnknown* editor) override;
	gmpi::ReturnCode unRegisterGui(gmpi::api::IUnknown* editor) override;

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

	// The controller's parameters as a <Preset> document. Const because reading
	// the store changes nothing, which is what lets a caller capture state from
	// a `const` reference and be sure it did not.
	std::string getPresetXml() const;

	// The name the wrappers have always called it by, kept so their call sites
	// do not all have to move.
	std::string getPreset() { return getPresetXml(); }

	// Reads a <Preset> document into the store. TRUE when it found one, false
	// when the text held no <Preset> element - in which case NOTHING was
	// changed, not even the reset-to-default pass, so a caller can tell "that
	// was not a preset" apart from "that was an empty preset" (which does reset
	// every stateful parameter, and is how a revert-to-defaults is spelled).
	bool setPresetXmlFromDaw(const std::string& xml);

	// Hand the current parameter values to the plugin's own <Controller/>.
	//
	// A plugin whose state IS a parameter has no other way to learn that its
	// state was restored: the controller is created and initialize()d, and then
	// nothing ever tells it what the preset said. It is not one of m_editors -
	// those are IEditors and get setPin - so initUi does not reach it either.
	//
	// Encoding is the PARAMETER's datatype, not a pin's: a plugin controller has
	// no pins. Blobs and strings arrive as their raw bytes, scalars as a double.
	// Non-stateful parameters are skipped for the same reason the preset writer
	// skips them - they hold things valid only for the run that published them,
	// such as a raw pointer to a live object.
	//
	// Null is accepted and does nothing, so a caller need not test for a plugin
	// that declares no controller.
	void notifyControllerOfPreset(gmpi::api::IParameterObserver* pluginController) const;

	// IParameterObserver / IControllerHost (identical signature; one impl serves both vtables).
	// Writes the value into patchManager so subsequent initUi calls deliver
	// it to the editor. This is what makes a plugin's <Controller/> able to
	// stash state (e.g. a pointer to its app object) before the editor opens.
	gmpi::ReturnCode setParameter(int32_t parameterHandle, gmpi::Field fieldId, int32_t voice, int32_t size, const uint8_t* data) override
	{
		auto* param = patchManager.getParameter(parameterHandle);
		if (!param)
			return gmpi::ReturnCode::Ok;

		if (fieldId == gmpi::Field::Value)
		{
			switch (param->info->datatype)
			{
			case gmpi::PinDatatype::Blob:
				// Unlike scalars, blobs cannot ride the DAW's normalized-value
				// path (notifyDaw/performEdit speak doubles), so a changed blob
				// goes to the processor through the wrapper-installed hook
				// below - each wrapper supplies its own transport (VST3: a
				// binary IMessage into the processor's ui->dsp queue).
				if (param->setBlob({ data, static_cast<size_t>(size) }))
					sendNonNativeParameterToProcessor(param);
				break;
			case gmpi::PinDatatype::Float32:
				if (size == sizeof(float))
					param->setReal(static_cast<double>(*reinterpret_cast<const float*>(data)));
				break;
			case gmpi::PinDatatype::Int32:
				if (size == sizeof(int32_t))
					param->setReal(static_cast<double>(*reinterpret_cast<const int32_t*>(data)));
				break;
			case gmpi::PinDatatype::Bool:
				if (size == sizeof(bool))
					param->setReal(static_cast<double>(*reinterpret_cast<const bool*>(data)));
				break;
			default:
				break;
			}
		}
		else if (fieldId == gmpi::Field::Normalized && size == sizeof(float))
		{
			param->setNormalised(static_cast<double>(*reinterpret_cast<const float*>(data)));
		}

		return gmpi::ReturnCode::Ok;
	}

	// interThreadQueUser
	bool onQueMessageReady(int handle, int msg_id, gmpi::hosting::my_input_stream& p_stream) override;

	gmpi::ReturnCode queryInterface(const gmpi::api::Guid* iid, void** returnInterface) override
	{
		GMPI_QUERYINTERFACE(gmpi::api::IEditorHost);
		GMPI_QUERYINTERFACE(gmpi::api::IControllerHost);
		GMPI_QUERYINTERFACE(gmpi::api::IParameterObserver);
		return gmpi::ReturnCode::NoSupport;
	}
	GMPI_REFCOUNT_NO_DELETE;
};

} // namespace hosting
} // namespace gmpi
