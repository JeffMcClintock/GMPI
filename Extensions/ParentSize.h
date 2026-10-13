#pragma once
#include "GmpiSdkCommon.h"
#include "RefCountMacros.h"

/*
#include "Extensions/ParentSize.h"
*/

// SynthEdit-specific.
namespace synthedit
{

// Implement this on a plugin/editor and pass it to IParentSize::relayParentSize() to find out
// the current size of whatever is hosting it (its immediate parent view/panel), without needing
// to be measured or arranged itself.
struct DECLSPEC_NOVTABLE IParentSizeClient : gmpi::api::IUnknown
{
    virtual gmpi::ReturnCode onParentSize(float width, float height) = 0;

    // {7E6D5C4B-3A29-4817-9F6E-2B8D4C1A9E3F}
    inline static const gmpi::api::Guid guid =
    { 0x7e6d5c4b, 0x3a29, 0x4817, { 0x9f, 0x6e, 0x2b, 0x8d, 0x4c, 0x1a, 0x9e, 0x3f } };
};

// extension to GMPI to provide support for finding out the size of whatever hosts this plugin,
// bypassing the normal measure()/arrange() dispatch (the plugin need not itself be measured).
struct DECLSPEC_NOVTABLE IParentSize : public gmpi::api::IUnknown
{
public:
    virtual void relayParentSize(IParentSizeClient* client) = 0;

    // {C2F8A6D1-9B4E-4C7A-8D5F-6A1E3B9C7D02}
    inline static const gmpi::api::Guid guid =
    { 0xc2f8a6d1, 0x9b4e, 0x4c7a, { 0x8d, 0x5f, 0x6a, 0x1e, 0x3b, 0x9c, 0x7d, 0x2 } };
};

} //namespace synthedit
