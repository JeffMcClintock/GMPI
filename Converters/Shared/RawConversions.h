#pragma once

#include <string>
#include "./MyTypeTraits.h"
#include <assert.h>
// Fix for <sstream> on Mac (sstream uses undefined int_64t)
#include "mp_api.h"
#include <sstream>
#include <vector>
#include "se_datatypes.h" // kill this

// Return actual address of a value's data.
// Advantage: Does NO memory allocation.
template <typename T>
const void* RawData3(const T& value)
{
	return &value;
}

// helpers to convert any parameter value to raw bytes
template <typename T>
int RawSize(const T& /*value*/)
{
	return sizeof(T);
}

int RawSize( const wchar_t* text );

// specialised for strings
template<>
const void* RawData3<std::wstring>(const std::wstring& value);
template<>
int RawSize<std::wstring>(const std::wstring& value);

typedef wchar_t* wide_char_ptr;

template<>
int RawSize<wide_char_ptr>(const wide_char_ptr& value);

//template<>
//int RawSize<std::wstring>(const std::wstring& value);

// specialised for Blobs
template<>
const void* RawData3<struct MpBlob>(const struct MpBlob& value);
template<>
int RawSize<MpBlob>(const struct MpBlob& value);


template <typename T>
inline bool IsVariableSize(const T& value)
{
	return false;
}

// specialised for strings
template<>
inline bool IsVariableSize<std::wstring>(const std::wstring& /*value*/)
{
	return true;
}

template<>
inline bool IsVariableSize<wide_char_ptr>(const wide_char_ptr& /*value*/)
{
	return true;
}

// specialised for Blobs
template<>
inline bool IsVariableSize<MpBlob>(const struct MpBlob& /*value*/)
{
	return true;
}


template <typename T>
T RawToValue(const void* data, [[maybe_unused]] int size)
{
	assert( sizeof(T) == size);
	return * ( (T*) data );
}

template <typename T>
T RawToValue(const void* data, size_t size = sizeof(T))
{
	assert(sizeof(T) == size);
	return *((T*)data);
}

// specialised for string
template<>
std::wstring RawToValue<std::wstring>(const void* data, int size);
template<>
std::wstring RawToValue<std::wstring>(const void* data, size_t size);

// specialised for blob
template<>
MpBlob RawToValue<MpBlob>(const void* data, int size);
template<>
MpBlob RawToValue<MpBlob>(const void* data, size_t size);


// helper to convert any parameter value to raw bytes (returns length too)
// caller to free memory. deprecated
template <typename T>
void RawData2(const T& value, void* *p_data, int* size)
{
	// caller must free memory
	*size = sizeof(T);
//#if defined( _MSC_VER )
//	void* temp = _malloc_dbg( *size, _NORMAL_BLOCK, THIS_FILE, __LINE__ );
//#else
	void* temp = malloc( *size );
//#endif
	memcpy( temp, &value, *size );
	*p_data = temp;
};

// specialised for string
template<>
void RawData2<std::wstring>( const std::wstring& value, void* *p_data, int* size);

// specialised for blob
template<>
void RawData2<MpBlob>( const MpBlob& value, void* *p_data, int* size);

// Raw data to string given datatype.
std::string RawToUtf8B(int datatype, const void* data, size_t size);

// Raw to Wide-string.
template <typename T>
std::wstring RawToString(const void* data, int size = sizeof(T))
{
	assert( sizeof(T) == size);
	T val = * ( (T*) data );
	std::wostringstream oss;
	oss << val;
	return oss.str();
}

// specialised for blob
template<>
std::wstring RawToString<MpBlob>(const void* data, int size);

template<>
std::wstring RawToString<std::wstring>(const void* data, int size);

// Raw to UTF8-string.
template <typename T>
std::string RawToUtf8(const void* data, int size = sizeof(T))
{
	assert(sizeof(T) == size);
	T val = *((T*)data);
	std::ostringstream oss;
	oss << val;
	return oss.str();
}

// specialised for blob
template<>
std::string RawToUtf8<MpBlob>(const void* data, int size);

template<>
std::string RawToUtf8<std::wstring>(const void* data, int size);

std::string ParseToRaw( int datatype, const std::wstring& s );
std::string ParseToRaw( int datatype, const std::string& s );

template<typename T>
std::string ToRaw4( const T& value )
{
	std::string returnRaw;
	returnRaw.resize( sizeof( value ) );
	returnRaw.assign( (char*) &value, sizeof( value ) );
	return returnRaw;
}

template<>
std::string ToRaw4<std::wstring>(const std::wstring& value);

template<>
std::string ToRaw4<MpBlob>( const MpBlob& value );

inline std::string literalToRaw(const wchar_t* value)
{
	std::wstring temp(value);
	return ToRaw4(temp);
}

inline int getDataTypeSize(int datatype)
{
	switch (datatype)
	{
	case DT_CLASS:
	case DT_BLOB:
	case DT_TEXT:
	case DT_STRING_UTF8:
	{
		return 0; // valiable size.
	}
	break;

	case DT_BOOL:
	{
		return sizeof(bool);
	}
	break;

	case DT_INT:
	case DT_ENUM:
	case DT_FLOAT:
	{
		assert(sizeof(float) == sizeof(int32_t));
		return sizeof(int32_t);
	}
	break;

	case DT_DOUBLE:
	case DT_INT64:
	{
		assert(sizeof(double) == sizeof(int64_t));
		return sizeof(int64_t);
	}
	break;

	}

	assert(false); // TODO
	return 0;
}