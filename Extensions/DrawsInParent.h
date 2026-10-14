#pragma once
#include "GmpiSdkCommon.h"
#include "RefCountMacros.h"

/*
#include "Extensions/DrawsInParent.h"
*/

// SynthEdit-specific.
namespace synthedit
{

// A host exposes this only when the module is in a panel sub-view, where the module should use its
// 'Panel Rect' pin in place of its own bounds. The Panel Rect is in the sub-view's visible coords:
// origin at its top-left, the same space as the size IParentSize reports.
// The answer must not change for the host object's lifetime.
struct DECLSPEC_NOVTABLE IDrawsInParent : public gmpi::api::IUnknown
{
public:
    // the module's top-left in the sub-view's visible coords. Subtract it from the Panel Rect to get local coords.
    virtual gmpi::ReturnCode getPositionInParent(float* returnX, float* returnY) = 0;

    // {4B9E2D17-6C3A-4F85-A1D9-8E7C5B2F0A64}
    inline static const gmpi::api::Guid guid =
    { 0x4b9e2d17, 0x6c3a, 0x4f85, { 0xa1, 0xd9, 0x8e, 0x7c, 0x5b, 0x2f, 0x0a, 0x64 } };
};

} //namespace synthedit
