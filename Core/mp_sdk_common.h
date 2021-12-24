#pragma once

/* Copyright (c) 2007-2021 SynthEdit Ltd
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name SEM, nor SynthEdit, nor 'Music Plugin Interface' nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY SynthEdit Ltd ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL SynthEdit Ltd BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <cassert>
#include <string>

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

// All methods return an error code.
// An int32_t may take one of these values.
enum
{
	MP_HANDLED		= 1,		// Success. In case of GUI - no further handing required.
	MP_OK			= 0,		// Success. In case of GUI indicates successful mouse "hit".
	MP_FAIL			= -1,		// General failure.
	MP_UNHANDLED	= -1,		// Event not handled.
	MP_NOSUPPORT	= -2,		// Interface not supported.
	MP_CANCEL		= -3,		// Async operation cancelled.
};

enum FieldType {
	MP_FT_VALUE
	, MP_FT_SHORT_NAME
	, MP_FT_LONG_NAME
	, MP_FT_MENU_ITEMS
	, MP_FT_MENU_SELECTION
	, MP_FT_RANGE_LO
	, MP_FT_RANGE_HI
	, MP_FT_ENUM_LIST
	, MP_FT_FILE_EXTENSION
	, MP_FT_IGNORE_PROGRAM_CHANGE
	, MP_FT_PRIVATE
	, MP_FT_AUTOMATION				// int
	, MP_FT_AUTOMATION_SYSEX		// string
	, MP_FT_DEFAULT					// same type as parameter
	, MP_FT_GRAB					// (mouse down) bool
	, MP_FT_NORMALIZED				// float
};

//!!! consitancy, each enum on it;s own line or not !!!!

// Silence detection - Audio pins may be in either of two states.
enum MpPinStatus{PIN_STATIC=1, PIN_STREAMING}; // !!!NAMING INCONSISTANT!!! 
enum MP_PinDirection { MP_IN, MP_OUT };
enum MP_PinDatatype { MP_ENUM = 0, MP_STRING = 1, MP_MIDI = 2, MP_FLOAT64, MP_BOOL = 4, MP_AUDIO = 5, MP_FLOAT32 = 6, MP_INT32 = 8, MP_INT64 = 9, MP_BLOB = 10 };

enum MpSubTypes { MP_SUB_TYPE_AUDIO, MP_SUB_TYPE_GUI, MP_SUB_TYPE_GUI2, MP_SUB_TYPE_UNUSED, MP_SUB_TYPE_CONTROLLER };

// GUID - Globally Unique Identifier, used to identify interfaces.
struct MpGuid
{
	uint32_t data1;
	uint16_t data2;
	uint16_t data3;
	unsigned char data4[8];
};

inline bool
operator==(const gmpi::MpGuid& left, const gmpi::MpGuid& right)
{
	return 0 == std::memcmp(&left, &right, sizeof(left));
}

// IMpUnknown Interface.
// This is the base interface.  All objects support these 3 methods.
class DECLSPEC_NOVTABLE IMpUnknown
{
public:
	// Query the object for a supported interface.
	virtual int32_t queryInterface( const MpGuid& iid, void** returnInterface ) = 0;

	// Increment the reference count of an object.
	virtual int32_t addRef() = 0;

	// Decrement the reference count of an object and possibly destroy it.
	virtual int32_t release() = 0;
};

// GUID for IMpUnknown - {00000000-0000-C000-0000-000000000046}
static const MpGuid MP_IID_UNKNOWN =
{
	0x00000000, 0x0000, 0xC000,
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}
};

/* put in factory of sommit
template<typename T>
class getGuid
{
	MpGuid& GuidFor(IMpUnknown*)
	{
		return MP_IID_UNKNOWN;
	}

	MpGuid& GuidFor(class IString*)
	{
		return MP_IID_RETURNSTRING;
	}
	MpGuid& GuidFor(IMpAudioPlugin*)
	{
		return MP_IID_AUDIO_PLUGIN;
	}
public:
	static MpGuid& value()
	{
		return GuidFor((T*) nullptr);
	}
};
*/

class IMpPluginFactory : public IMpUnknown
{
public:
	// Instantiate a plugin.
	virtual int32_t createInstance2(
		const char* id,
		int32_t subType,
		void** returnInterface
	) = 0;
};

// GUID for IMpPluginFactory
// {31DC1CD9-6BDF-412A-B758-B2E5CD1D8870}
static const MpGuid MP_IID_PLUGIN_FACTORY =
{ 0x31dc1cd9, 0x6bdf, 0x412a, { 0xb7, 0x58, 0xb2, 0xe5, 0xcd, 0x1d, 0x88, 0x70 } };

// Plugin factory - holds a list of the plugins available in this dll. Creates plugin instances on request.

