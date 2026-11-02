#pragma once
#include "GmpiSdkCommon.h"
#include "RefCountMacros.h"

/*
#include "Extensions/TraceSpec.h"
*/

// SynthEdit-specific.
namespace synthedit
{

// A module that wants hardware painted on the faceplate beneath it: a knob wants a knob body
// under its pointer, a patch point wants a jack. The module says WHAT, in its own coordinates.
// The host, which is the only party that knows where every module sits, works out WHERE and
// tells the panel (ITraceSink). The module is never told its position and never has to be.
struct DECLSPEC_NOVTABLE ITraceSource : public gmpi::api::IUnknown
{
public:
    // The spec in the module's OWN coordinates (its top-left is 0,0), in the panel's Layout
    // grammar, e.g. "knob 7.5 7 6.6". An empty string means nothing to paint right now.
    virtual gmpi::ReturnCode getTraceSpec(gmpi::api::IString* returnSpec) = 0;

    // {A3C1E7D9-52B4-4F6E-9D08-7B2C4E1F8A35}
    inline static const gmpi::api::Guid guid =
    { 0xa3c1e7d9, 0x52b4, 0x4f6e, { 0x9d, 0x08, 0x7b, 0x2c, 0x4e, 0x1f, 0x8a, 0x35 } };
};

struct TraceSpecEntry
{
    const char* spec; // exactly as the source gave it; the host does not parse it
    float x, y;       // the source's top-left in the sink's local coordinates
};

// The module that paints the hardware (the faceplate). The host calls this whenever the set
// of sources over it may have changed - after layout and after every drag - with one entry
// per source whose centre lies inside this module's bounds. The sink adds each entry's offset
// to the coordinates in its spec. The same list can arrive again unchanged, so compare before
// doing anything expensive.
struct DECLSPEC_NOVTABLE ITraceSink : public gmpi::api::IUnknown
{
public:
    virtual gmpi::ReturnCode setTraceSpecs(const TraceSpecEntry* entries, int32_t count) = 0;

    // {5E8B2A47-C913-4D6F-B0A2-3F7D9C4E6B18}
    inline static const gmpi::api::Guid guid =
    { 0x5e8b2a47, 0xc913, 0x4d6f, { 0xb0, 0xa2, 0x3f, 0x7d, 0x9c, 0x4e, 0x6b, 0x18 } };
};

} //namespace synthedit
