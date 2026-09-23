#pragma once
#include "GmpiSdkCommon.h"
#include "RefCountMacros.h"
#include "Common.h"

/*
#include "Extensions/ParameterIterator.h"
*/

// SynthEdit-specific.
namespace synthedit
{

// a value, plus the datatype needed to interpret it.
struct DECLSPEC_NOVTABLE IVariant : gmpi::api::IUnknown
{
	virtual gmpi::ReturnCode setData(gmpi::PinDatatype datatype, const uint8_t* data, int32_t size) = 0;

	// {A8B23A3E-B232-41C5-A037-E9C79B90ED9C}
	inline static const gmpi::api::Guid guid =
	{ 0xa8b23a3e, 0xb232, 0x41c5, { 0xa0, 0x37, 0xe9, 0xc7, 0x9b, 0x90, 0xed, 0x9c } };
};

struct DECLSPEC_NOVTABLE IParameter : gmpi::api::IUnknown
{
	virtual gmpi::ReturnCode getValue(gmpi::Field field, int32_t voice, IVariant* returnValue) = 0;

	// {A3081EDC-B544-4D5E-A626-1C6C3C62B36A}
	inline static const gmpi::api::Guid guid =
	{ 0xa3081edc, 0xb544, 0x4d5e, { 0xa6, 0x26, 0x1c, 0x6c, 0x3c, 0x62, 0xb3, 0x6a } };
};

struct DECLSPEC_NOVTABLE IParameterCallback : gmpi::api::IUnknown
{
    virtual gmpi::ReturnCode onParameter(IParameter* param) = 0;

	// {1BC85CFE-797A-47EF-A0C0-A4469203A046}
	inline static const gmpi::api::Guid guid =
	{ 0x1bc85cfe, 0x797a, 0x47ef, { 0xa0, 0xc0, 0xa4, 0x46, 0x92, 0x3, 0xa0, 0x46 } };
};
	
// extension to GMPI to provide support for auto-duplicating pins.
struct DECLSPEC_NOVTABLE IParameterIterator : public gmpi::api::IUnknown
{
public:
	virtual void listParameters(gmpi::api::IUnknown* callback) = 0;

	// {168BE5DF-5250-48EC-B285-83B1CC3CBB60}
	inline static const gmpi::api::Guid guid =
	{ 0x168be5df, 0x5250, 0x48ec, { 0xb2, 0x85, 0x83, 0xb1, 0xcc, 0x3c, 0xbb, 0x60 } };
};

namespace detail
{
	// which gmpi::PinDatatype transports a given C++ type.
	template<typename T>
	struct VariantTypeTraits
	{
		static constexpr gmpi::PinDatatype datatype = static_cast<gmpi::PinDatatype>(gmpi::PinTypeTraits<T>::PinDataType);
	};

	// gmpi::PinTypeTraits has no entry for wide strings (the datatype of a 'text' parameter's value).
	template<>
	struct VariantTypeTraits<std::wstring>
	{
		static constexpr gmpi::PinDatatype datatype = gmpi::PinDatatype::WideString;
	};
}

// helper for receiving a value of any datatype. c.f. gmpi::ReturnString
struct ReturnVariant : IVariant
{
	std::vector<uint8_t> bytes;
	gmpi::PinDatatype datatype{};

	gmpi::ReturnCode setData(gmpi::PinDatatype pdatatype, const uint8_t* data, int32_t size) override
	{
		datatype = pdatatype;
		bytes.assign(data, data + (size > 0 ? size : 0));
		return gmpi::ReturnCode::Ok;
	}

	// convert to a C++ type. fails if the datatype does not match.
	template<typename T>
	bool get(T& returnValue) const
	{
		if(datatype != detail::VariantTypeTraits<T>::datatype)
			return false;

		gmpi::valueFromData(bytes, returnValue);
		return true;
	}

	GMPI_QUERYINTERFACE_METHOD(synthedit::IVariant);
	GMPI_REFCOUNT_NO_DELETE;
};

// helper to read the fields of one parameter. Manages the lifetime of the IParameter,
// and converts each field from the host's raw bytes to the appropriate C++ type.
//
// All text fields are utf-8 std::string. Only the value of a 'text' parameter is a std::wstring.
//
// note: Field::Default, Field::RangeLo and Field::RangeHi share the parameter's own datatype,
// but are not served by SynthEdit yet. Read them with getField<T>() once they are.
struct ParameterHelper
{
	gmpi::shared_ptr<IParameter> param_;

	ParameterHelper() = default;

	ParameterHelper(IParameter* param)
	{
		param_ = param; // adds a reference. released by the destructor.
	}

	bool isNull() const
	{
		return param_.isNull();
	}

	// read any field as a specific C++ type. returns false if the field is not of that datatype.
	template<typename T>
	bool getField(gmpi::Field field, T& returnValue, int32_t voice = 0) const
	{
		if(param_.isNull())
			return false;

		ReturnVariant v;
		if(gmpi::ReturnCode::Ok != param_->getValue(field, voice, &v))
			return false;

		return v.get(returnValue);
	}

