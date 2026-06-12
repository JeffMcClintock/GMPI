#pragma once
#include "GmpiApiCommon.h"

/*
#include "Extensions/ElapsedTime.h"
*/

// SynthEdit-specific.
namespace synthedit
{

// Extension to GMPI to provide the elapsed render time, measured in samples
// since the start of rendering.
struct DECLSPEC_NOVTABLE IElapsedTime : public gmpi::api::IUnknown
{
	// the elapsed render time in samples.
	virtual int64_t getElapsedTime() = 0;

	// {09B9DABB-2AF3-473E-B044-A98DE3B16B58}
	inline static const gmpi::api::Guid guid =
	{ 0x09B9DABB, 0x2AF3, 0x473E, { 0xB0, 0x44, 0xA9, 0x8D, 0xE3, 0xB1, 0x6B, 0x58} };
};

} //namespace synthedit
