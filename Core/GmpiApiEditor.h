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

#include "GmpiApiCommon.h"

// Platform specific definitions.
#pragma pack(push,8)

namespace gmpi
{
namespace api
{

// INTERFACE 'Editor' (experimental)
struct DECLSPEC_NOVTABLE IEditor_x : public IUnknown
{
    // Establish connection to host.
    virtual ReturnCode setHost(IUnknown* host) = 0;

    // Object created and connections established.
    virtual ReturnCode initialize() = 0;

    // Host setting a pin due to patch-change or automation.
    virtual ReturnCode setPin(int32_t pinId, int32_t voice, int32_t size, const void* data) = 0;

    // During initialisation it's nesc to notify all input pins.
    // setPin() will not do so in the case where the pin is already set to that value by default (i.e. zero).
    // A second reason is self-notification, which needs to happen only on input pins only, when the module updates them.
    // Given that the module can only detect input pins though costly iteration, better that host handles this case.
    virtual ReturnCode notifyPin(int32_t pinId, int32_t voice) = 0;

    // {E3942893-69B2-401C-8BCA-74650FAAE31C}
    inline static const Guid guid =
    { 0xe3942893, 0x69b2, 0x401c, { 0x8b, 0xca, 0x74, 0x65, 0xf, 0xaa, 0xe3, 0x1c } };
};

// INTERFACE 'IEditorHost' (experimental)
struct DECLSPEC_NOVTABLE IEditorHost_x : public IUnknown
{
    virtual ReturnCode setPin(int32_t pinId, int32_t voice, int32_t size, const void* data) = 0;
    virtual int32_t getHandle() = 0;

    // {8A8CF7DC-BC36-4743-A09A-967571355A1A}
    inline static const Guid guid =
    { 0x8a8cf7dc, 0xbc36, 0x4743, { 0xa0, 0x9a, 0x96, 0x75, 0x71, 0x35, 0x5a, 0x1a } };
};

class IParameterObserver : public IUnknown
{
public:
    virtual ReturnCode setParameter(int32_t parameterHandle, int32_t fieldId, int32_t voice, int32_t size, const void* data) = 0;

    // {7D5AD528-A035-44E5-82A2-4E3A70AEA099}
    inline static const Guid guid =
    { 0x7d5ad528, 0xa035, 0x44e5, { 0x82, 0xa2, 0x4e, 0x3a, 0x70, 0xae, 0xa0, 0x99 } };
};

using IEditor = IEditor_x;
using IEditorHost = IEditorHost_x;

// Platform specific definitions.
#pragma pack(pop)

} // namespace api
} // namespace gmpi
