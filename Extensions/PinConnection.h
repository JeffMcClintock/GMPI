#pragma once
#include "GmpiSdkCommon.h"
#include "RefCountMacros.h"

/*
#include "Extensions/PinConnection.h"
*/

// SynthEdit-specific.
namespace synthedit
{

// Extension to GMPI: is anything actually patched into this pin?
//
// WHY IT IS NOT isStreaming(). setPinStreaming reports whether a pin carries
// TIME-VARYING audio, so a connected input holding a steady CV reports false -
// indistinguishable from an unpatched one, and backwards for the case that
// matters most. This asks the graph, not the signal.
//
// WHAT IT IS FOR: normalling. A module that behaves differently when a jack is
// empty - falling back to an internal source, inheriting the channel above it,
// bypassing a modulation stage - cannot be written without it. A host that does
// not offer this interface leaves such a module treating every input as
// patched, which for some designs means silence.
//
// CONNECTIONS DO NOT CHANGE while a processor lives: SynthEdit rebuilds the DSP
// graph when the user patches a cable, constructing fresh processors. So the
// answer is stable for the lifetime of the plugin and callers should ask once,
// in open(), and cache it. Asking per sample would be wasteful for a value that
// cannot move.
struct DECLSPEC_NOVTABLE IPinConnection : public gmpi::api::IUnknown
{
public:
	// pinId indexes the plugin's pins in declaration order, the same numbering
	// setPin() and setBuffer() use.
	//
	// An input fed only by its default value counts as NOT connected, which is
	// the question a module is really asking. Out-of-range ids fail rather than
	// guessing.
	virtual gmpi::ReturnCode isPinConnected(int32_t pinId, bool* returnValue) = 0;

	// {6F2A9C41-8E3D-4B77-9A5E-2D1F4C8B0E63}
	inline static const gmpi::api::Guid guid =
	{ 0x6f2a9c41, 0x8e3d, 0x4b77, { 0x9a, 0x5e, 0x2d, 0x1f, 0x4c, 0x8b, 0x0e, 0x63 } };
};

// Reads every pin's connection state once, so a plugin can cache it.
//
// Degrades on purpose: a host predating this interface leaves `connected`
// empty, and isConnected() then answers true for everything - which is what
// such a host effectively meant, and what plugins saw before this existed.
struct PinConnections
{
	std::vector<bool> connected;
	bool supported = false;

	PinConnections() = default;

	PinConnections(gmpi::api::IUnknown* phost, int32_t pinCount)
	{
		query(phost, pinCount);
	}

	void query(gmpi::api::IUnknown* phost, int32_t pinCount)
	{
		connected.clear();
		supported = false;

		if (!phost || pinCount <= 0)
			return;

		gmpi::shared_ptr<synthedit::IPinConnection> pins;
		if (phost->queryInterface(&synthedit::IPinConnection::guid, pins.put_void()) != gmpi::ReturnCode::Ok || !pins)
			return;

		connected.resize(static_cast<size_t>(pinCount), true);

		for (int32_t i = 0; i < pinCount; ++i)
		{
			bool value = true;
			if (pins->isPinConnected(i, &value) == gmpi::ReturnCode::Ok)
				connected[static_cast<size_t>(i)] = value;
		}

		supported = true;
	}

	// True for an unknown pin, so a plugin that asks about a pin the host does
	// not know about behaves as it did before this interface existed.
	bool isConnected(int32_t pinId) const
	{
		if (pinId < 0 || static_cast<size_t>(pinId) >= connected.size())
			return true;

		return connected[static_cast<size_t>(pinId)];
	}
};

} //namespace synthedit
