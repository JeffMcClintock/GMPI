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

#include <cstdint>
#include <cassert>
#include <vector>
#include <string>
#include "GmpiSdkCommon.h"

namespace gmpi2
{
	struct IAudioPlugin;
}

namespace gmpi
{

typedef gmpi2::IUnknown* (*CreatePluginPtr)();

gmpi::ReturnCode RegisterPlugin(gmpi2::PluginSubtype subType, const char* uniqueId, CreatePluginPtr create);
gmpi::ReturnCode RegisterPluginWithXml(gmpi2::PluginSubtype subType, const char* xml, CreatePluginPtr create);

template< class moduleClass >
class Register
{
#if 0 // temp disabled
	inline static int subType(gmpi2::IMpUserInterface* /*unused*/)
	{
		return gmpi2::MP_SUB_TYPE_GUI;
	}
	inline static int subType(gmpi2::IMpUserInterface2* /*unused*/)
	{
		return gmpi2::MP_SUB_TYPE_GUI2;
	}
	inline static int subType(gmpi2::IMpController* /*unused*/)
	{
		return gmpi2::MP_SUB_TYPE_CONTROLLER;
	}
#endif
	inline static gmpi2::PluginSubtype subType(gmpi2::IAudioPlugin*)
	{
		return gmpi2::PluginSubtype::Audio;
	}
#if 0 // temp disabled

	// Allows unambiguous cast when you support multiple interfaces.
	inline static gmpi2::IUnknown* toUnknown(gmpi2::IMpController* object) // Controller classes.
	{
		return static_cast<gmpi2::IUnknown*>(object);
	}
	inline static gmpi2::IUnknown* toUnknown(gmpi2::IMpUserInterface2* object) // GUI classes
	{
		return static_cast<gmpi2::IUnknown*>(object);
	}
#endif

	inline static gmpi2::IUnknown* toUnknown(gmpi2::IAudioPlugin* object) // Processor classes.
	{
		return static_cast<gmpi2::IUnknown*>(object);
	}

public:
	static bool withId(const char* moduleIdentifier)
	{
		RegisterPlugin(subType((moduleClass*) nullptr), moduleIdentifier,
			[]() -> gmpi2::IUnknown* { return toUnknown(new moduleClass()); }
		);

		return false; // value not used, but required.
	}

	static bool withXml(const char* xml)
	{
		RegisterPluginWithXml(subType((moduleClass*) nullptr), xml,
			[]() -> gmpi2::IUnknown* { return toUnknown(new moduleClass()); }
		);

		return false;
	}
};

// BLOB - Binary datatype.
typedef std::vector<uint8_t> Blob;

template <typename T>
class PinTypeTraits
{
private:
	// convert type to int representing datatype. N is dummy to satisfy partial specialization rules enforced by GCC.
	template <class U, int N> struct PinDataTypeTraits
	{
	};
	template<int N> struct PinDataTypeTraits<int, N>
	{
		enum { result = static_cast<int>(gmpi2::PinDatatype::Int32) };
	};
	template<int N> struct PinDataTypeTraits<bool, N>
	{
		enum { result = static_cast<int>(gmpi2::PinDatatype::Bool) };
	};
	template<int N> struct PinDataTypeTraits<float, N>
	{
		enum { result = static_cast<int>(gmpi2::PinDatatype::Float32) };
	};
	template<int N> struct PinDataTypeTraits<std::string, N>
	{
		enum { result = static_cast<int>(gmpi2::PinDatatype::String) };
	};
	template<int N> struct PinDataTypeTraits<Blob, N>
	{
		enum { result = static_cast<int>(gmpi2::PinDatatype::Blob) };
	};

public:
	enum { PinDataType = PinDataTypeTraits<T, 0>::result };
};


// Get size of variable's data.
template <typename T>
inline int variableRawSize(const T& /*value*/)
{
	return sizeof(T);
}

template<>
inline int variableRawSize<std::string>(const std::string& value)
{
	return static_cast<int>(value.size());
}

template<>
inline int variableRawSize<Blob>(const Blob& value)
{
	return static_cast<int>(value.size());
}

// Serialize variable's value as bytes.
template <typename T>
inline const void* variableRawData(const T& value)
{
	return reinterpret_cast<const void*>(&value);
}

template<>
inline const void* variableRawData<std::string>(const std::string& value)
{
	return reinterpret_cast<const void*>(value.data());
}

template<>
inline const void* variableRawData<Blob>(const Blob& value)
{
	return reinterpret_cast<const void*>(value.data());
}

// De-serialize type.
template <typename T>
inline void VariableFromRaw(int size, const void* data, T& returnValue)
{
	assert(size == sizeof(T) && "check pin datatype matches XML"); // Have you re-scanned modules since last change?
	memcpy(&returnValue, data, size);
}

template <>
inline void VariableFromRaw<Blob>(int size, const void* data, Blob& returnValue)
{
	returnValue.assign((uint8_t*)data, (uint8_t*)data + size);
}

template <>
inline void VariableFromRaw<bool>(int size, const void* data, bool& returnValue)
{
	// bool is pased as int.
	if (size == 4) // DSP sends bool events as int.
	{
		returnValue = *((int*)data) != 0;
	}
	else
	{
		assert(size == 1);
		returnValue = *((bool*)data);
	}
}

template <>
inline void VariableFromRaw<std::string>(int size, const void* data, std::string& returnValue)
{
	returnValue.assign((const char*)data, size);
}

} // namespace