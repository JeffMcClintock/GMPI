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
    enum class Field
    {
          Value
        , ShortName
        , LongName
        , MenuItems
        , MenuSelection
        , RangeLo
        , RangeHi
        , EnumList
        , FileExtension
        , IgnoreProgramChange
        , Private
        , Automation				// int
        , AutomationSysex			// STRING
        , Default					// same type as parameter
        , Grab						// (mouse down) bool
        , Normalized				// float
        , Stateful				    // bool
    };

namespace api
{

#if 0 // ref: IEditor2_x
// INTERFACE (experimental)
struct DECLSPEC_NOVTABLE IDeterministicUi : IUnknown
{
    virtual ReturnCode step() = 0;

    // {51A635CA-7A2A-4BC8-8F4F-78BA9A574E25}
    inline static const Guid guid =
    { 0x51a635ca, 0x7a2a, 0x4bc8, { 0x8f, 0x4f, 0x78, 0xba, 0x9a, 0x57, 0x4e, 0x25 } };
};
#endif

// INTERFACE 'Editor' (experimental)
struct DECLSPEC_NOVTABLE IEditor_x : IUnknown
{
    // Establish connection to host.
    virtual ReturnCode setHost(IUnknown* host) = 0;

    // Object created and connections established.
    virtual ReturnCode initialize() = 0;

    // Host setting a pin due to patch-change or automation.
    virtual ReturnCode setPin(int32_t PinIndex, int32_t voice, int32_t size, const uint8_t* data) = 0;

    // During initialisation it's nesc to notify all input pins.
    // setPin() will not do so in the case where the pin is already set to that value by default (i.e. zero).
    // A second reason is self-notification, which needs to happen only on input pins only, when the module updates them.
    // Given that the module can only detect input pins though costly iteration, better that host handles this case.
    virtual ReturnCode notifyPin(int32_t PinIndex, int32_t voice) = 0;

    // {E3942893-69B2-401C-8BCA-74650FAAE31C}
    inline static const Guid guid =
    { 0xe3942893, 0x69b2, 0x401c, { 0x8b, 0xca, 0x74, 0x65, 0xf, 0xaa, 0xe3, 0x1c } };
};

// experimental uni-directional signal-flow.
struct DECLSPEC_NOVTABLE IEditor2_x : IUnknown
{
    virtual ReturnCode process() = 0;

    // {593ABAD0-295A-419C-B34A-68D4B6ACF472}
    inline static const Guid guid =
    { 0x593abad0, 0x295a, 0x419c, { 0xb3, 0x4a, 0x68, 0xd4, 0xb6, 0xac, 0xf4, 0x72 } };
};
// and host
struct DECLSPEC_NOVTABLE IEditorHost2_x : IUnknown
{
    virtual ReturnCode setDirty() = 0;

    // {89C64A37-E10E-42A3-AC3D-44A58BC60674}
    inline static const Guid guid =
    { 0x89c64a37, 0xe10e, 0x42a3, { 0xac, 0x3d, 0x44, 0xa5, 0x8b, 0xc6, 0x6, 0x74 } };
};


// INTERFACE 'IEditorHost' (experimental)
struct DECLSPEC_NOVTABLE IEditorHost_x : IUnknown
{
    virtual ReturnCode setPin(int32_t PinIndex, int32_t voice, int32_t size, const uint8_t* data) = 0;
    virtual int32_t getHandle() = 0;

    // {8A8CF7DC-BC36-4743-A09A-967571355A1A}
    inline static const Guid guid =
    { 0x8a8cf7dc, 0xbc36, 0x4743, { 0xa0, 0x9a, 0x96, 0x75, 0x71, 0x35, 0x5a, 0x1a } };
};

struct DECLSPEC_NOVTABLE IParameterObserver : IUnknown
{
public:
    virtual ReturnCode setParameter(int32_t parameterHandle, gmpi::Field fieldId, int32_t voice, int32_t size, const uint8_t* data) = 0;

