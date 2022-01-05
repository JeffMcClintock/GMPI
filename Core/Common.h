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
#include <string>
#include <vector>

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

namespace gmpi
{

enum class ReturnCode : int32_t
{
    Ok        = 0,  // Success.
    Handled   = 1,  // Success, no further handing required.
    Fail      = -1, // General failure.
    Unhandled = -1, // Event not handled.
    NoSupport = -2, // Interface not supported.
    Cancel    = -3, // Async operation cancelled.
};

enum class PluginSubtype : int32_t
{
    Audio      = 0, // An audio processor object.
    Editor     = 2, // A graphical editor object.
    Controller = 4, // A controller object.
};

enum class PinDirection : int32_t
{
	In,
	Out,
};

enum class PinDatatype : int32_t
{
	Enum,
	String,
	Midi,
	Float64,
	Bool,
	Audio,
	Float32,
	Int32 = 8,
	Int64,
	Blob,
};

struct Guid
{
	uint32_t data1;
	uint16_t data2;
	uint16_t data3;
	uint8_t data4[8];
};

inline bool operator==(const gmpi::Guid& left, const gmpi::Guid& right)
{
	return 0 == std::memcmp(&left, &right, sizeof(left));
}


// INTERFACE 'IUnknown'
struct DECLSPEC_NOVTABLE IUnknown
{
	virtual ReturnCode queryInterface(const Guid* iid, void** returnInterface) = 0;
	virtual int32_t addRef() = 0;
	virtual int32_t release() = 0;
};

// {00000000-0000-C000-0000-000000000046}
static const Guid IID_UNKNOWN =
{ 0x00000000, 0x0000, 0xC000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };


// INTERFACE 'IString'
struct DECLSPEC_NOVTABLE IString : public IUnknown
{
    virtual ReturnCode setData(const char* data, int32_t size) = 0;
    virtual ReturnCode getSize() = 0;
    virtual const char* getData() = 0;

    // {AB8FFB21-44FF-42B7-8885-29431399E7E4}
    inline static const Guid guid =
    { 0xAB8FFB21, 0x44FF, 0x42B7, { 0x88, 0x85, 0x29, 0x43, 0x13, 0x99, 0xE7, 0xE4} };
};


// INTERFACE 'IPluginFactory'
struct DECLSPEC_NOVTABLE IPluginFactory : public IUnknown
{
	virtual ReturnCode createInstance(const char* id, PluginSubtype subtype, void** returnInterface) = 0;
	virtual ReturnCode getPluginInformation(int32_t index, IString* returnXml) = 0;

	inline static const Guid guid = // {066C55EB-0EA8-4D73-A6F3-06D948D9E232}
	{ 0x66c55eb, 0xea8, 0x4d73, { 0xa6, 0xf3, 0x6, 0xd9, 0x48, 0xd9, 0xe2, 0x32 } };
};
} // namespace


// Helper class to make registering concise.
/* e.g.
using namespace gmpi;

namespace
{
	auto r = Register< Oscillator >::withId(L"MY Oscillator");
}
*/

class IAudioPlugin;
namespace gmpi_gui_api
{
	class IMpGraphics2;
}

namespace gmpi_sdk
{
// type of function to instantiate a plugin component.
typedef struct gmpi::IUnknown* (*CreatePluginPtr)();
}

namespace gmpi
{

gmpi::ReturnCode RegisterPlugin(gmpi::PluginSubtype subType, const char* uniqueId, gmpi_sdk::CreatePluginPtr create);
gmpi::ReturnCode RegisterPluginWithXml(gmpi::PluginSubtype subType, const char* xml, gmpi_sdk::CreatePluginPtr create);


template< class moduleClass >
class Register
{
#if 0 // temp disabled
	inline static int subType(gmpi::IMpUserInterface* /*unused*/)
	{
		return gmpi::MP_SUB_TYPE_GUI;
	}
	inline static int subType(gmpi::IMpUserInterface2* /*unused*/)
	{
		return gmpi::MP_SUB_TYPE_GUI2;
	}
	inline static int subType(gmpi::IMpController* /*unused*/)
	{
		return gmpi::MP_SUB_TYPE_CONTROLLER;
	}
#endif
	inline static gmpi::PluginSubtype subType(gmpi::IAudioPlugin* /*unused*/)
	{
		return gmpi::PluginSubtype::Audio;
	}
#if 0 // temp disabled

	// Allows unambiguous cast when you support multiple interfaces.
	inline static gmpi::IUnknown* toUnknown(gmpi::IMpController* object) // Controller classes.
	{
		return static_cast<gmpi::IUnknown*>(object);
	}
	inline static gmpi::IUnknown* toUnknown(gmpi::IMpUserInterface2* object) // GUI classes
	{
		return static_cast<gmpi::IUnknown*>(object);
	}
#endif

	inline static gmpi::IUnknown* toUnknown(gmpi::IAudioPlugin* object) // Processor classes.
	{
		return static_cast<gmpi::IUnknown*>(object);
	}

public:
	static bool withId(const char* moduleIdentifier)
	{
		gmpi::RegisterPlugin(subType((moduleClass*) nullptr), moduleIdentifier,
			[]() -> gmpi::IUnknown* { return toUnknown(new moduleClass()); }
		);

		return false; // value not used, but required.
	}

