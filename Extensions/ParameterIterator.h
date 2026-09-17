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
		if(datatype != static_cast<gmpi::PinDatatype>(gmpi::PinTypeTraits<T>::PinDataType))
			return false;

		gmpi::valueFromData(bytes, returnValue);
		return true;
	}

	GMPI_QUERYINTERFACE_METHOD(synthedit::IVariant);
	GMPI_REFCOUNT_NO_DELETE;
};

// helper class to retrieve pin information.
struct ParameterInformation : public synthedit::IParameterCallback
{
	struct ParameterInfo
	{
		int32_t handle;
		gmpi::PinDatatype datatype;
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
		ReturnVariant v;

		int32_t handle{};
		param->getValue(gmpi::Field::Handle, 0, &v);
		v.get(handle);

		// a parameter's datatype is that of its value field.
		param->getValue(gmpi::Field::Value, 0, &v);

		parameters.push_back({ handle, v.datatype });
		return gmpi::ReturnCode::Ok;
	}

	GMPI_QUERYINTERFACE_METHOD(synthedit::IParameterCallback);
	GMPI_REFCOUNT
};


} //namespace synthedit
