#pragma once

/*
  GMPI - Generalized Music Plugin Interface specification.
  Copyright 2007-2024 Jeff McClintock.

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
#include "GmpiApiAudio.h"
#include "GmpiApiEditor.h"

namespace gmpi
{
typedef api::IUnknown* (*CreatePluginPtr)();

gmpi::ReturnCode RegisterPlugin(api::PluginSubtype subType, const char* uniqueId, CreatePluginPtr create);
gmpi::ReturnCode RegisterPluginWithXml(api::PluginSubtype subType, const char* xml, CreatePluginPtr create);

template< class moduleClass >
class Register
{
#if 0 // temp disabled
	inline static int subType(api::IMpUserInterface* /*unused*/)
	{
		return api::MP_SUB_TYPE_GUI;
	}
	inline static int subType(api::IMpUserInterface2* /*unused*/)
	{
		return api::MP_SUB_TYPE_GUI2;
	}
#endif
	inline static api::PluginSubtype subType(api::IController*)
	{
		return api::PluginSubtype::Controller;
	}
	inline static api::PluginSubtype subType(api::IProcessor*)
	{
		return api::PluginSubtype::Audio;
	}
	inline static api::PluginSubtype subType(api::IEditor*)
	{
		return api::PluginSubtype::Editor;
	}
#if 0 // temp disabled
	// Allows unambiguous cast when you support multiple interfaces.
	inline static api::IUnknown* toUnknown(api::IMpController* object) // Controller classes.
	{
		return static_cast<api::IUnknown*>(object);
	}
	inline static api::IUnknown* toUnknown(api::IMpUserInterface2* object) // GUI classes
	{
		return static_cast<api::IUnknown*>(object);
	}
#endif

	inline static api::IUnknown* toUnknown(api::IEditor* object)
	{
		return static_cast<api::IUnknown*>(object);
	}
	inline static api::IUnknown* toUnknown(api::IProcessor* object)
	{
		return static_cast<api::IUnknown*>(object);
	}
	inline static api::IUnknown* toUnknown(api::IController* object)
	{
		return static_cast<api::IUnknown*>(object);
	}

public:
	static bool withId(const char* moduleIdentifier)
	{
		RegisterPlugin(subType((moduleClass*) nullptr), moduleIdentifier,
			[]() -> api::IUnknown* { return toUnknown(new moduleClass()); }
		);

		return false; // value not used, but required.
	}

	static bool withXml(const char* xml)
	{
		RegisterPluginWithXml(subType((moduleClass*) nullptr), xml,
			[]() -> api::IUnknown* { return toUnknown(new moduleClass()); }
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
		enum { result = static_cast<int>(PinDatatype::Int32) };
	};
	template<int N> struct PinDataTypeTraits<bool, N>
	{
		enum { result = static_cast<int>(PinDatatype::Bool) };
	};
	template<int N> struct PinDataTypeTraits<float, N>
	{
		enum { result = static_cast<int>(PinDatatype::Float32) };
	};
	template<int N> struct PinDataTypeTraits<std::string, N>
	{
		enum { result = static_cast<int>(PinDatatype::String) };
	};
	template<int N> struct PinDataTypeTraits<Blob, N>
	{
		enum { result = static_cast<int>(PinDatatype::Blob) };
	};

public:
	enum { PinDataType = PinDataTypeTraits<T, 0>::result };
};


// Get size of variable's data.
template <typename T>
inline int dataSize(const T& /*value*/)
{
	return sizeof(T);
}

template<>
inline int dataSize<std::string>(const std::string& value)
{
	return static_cast<int>(value.size());
}

template<>
inline int dataSize<std::wstring>(const std::wstring& value)
{
	return static_cast<int>(value.size() * sizeof(wchar_t));
}

template<>
inline int dataSize<Blob>(const Blob& value)
{
	return static_cast<int>(value.size());
}

// Serialize variable's value as bytes.
template <typename T>
inline const void* dataPtr(const T& value)
{
	return reinterpret_cast<const void*>(&value);
}

template<>
inline const void* dataPtr<std::string>(const std::string& value)
{
	return reinterpret_cast<const void*>(value.data());
}

template<>
inline const void* dataPtr<std::wstring>(const std::wstring& value)
{
	return reinterpret_cast<const void*>(value.data());
}

template<>
inline const void* dataPtr<Blob>(const Blob& value)
{
	return reinterpret_cast<const void*>(value.data());
}

// De-serialize type.
template <typename T>
inline void valueFromData(int size, const void* data, T& returnValue)
{
	assert(size == sizeof(T) && "check pin datatype matches XML"); // Have you re-scanned modules since last change?
	memcpy(&returnValue, data, size);
}

template <>
inline void valueFromData<Blob>(int size, const void* data, Blob& returnValue)
{
	returnValue.assign((uint8_t*)data, (uint8_t*)data + size);
}

template <>
inline void valueFromData<bool>(int size, const void* data, bool& returnValue)
{
	// bool is passed as int.
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
inline void valueFromData<std::string>(int size, const void* data, std::string& returnValue)
{
	returnValue.assign((const char*)data, size);
}

} // namespace