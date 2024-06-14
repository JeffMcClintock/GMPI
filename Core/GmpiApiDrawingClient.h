#pragma once

/*
  GMPI - Generalized Music Plugin Interface specification.
  Copyright 2023 Jeff McClintock.

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

// TODO:: filename sync with interface name (IGraphicsClient vs GmpiApiDrawingClient.h)

#include "GmpiApiCommon.h"
#include "GmpiApiDrawing.h"

// Platform specific definitions.
#pragma pack(push,8)

namespace gmpi
{
namespace api
{

// INTERFACE 'Editor' (experimental)
struct DECLSPEC_NOVTABLE IGraphicsClient_x : public IUnknown
{
    virtual ReturnCode render(drawing::api::IDeviceContext* drawingContext) = 0;

    // First pass of layout update. Return minimum size required for given available size
    virtual ReturnCode measure(drawing::SizeU availableSize, drawing::SizeU* returnDesiredSize) = 0;

    // Second pass of layout.
    // TODO: should all rects be passed as pointers (for speed and consistency w D2D). !!!
    virtual ReturnCode arrange(drawing::RectL const* finalRect) = 0; // TODO const, and reference maybe?

    virtual ReturnCode getClipArea(drawing::RectL* returnRect) = 0;

    // {AF8C7A9B-CEAD-4B3F-8E9B-394C43F940BD}
    inline static const Guid guid =
    { 0xaf8c7a9b, 0xcead, 0x4b3f, { 0x8e, 0x9b, 0x39, 0x4c, 0x43, 0xf9, 0x40, 0xbd } };
};

using IGraphicsClient = IGraphicsClient_x;

// Platform specific definitions.
#pragma pack(pop)

} // namespace api
} // namespace gmpi
