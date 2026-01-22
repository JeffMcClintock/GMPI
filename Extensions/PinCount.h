#pragma once
#include "GmpiSdkCommon.h"
#include "RefCountMacros.h"

/*
#include "Extensions/PinCount.h"
*/

// SynthEdit-specific.
namespace synthedit
{
	
struct DECLSPEC_NOVTABLE IProcessorPinsCallback : gmpi::api::IUnknown
{
    virtual gmpi::ReturnCode onPin(gmpi::PinDirection direction, gmpi::PinDatatype datatype) = 0;

    // {D3A5F4C1-6C2E-4D2E-8F1C-3C8D1B6F4E2A}
    inline static const gmpi::api::Guid guid =
	{ 0xD3A5F4C1, 0x6C2E, 0x4D2E, { 0x8F, 0x1C, 0x3C, 0x8D, 0x1B, 0x6F, 0x4E, 0x2A} };
};
	
	
// extension to GMPI to provide support for auto-duplicating pins.
struct DECLSPEC_NOVTABLE IPinCount : public gmpi::api::IUnknown
{
public:
	// provide the total count of all auto-duplicating pins
	// deprecated, use listPins instead.
	virtual int32_t getAutoduplicatePinCount_deprecated() = 0;
	virtual void listPins(gmpi::api::IUnknown* callback) = 0;

	// {B766D3F6-4CC2-49BC-9C89-87065CD19470}
	inline static const gmpi::api::Guid guid =
	{ 0xb766d3f6, 0x4cc2, 0x49bc, { 0x9c, 0x89, 0x87, 0x6, 0x5c, 0xd1, 0x94, 0x70 } };
};

// helper class to retrieve pin information.
struct PinInformation : public synthedit::IProcessorPinsCallback
{
	struct PinInfo
	{
		gmpi::PinDirection direction;
		gmpi::PinDatatype datatype;
	};

	std::vector<PinInfo> pins;

	PinInformation(gmpi::api::IUnknown* phost)
	{
		gmpi::shared_ptr<synthedit::IPinCount> pinCount;
		phost->queryInterface(&synthedit::IPinCount::guid, pinCount.put_void());

		pinCount->listPins(this);
	}

	gmpi::ReturnCode onPin(gmpi::PinDirection direction, gmpi::PinDatatype datatype) override
	{
		pins.push_back({ direction, datatype });
		return gmpi::ReturnCode::Ok;
	}

	GMPI_QUERYINTERFACE_METHOD(synthedit::IProcessorPinsCallback);
	GMPI_REFCOUNT
};


} //namespace synthedit