	static bool withXml(const char* xml)
	{
		gmpi::RegisterPluginWithXml(subType((moduleClass*) nullptr), xml,
			[]() -> gmpi::IUnknown* { return toUnknown(new moduleClass()); }
		);

		return false;
	}
};

// BLOB - Binary datatype.
typedef std::vector<uint8_t> Blob;
} // namespace

namespace gmpi_sdk
{
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
			enum { result = static_cast<int>(gmpi::PinDatatype::Int32) };
		};
		template<int N> struct PinDataTypeTraits<bool, N>
		{
			enum { result = static_cast<int>(gmpi::PinDatatype::Bool) };
		};
		template<int N> struct PinDataTypeTraits<float, N>
		{
			enum { result = static_cast<int>(gmpi::PinDatatype::Float32) };
		};
		template<int N> struct PinDataTypeTraits<std::wstring, N>
		{
			enum { result = static_cast<int>(gmpi::PinDatatype::String) };
		};
		template<int N> struct PinDataTypeTraits<gmpi::Blob, N>
		{
			enum { result = static_cast<int>(gmpi::PinDatatype::Blob) };
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
	inline int variableRawSize<std::wstring>(const std::wstring& value)
	{
		return (int)sizeof(wchar_t) * (int)value.length();
	}

	template<>
	inline int variableRawSize<gmpi::Blob>(const gmpi::Blob& value)
	{
		return value.size();
	}

	// Serialize variable's value as bytes.
	template <typename T>
	inline void* variableRawData(const T& value)
	{
		return (void*)&value;
	}

	template<>
	inline void* variableRawData<std::wstring>(const std::wstring& value)
	{
		return (void*)value.data();
	}

	template<>
	inline void* variableRawData<gmpi::Blob>(const gmpi::Blob& value)
	{
		return (void*)value.data();
	}

	// De-serialize type.
	template <typename T>
	inline void VariableFromRaw(int size, const void* data, T& returnValue)
	{
		assert(size == sizeof(T) && "check pin datatype matches XML"); // Have you re-scanned modules since last change?
		memcpy(&returnValue, data, size);
	}

	template <>
	inline void VariableFromRaw<gmpi::Blob>(int size, const void* data, gmpi::Blob& returnValue)
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
	inline void VariableFromRaw<std::wstring>(int size, const void* data, std::wstring& returnValue)
	{
		returnValue.assign((wchar_t*)data, size / sizeof(wchar_t));
	}

	// Helper for managing lifetime of reference-counted interface pointer
	template<class wrappedObjT>
	class mp_shared_ptr
	{
		wrappedObjT* obj = {};

	public:
		mp_shared_ptr(){}

		explicit mp_shared_ptr(wrappedObjT* newobj) : obj(0)
		{
			Assign(newobj);
		}
		mp_shared_ptr(const mp_shared_ptr<wrappedObjT>& value) : obj(0)
		{
			Assign(value.obj);
		}
		// Attach object without incrementing ref count. For objects created with new.
		void Attach(wrappedObjT* newobj)
		{
			wrappedObjT* old = obj;
			obj = newobj;

			if( old )
			{
				old->release();
			}
		}

		~mp_shared_ptr()
		{
			if( obj )
			{
				obj->release();
			}
		}
		operator wrappedObjT*( )
		{
			return obj;
		}
		const wrappedObjT* operator=( wrappedObjT* value )
		{
			Assign(value);
			return value;
		}
		mp_shared_ptr<wrappedObjT>& operator=( mp_shared_ptr<wrappedObjT>& value )
		{
			Assign(value.get());
			return *this;
		}
		bool operator==( const wrappedObjT* other ) const
		{
			return obj == other;
		}
		bool operator==( const mp_shared_ptr<wrappedObjT>& other ) const
		{
			return obj == other.obj;
		}
		wrappedObjT* operator->( ) const
		{
			return obj;
		}

		wrappedObjT*& get()
		{
			return obj;
		}

		wrappedObjT** getAddressOf()
		{
			assert(obj == nullptr); // Free it before you re-use it!
			return &obj;
		}
		void** asIMpUnknownPtr()
		{
			assert(obj == 0); // Free it before you re-use it!
			return reinterpret_cast<void**>(&obj);
		}

		bool isNull()
		{
			return obj == nullptr;
		}

	private:
		// Attach object and increment ref count.
		void Assign(wrappedObjT* newobj)
		{
			Attach(newobj);
			if( newobj )
			{
				newobj->addRef();
			}
		}
	};

// macros to save typing the reference counting.
#define GMPI_QUERYINTERFACE( INTERFACE_IID, CLASS_NAME ) \
	gmpi::ReturnCode queryInterface(const gmpi::Guid* iid, void** returnInterface) override{ \
	*returnInterface = 0; \
	if ((*iid) == INTERFACE_IID || (*iid) == gmpi::IID_UNKNOWN ){ \
	*returnInterface = static_cast<CLASS_NAME*>(this); addRef(); \
	return gmpi::ReturnCode::Ok;} \
	return gmpi::ReturnCode::NoSupport;}

#define GMPI_REFCOUNT int refCount2_ = 1; \
	int32_t addRef() override {return ++refCount2_;} \
	int32_t release() override {if (--refCount2_ == 0){delete this;return 0;}return refCount2_;}

#define GMPI_REFCOUNT_NO_DELETE \
	int32_t addRef() override{return 1;} \
	int32_t release() override {return 1;}
}

// back to default structure alignment.
#if defined(_WIN32) || defined(__FLAT__)
#pragma pack(pop)
#elif defined __BORLANDC__
#pragma -a-
#endif
