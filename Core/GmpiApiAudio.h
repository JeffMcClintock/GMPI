#pragma once

/*
  GMPI - Generalized Music Plugin Interface specification.
  Copyright 2007-2022 Jeff McClintock.

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
#if defined __BORLANDC__
#pragma -a8
#elif defined(_WIN32) || defined(__FLAT__) || defined (CBUILDER)
#pragma pack(push,8)
#endif

#ifndef DECLSPEC_NOVTABLE
#if defined(__cplusplus)
#define DECLSPEC_NOVTABLE   __declspec(novtable)
#else
#define DECLSPEC_NOVTABLE
#endif
#endif

namespace gmpi2
{

enum class EventType : int32_t
{
    PinSet            = 100, // A parameter has changed value.
    PinStreamingStart = 101, // An input is not silent.
    PinStreamingStop  = 102, // An input is silent.
    Midi              = 103, // A MIDI message.
    GraphStart        = 104, // Plugin is about to process the very first sample.
};

struct Event
{
    int32_t timeDelta;
    EventType eventType;
    int32_t parm1;
    int32_t parm2;
    int32_t parm3;
    int32_t parm4;
    char* extraData;
    Event* next;
};

// INTERFACE 'IAudioPlugin'
struct DECLSPEC_NOVTABLE IAudioPlugin : public IUnknown
{
    virtual ReturnCode open(IUnknown* host) = 0;
    virtual ReturnCode setBuffer(int32_t pinId, float* buffer) = 0;
    virtual void process(int32_t count, const Event* events) = 0;

    // {23835D7E-DCEB-4B08-A9E7-B43F8465939E}
    inline static const Guid guid =
    { 0x23835D7E, 0xDCEB, 0x4B08, { 0xA9, 0xE7, 0xB4, 0x3F, 0x84, 0x65, 0x93, 0x9E} };
};

// INTERFACE 'IAudioPluginHost'
struct DECLSPEC_NOVTABLE IAudioPluginHost : public IUnknown
{
    virtual ReturnCode setPin(int32_t timestamp, int32_t pinId, int32_t size, const void* data) = 0;
    virtual ReturnCode setPinStreaming(int32_t timestamp, int32_t pinId, bool isStreaming) = 0;
    virtual ReturnCode setLatency(int32_t latency) = 0;
    virtual ReturnCode sleep() = 0;
    virtual int32_t getBlockSize() = 0;
    virtual int32_t getSampleRate() = 0;
    virtual int32_t getHandle() = 0;

    // {87CCD426-71D7-414E-A9A6-5ADCA81C7420}
    inline static const Guid guid =
    { 0x87CCD426, 0x71D7, 0x414E, { 0xA9, 0xA6, 0x5A, 0xDC, 0xA8, 0x1C, 0x74, 0x20} };
};

// Platform specific definitions.
#if defined __BORLANDC__
#pragma -a-
#elif defined(_WIN32) || defined(__FLAT__) || defined (CBUILDER)
#pragma pack(pop)
#endif

} // namespace
