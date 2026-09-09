#pragma once
#include "GmpiSdkCommon.h"
#include "RefCountMacros.h"

/*
#include "Extensions/PinCount.h"
*/

// SynthEdit-specific.
namespace synthedit
{
	
struct DECLSPEC_NOVTABLE IParameterCallback : gmpi::api::IUnknown
{
    virtual gmpi::ReturnCode onParameter(int32_t handle, gmpi::PinDatatype datatype) = 0;

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

	gmpi::ReturnCode onParameter(int32_t handle, gmpi::PinDatatype datatype) override
	{
		parameters.push_back({ handle, datatype });
		return gmpi::ReturnCode::Ok;
	}

	GMPI_QUERYINTERFACE_METHOD(synthedit::IParameterCallback);
	GMPI_REFCOUNT
};


} //namespace synthedit