    // {7D5AD528-A035-44E5-82A2-4E3A70AEA099}
    inline static const Guid guid =
    { 0x7d5ad528, 0xa035, 0x44e5, { 0x82, 0xa2, 0x4e, 0x3a, 0x70, 0xae, 0xa0, 0x99 } };
};

// Controller interface.
struct DECLSPEC_NOVTABLE IController_x : IParameterObserver
{
public:
    // Establish connection to host.
    virtual ReturnCode initialize(gmpi::api::IUnknown* host, int32_t handle) = 0;
#if 0
    // Patch-Manager -> plugin.
    virtual ReturnCode preSaveState() = 0;
    virtual ReturnCode open() = 0;

    // Pins.
    virtual ReturnCode setPinDefault(int32_t pinType, int32_t pinId, const char* defaultValue) = 0;
    virtual ReturnCode setPin(int32_t pinId, int32_t voice, int64_t size, const uint8_t* data) = 0;
    virtual ReturnCode notifyPin(int32_t pinId, int32_t voice) = 0;

    virtual ReturnCode onDelete() = 0;
#endif

    // {B379BA45-E486-4545-8D91-4CE11C4811DE}
    inline static const Guid guid =
    { 0xb379ba45, 0xe486, 0x4545, { 0x8d, 0x91, 0x4c, 0xe1, 0x1c, 0x48, 0x11, 0xde } };
};

struct DECLSPEC_NOVTABLE IControllerHost_x : IUnknown
{
public:
    virtual ReturnCode getParameterHandle(int32_t moduleParameterId, int32_t& returnHandle) = 0;
    virtual ReturnCode setParameter(int32_t parameterHandle, gmpi::Field fieldId, int32_t voice, int32_t size, const uint8_t* data) = 0;
#if 0
    // Each plugin instance has a host-assigned unique handle shared by UI and Audio class.
    virtual ReturnCode getHandle(int32_t& returnHandle) = 0;
    virtual void updateParameter(int32_t parameterHandle, int32_t paramFieldType, int32_t voice) = 0;
    virtual ReturnCode getParameterHandle(int32_t moduleHandle, int32_t moduleParameterId) = 0;
    virtual ReturnCode getParameterModuleAndParamId(int32_t parameterHandle, int32_t* returnModuleHandle, int32_t* returnModuleParameterId) = 0;
    //?	virtual ReturnCode setPinDefault(int32_t pinIndex, int32_t paramFieldType, const char* utf8Value) = 0;
    virtual ReturnCode setLatency(int32_t latency) = 0;

    // Backdoor to Audio class. Not recommended. Use Parameters instead to support proper automation.
    //virtual ReturnCode sendMessageToAudio(int32_t id, int32_t size, const void* messageData) = 0;
    virtual ReturnCode createControllerIterator(gmpi::IMpControllerIterator** returnIterator) = 0;
    virtual ReturnCode pinTransmit(int32_t pinId, int32_t voice, int64_t size, const uint8_t* data) = 0;
#endif

    // {CD0F9C61-E546-47E2-A31C-09B3FBC8F5D0}
    inline static const Guid guid =
    { 0xcd0f9c61, 0xe546, 0x47e2, { 0xa3, 0x1c, 0x9, 0xb3, 0xfb, 0xc8, 0xf5, 0xd0 } };
};

struct DECLSPEC_NOVTABLE IParameterSetter_x : IUnknown
{
public:
    virtual ReturnCode getParameterHandle(int32_t moduleParameterId, int32_t& returnHandle) = 0;
    virtual ReturnCode setParameter(int32_t parameterHandle, gmpi::Field fieldId, int32_t voice, int32_t size, const uint8_t* data) = 0;

    // {CC7A7938-2CD0-4413-8CA8-11781B2E89B2}
    inline static const Guid guid =
    { 0xcc7a7938, 0x2cd0, 0x4413, { 0x8c, 0xa8, 0x11, 0x78, 0x1b, 0x2e, 0x89, 0xb2 } };
};

using IEditor = IEditor_x;
using IEditorHost = IEditorHost_x;
using IController = IController_x;
using IControllerHost = IControllerHost_x;



// Platform specific definitions.
#pragma pack(pop)

} // namespace api
} // namespace gmpi