// type of function to instantiate a plugin component.
typedef class gmpi::IMpUnknown* ( *MP_CreateFunc2 )();

// generic object register.
int32_t RegisterPlugin(int subType, const char* uniqueId, MP_CreateFunc2 create);
int32_t RegisterPluginWithXml(int subType, const char* xml, MP_CreateFunc2 create);

// When building the plugin as part of a lib
// we need to ensure linker does not discard the plugin from the static-library.
// This macro provides a dummy 'entry point' into the compilation unit that causes the linker to 
// retain the code.
// See also INIT_STATIC_FILE
#define SE_DECLARE_INIT_STATIC_FILE(filename) void se_static_library_init_##filename(){}


// Helper class to make registering concise.
/* e.g.
using namespace gmpi;

namespace
{
	auto r = Register< Oscillator >::withId(L"MY Oscillator");
}
*/
namespace gmpi_gui_api
{
	class IMpGraphics2;
}

class IMpAudioPlugin;

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
	inline static int subType(gmpi::IMpAudioPlugin* /*unused*/)
	{
		return gmpi::MP_SUB_TYPE_AUDIO;
	}
#if 0 // temp disabled

	// Allows unambiguous cast when you support multiple interfaces.
	inline static gmpi::IMpUnknown* toUnknown(gmpi::IMpController* object) // Controller classes.
	{
		return static_cast<gmpi::IMpUnknown*>(object);
	}
	inline static gmpi::IMpUnknown* toUnknown(gmpi::IMpUserInterface2* object) // GUI classes
	{
		return static_cast<gmpi::IMpUnknown*>(object);
	}
#endif

	inline static gmpi::IMpUnknown* toUnknown(gmpi::IMpAudioPlugin* object) // Processor classes.
	{
		return static_cast<gmpi::IMpUnknown*>(object);
	}

public:
	static bool withId(const char* moduleIdentifier)
	{
		RegisterPlugin(subType((moduleClass*) nullptr), moduleIdentifier,
			[]() -> gmpi::IMpUnknown* { return toUnknown(new moduleClass()); }
		);

		return false; // value not used, but required.
	}

	static bool withXml(const char* xml)
	{
		RegisterPluginWithXml(subType((moduleClass*) nullptr), xml,
			[]() -> gmpi::IMpUnknown* { return toUnknown(new moduleClass()); }
		);

		return false;
	}
};

// BLOB - Binary datatype.
struct MpBlob
{
	MpBlob();
	// copy constructor. supports use in standard containers.
	MpBlob( const MpBlob& other )
	{
		Init(other.size_, other.data_);
	}
	// contruct from raw data.
	MpBlob(int size, const void* data)
	{
		Init((size_t) size, data);
	}
	MpBlob(size_t size, const void* data)
	{
		Init(size, data);
	}
	~MpBlob();

	void setValueRaw(size_t size, const void* data);
	void setValueRaw(int size, const void* data)
	{
		setValueRaw((size_t)size, data);
	}
	const MpBlob &operator=( const MpBlob& other );
	bool operator==( const MpBlob& other ) const;
	bool compare( char* data, int size );
	bool operator!=( const MpBlob& other );
	int32_t getSize() const;
	char* getData() const;
	void resize( int size );

private:
	void Init(size_t size, const void* data)
	{
		size_ = size;
		if (size_ > 0)
		{
			data_ = new char[size_];
			memcpy(data_, data, size_);
		}
		else
		{
			data_ = nullptr;
		}
	}

	size_t size_;
	char* data_;
};

// UTILITY classes, macros and helpers.

// safe way to return string from method.
class DECLSPEC_NOVTABLE IString : public IMpUnknown
{
public:
	virtual int32_t setData(const char* pData, int32_t pSize) = 0;
	virtual int32_t getSize() = 0;
	virtual const char* getData() = 0;
};

// GUID for IString
// {AB8FFB21-44FF-42B7-8885-29431399E7E4}
static const MpGuid MP_IID_RETURNSTRING =
{ 0xab8ffb21, 0x44ff, 0x42b7,{ 0x88, 0x85, 0x29, 0x43, 0x13, 0x99, 0xe7, 0xe4 } };

template <typename T>
class MpTypeTraits
{
private:
	// convert type to int representing datatype. N is dummy to satisfy partial specialization rules enforced by GCC.
	template <class U, int N> struct PinDataTypeTraits
	{
	};
	template<int N> struct PinDataTypeTraits<int,N>
	{
		enum { result = gmpi::MP_INT32 };
	};
	template<int N> struct PinDataTypeTraits<bool,N>
	{
		enum { result = gmpi::MP_BOOL };
	};
	template<int N> struct PinDataTypeTraits<float,N>
	{
		enum { result = gmpi::MP_FLOAT32 };
	};
	template<int N> struct PinDataTypeTraits<std::wstring,N>
	{
		enum { result = gmpi::MP_STRING };
	};
	template<int N> struct PinDataTypeTraits<MpBlob,N>
	{
		enum { result = gmpi::MP_BLOB };
	};

public:
	enum{ PinDataType = PinDataTypeTraits<T,0>::result };
};


