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

#include <span>
#include "GmpiApiCommon.h"

// Platform specific definitions.
#pragma pack(push,8)

namespace gmpi
{
namespace api
{

enum class EventType : int32_t
{
    PinSet            = 0, // A parameter has changed value.
    PinStreamingStart = 1, // An input is not silent.
    PinStreamingStop  = 2, // An input is silent.
    Midi              = 3, // A MIDI message.
    GraphStart        = 4, // Plugin is about to process the very first sample.
};

#pragma pack(push,4)

struct Event
{
	Event* next;
	int32_t timeDelta;
	EventType eventType;
	int32_t pinIdx;
	int32_t size_;
	union
	{
		uint8_t data_[8];
		const uint8_t* oversizeData_;
	};

    std::span<const uint8_t> payload() const
    {
        const uint8_t* data = size_ > 8 ? oversizeData_ : data_;
        return std::span<const uint8_t>(data, static_cast<size_t>(size_));
	}
};

#pragma pack(pop)

struct DECLSPEC_NOVTABLE IProcessor : IUnknown
{
    virtual ReturnCode open(IUnknown* host) = 0;
    virtual ReturnCode setBuffer(int32_t pinId, float* buffer) = 0;
    virtual void process(int32_t count, const Event* events) = 0;

    // {23835D7E-DCEB-4B08-A9E7-B43F8465939E}
    inline static const Guid guid =
    { 0x23835D7E, 0xDCEB, 0x4B08, { 0xA9, 0xE7, 0xB4, 0x3F, 0x84, 0x65, 0x93, 0x9E} };
};

struct DECLSPEC_NOVTABLE IProcessorHost : IUnknown
{
    virtual ReturnCode setPin(int32_t timestamp, int32_t pinId, int32_t size, const uint8_t* data) = 0;
    virtual ReturnCode setPinStreaming(int32_t timestamp, int32_t pinId, bool isStreaming) = 0;
    virtual ReturnCode setLatency(int32_t latency) = 0;
    virtual ReturnCode sleep() = 0;
    virtual int32_t getBlockSize() = 0;
    virtual float getSampleRate() = 0;
    virtual int32_t getHandle() = 0;

    // {87CCD426-71D7-414E-A9A6-5ADCA81C7420}
    inline static const Guid guid =
    { 0x87CCD426, 0x71D7, 0x414E, { 0xA9, 0xA6, 0x5A, 0xDC, 0xA8, 0x1C, 0x74, 0x20} };
};

// Platform specific definitions.
#pragma pack(pop)

} // namespace api
} // namespace gmpi