	// read any field without knowing its datatype up-front.
	ReturnVariant getFieldRaw(gmpi::Field field, int32_t voice = 0) const
	{
		ReturnVariant v;

		if(!param_.isNull())
			param_->getValue(field, voice, &v);

		return v;
	}

	// the parameter's unique id (within the patch manager).
	int32_t getHandle() const
	{
		return getOrDefault<int32_t>(gmpi::Field::Handle);
	}

	// host-controls (Patch Commands, Polyphony etc) are driven by SynthEdit, not by the user.
	bool isHostControl() const
	{
		int32_t hostControl{ -1 };
		getField(gmpi::Field::HostControl, hostControl);
		return hostControl != -1;
	}

	// the datatype of the parameter's value.
	gmpi::PinDatatype getDatatype() const
	{
		return getFieldRaw(gmpi::Field::Value).datatype;
	}

	// the value itself. use getValue<T>() when you know the datatype, else getValueRaw().
	template<typename T>
	bool getValue(T& returnValue, int32_t voice = 0) const
	{
		return getField(gmpi::Field::Value, returnValue, voice);
	}

	ReturnVariant getValueRaw(int32_t voice = 0) const
	{
		return getFieldRaw(gmpi::Field::Value, voice);
	}

	// the value scaled 0.0 -> 1.0, as automation sees it.
	float getNormalized(int32_t voice = 0) const
	{
		return getOrDefault<float>(gmpi::Field::Normalized, voice);
	}

	std::string getShortName() const
	{
		return getOrDefault<std::string>(gmpi::Field::ShortName);
	}

	// slash-separated path, e.g. "Patch Memory/Cutoff".
	std::string getLongName() const
	{
		return getOrDefault<std::string>(gmpi::Field::LongName);
	}

	// tooltip text.
	std::string getHint() const
	{
		return getOrDefault<std::string>(gmpi::Field::Hint);
	}

	// comma-separated choices of a 'list' parameter, e.g. "Off,Low,High".
	std::string getEnumList() const
	{
		return getOrDefault<std::string>(gmpi::Field::EnumList);
	}

	// file filter of a 'filename' parameter, e.g. "wav".
	std::string getFileExtension() const
	{
		return getOrDefault<std::string>(gmpi::Field::FileExtension);
	}

	// the parameter's context-menu (MIDI learn etc).
	std::string getMenuItems() const
	{
		return getOrDefault<std::string>(gmpi::Field::MenuItems);
	}

	int32_t getMenuSelection() const
	{
		return getOrDefault<int32_t>(gmpi::Field::MenuSelection);
	}

	// MIDI CC number this parameter is learned to, or -1 for none.
	int32_t getAutomation() const
	{
		return getOrDefault<int32_t>(gmpi::Field::Automation);
	}

	std::string getAutomationSysex() const
	{
		return getOrDefault<std::string>(gmpi::Field::AutomationSysex);
	}

	// true while the user is dragging the control (mouse down).
	bool isGrabbed() const
	{
		return getOrDefault<bool>(gmpi::Field::Grab);
	}

	// private parameters are hidden from the host's automation list.
	bool isPrivate() const
	{
		return getOrDefault<bool>(gmpi::Field::Private);
	}

	// stateful (aka persistant) parameters are saved in the patch.
	bool isStateful() const
	{
		return getOrDefault<bool>(gmpi::Field::Stateful);
	}

	bool getIgnoreProgramChange() const
	{
		return getOrDefault<bool>(gmpi::Field::IgnoreProgramChange);
	}

private:
	// read a field, returning a default-constructed value if it's missing or the wrong datatype.
	template<typename T>
	T getOrDefault(gmpi::Field field, int32_t voice = 0) const
	{
		T returnValue{};
		getField(field, returnValue, voice);
		return returnValue;
	}
};

// helper class to retrieve pin information.
struct ParameterInformation : public synthedit::IParameterCallback
{
	struct ParameterInfo
	{
		int32_t handle;
		gmpi::PinDatatype datatype;
		std::string shortName;
		std::string longName;	// slash-separated path
		bool isHostControl;
	};

	std::vector<ParameterInfo> parameters;

	ParameterInformation(gmpi::api::IUnknown* phost)
	{
		gmpi::shared_ptr<synthedit::IParameterIterator> parameterIterator;
		phost->queryInterface(&synthedit::IParameterIterator::guid, parameterIterator.put_void());

		if(parameterIterator)
			parameterIterator->listParameters(this);
	}

	gmpi::ReturnCode onParameter(IParameter* param) override
	{
		ParameterHelper p(param);

		parameters.push_back({ p.getHandle(), p.getDatatype(), p.getShortName(), p.getLongName(), p.isHostControl() });
		return gmpi::ReturnCode::Ok;
	}

	GMPI_QUERYINTERFACE_METHOD(synthedit::IParameterCallback);
	GMPI_REFCOUNT
};


} //namespace synthedit
