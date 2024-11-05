#pragma once
#include "GmpiApiCommon.h"

/*
#include "Extensions/PinCount.h"
*/

// SynthEdit-specific.
namespace synthedit
{
// extension to GMPI to provide support for auto-duplicating pins.
struct DECLSPEC_NOVTABLE IPinCount : public gmpi::api::IUnknown
{
public:
	// provide the total count of all auto-duplicating pins
	virtual int32_t getAutoduplicatePinCount() = 0;

	// {B766D3F6-4CC2-49BC-9C89-87065CD19470}
	inline static const gmpi::api::Guid guid =
	{ 0xb766d3f6, 0x4cc2, 0x49bc, { 0x9c, 0x89, 0x87, 0x6, 0x5c, 0xd1, 0x94, 0x70 } };
};

} //namespace synthedit