// Get size of variable's data.
template <typename T>
inline int variableRawSize( const T& /*value*/ )
{
	return sizeof(T);
}

template<>
inline int variableRawSize<std::wstring>( const std::wstring& value )
{
	return (int) sizeof(wchar_t) * (int) value.length();
}

template<>
inline int variableRawSize<MpBlob>( const MpBlob& value )
{
	return value.getSize();
}

// Serialize variable's value as bytes.
template <typename T>
inline void* variableRawData( const T &value )
{
	return (void*) &value;
}

template<>
inline void* variableRawData<std::wstring>( const std::wstring& value )
{
	return (void*) value.data();
}

template<>
inline void* variableRawData<MpBlob>( const MpBlob& value )
{
	return (void*) value.getData();
}


// Compare two instances of a type.
template <typename T>
inline bool variablesAreEqual( const T& a, const T& b )
{
	return a == b;
}

/* ? redundant?
template <>
inline bool variablesAreEqual<std::wstring>( const std::wstring& a, const std::wstring& b )
{
	return a == b;
}
*/

// De-serialize type.
template <typename T>
inline void VariableFromRaw( int size, const void* data, T& returnValue )
{
	assert( size == sizeof(T) && "check pin datatype matches XML" ); // Have you re-scanned modules since last change?
	memcpy( &returnValue, data, size );
}

template <>
inline void VariableFromRaw<struct MpBlob>( int size, const void* data, struct MpBlob& returnValue )
{
	returnValue.setValueRaw( size, data );
}

template <>
inline void VariableFromRaw<bool>( int size, const void* data, bool& returnValue )
{
	// bool is pased as int.
	if( size == 4 ) // DSP sends bool events as int.
	{
		returnValue = *((int*) data) != 0;
	}
	else
	{
		assert( size == 1 );
		returnValue = *((bool*) data);
	}
}

template <>
inline void VariableFromRaw<std::wstring>( int size, const void* data, std::wstring& returnValue )
{
	returnValue.assign( (wchar_t* ) data, size / sizeof(wchar_t) );
}

} // namespace gmpi

namespace gmpi_sdk // TODO !!! decide what goes in 'gmpi_sdk' vs 'gmpi'
{
	// Helper for managing lifetime of interface pointer
	template<class wrappedObjT>
	class mp_shared_ptr
	{
		wrappedObjT* obj;

	public:
		mp_shared_ptr() : obj(0)
		{
		}

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
			assert(obj == 0); // Free it before you re-use it!
			return &obj;
		}
		void** asIMpUnknownPtr()
		{
			assert(obj == 0); // Free it before you re-use it!
			return reinterpret_cast<void**>(&obj);
		}

		bool isNull()
		{
			return obj == 0;
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
}

// Handy macro to save typing.
#define GMPI_QUERYINTERFACE1( INTERFACE_IID, CLASS_NAME ) \
	int32_t queryInterface(const gmpi::MpGuid& iid, void** returnInterface) override \
{ \
	*returnInterface = 0; \
	if (iid == INTERFACE_IID || iid == gmpi::MP_IID_UNKNOWN ) \
{ \
	*returnInterface = static_cast<CLASS_NAME*>(this); \
	addRef(); \
	return gmpi::MP_OK; \
} \
	return gmpi::MP_NOSUPPORT; \
}

#define GMPI_REFCOUNT int refCount2_ = 1; \
	int32_t addRef() override \
{ \
	return ++refCount2_; \
} \
	int32_t release() override \
{ \
	if (--refCount2_ == 0) \
	{ \
	delete this; \
	return 0; \
	} \
	return refCount2_; \
} \

#define GMPI_REFCOUNT_NO_DELETE	\
	int32_t addRef() override \
{ \
	return 1; \
} \
	int32_t release() override \
{ \
	return 1; \
} \

#define GMPI_QUERYINTERFACE2( INTERFACE_IID, CLASS_NAME, BASE_CLASS ) \
	int32_t queryInterface(const gmpi::MpGuid& iid, void** returnInterface) override \
{ \
	*returnInterface = 0; \
	if (iid == INTERFACE_IID ) \
{ \
	*returnInterface = static_cast<CLASS_NAME*>(this); \
	addRef(); \
	return gmpi::MP_OK; \
} \
return BASE_CLASS::queryInterface(iid, returnInterface); \
} \


// back to default structure alignment.
#if defined(_WIN32) || defined(__FLAT__)
#pragma pack(pop)
#elif defined __BORLANDC__
#pragma -a-
#endif
