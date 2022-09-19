//#include "pch.h"
#include "RawConversions.h"
#include "Base64.h"
#include "se_datatypes.h"

// caller must free memory
template<>
void RawData2(const std::wstring& value, void* *p_data, int* size)
{
	std::wstring temp = ( value );
	*size = (int)( sizeof(wchar_t) * temp.length() );
	void* temp2 = malloc( *size );
	memcpy( temp2, temp.c_str(), *size );
	*p_data = temp2;
};

template<>
void RawData2(const MpBlob& value, void* *p_data, int* size)
{
	*size = value.getSize();
	void* temp2 = malloc( *size );
	memcpy( temp2, value.getData(), *size );
	*p_data = temp2;
}

template<>
int RawSize(const std::wstring& value)
{
	return (int) (sizeof(wchar_t) * value.size());
}

typedef wchar_t* wide_char_ptr;

template<>
const void* RawData3<std::wstring>( const std::wstring& value )
{
	return value.data();
};

template<>
int RawSize<wide_char_ptr>(const wide_char_ptr& value)
{
	return (int) (sizeof(wchar_t) * wcslen(value));
}


template<>
const void* RawData3<struct MpBlob>(const struct MpBlob& value)
{
	return value.getData();
}

template<>
int RawSize<MpBlob>(const struct MpBlob& value)
{
	return value.getSize();
}

/*
template<>
void RawData2(std::wstring value, void* *p_data, int *size)
{
/ *
	sta tic std::wstring temp; // VST prob??!!!!!!

	temp = ( value );

	*p_data =  (void*) temp.c_str();
	*size = sizeof(wchar_t) * ( temp.size() + 1);
* /
	std::wstring temp = ( value );

	// caller must free memory
	*size = sizeof(wchar_t) * ( temp.length() + 1);
	void* temp2 = mal loc( *size );
	memcpy( temp2, temp.c_str(), *size );

	*p_data = temp2;

};
*/
template<>
std::wstring RawToValue(const void* data, int size)
{
	std::wstring temp;

	if (size > 0)
		temp.assign((wchar_t*)data, size / sizeof(wchar_t));

	return (temp);
}

template<>
std::wstring RawToValue(const void* data, size_t size)
{
	std::wstring temp;

	if (size > 0)
		temp.assign((wchar_t*)data, size / sizeof(wchar_t));

	return (temp);
}

template<>
MpBlob RawToValue(const void* data, int size)
{
	MpBlob temp;
	temp.setValueRaw(size, data);
	return temp;
}

template<>
MpBlob RawToValue(const void* data, size_t size)
{
	MpBlob temp;
	temp.setValueRaw(size, data);
	return temp;
}

int RawSize( const wchar_t* text )
{
	return (int) (wcslen( text ) * sizeof( wchar_t));
}

// specialised for blob
template<>
std::wstring RawToString<MpBlob>(const void* data, int size)
{
	return L"<Blob>";
}

template<>
std::wstring RawToString<std::wstring>(const void* data, int size)
{
	std::wstring v( (wchar_t*) data, size / sizeof(wchar_t) );
	return v;
}

// specialised for blob
template<>
std::string RawToUtf8<MpBlob>(const void* data, int size)
{
	return "<Blob>";
}

template<>
std::string RawToUtf8<std::wstring>(const void* data, int size)
{
	std::wstring v((wchar_t*)data, size / sizeof(wchar_t));
	return WStringToUtf8(v);
}

std::string ParseToRaw( int datatype, const std::wstring& s )
{
	std::string result;

	switch( datatype )
	{
	case DT_FSAMPLE:
	case DT_FLOAT:
		result.resize( sizeof( float ) );
		*(float*) ( &result[0] ) = (float) wcstod( s.c_str(), 0 );
		break;

	case DT_DOUBLE:
		result.resize( sizeof( double ) );
		*(double*) ( &result[0] ) = wcstod( s.c_str(), 0 );
		break;

	case DT_ENUM:
		result.resize( sizeof( short ) );
		*(short*) ( &result[0] ) = (short) wcstol( s.c_str(), 0, 10 );
		break;

	case DT_INT:
		result.resize( sizeof( int32_t ) );
		*(int32_t*) ( &result[0] ) = wcstol( s.c_str(), 0, 10 );
		break;

	case DT_TEXT:
		result.resize( sizeof(wchar_t) * s.size() );
		memcpy( &result[0], s.data(), result.size() );
		break;

	case DT_STRING_UTF8:
		result.resize( s.size() );
		memcpy( &result[0], s.data(), result.size() );
		break;

	case DT_BOOL:
		result.resize( sizeof( bool ) );
		*(bool*) ( &result[0] ) = 0 != wcstol( s.c_str(), 0, 10 );
		break;

	case DT_BLOB:
		result = Base64::decode(WStringToUtf8(s));
		break;

	case DT_CLASS: // currently can't convert string to class
		break;

	default:
		assert( false ); // you are using an undefined datatype
	}

	return result;
}

std::string ParseToRaw( int datatype, const std::string& s )
{
	std::string result;

	switch( datatype )
	{
	case DT_FSAMPLE:
	case DT_FLOAT:
		result.resize( sizeof( float ) );
		*(float*) ( &result[0] ) = (float) strtod( s.c_str(), 0 );
		break;

	case DT_DOUBLE:
		result.resize( sizeof( double ) );
		*(double*) ( &result[0] ) = strtod( s.c_str(), 0 );
		break;

	case DT_ENUM:
		result.resize( sizeof( short ) );
		*(short*) ( &result[0] ) = (short) strtol( s.c_str(), 0, 10 );
		break;

	case DT_INT:
		result.resize( sizeof( int32_t ) );
		*(int32_t*) ( &result[0] ) = strtol( s.c_str(), 0, 10 );
		break;

	case DT_TEXT:
		{
			std::wstring wide = Utf8ToWstring( s );
			result.resize( sizeof(wchar_t) * wide.size() );
			memcpy( &result[0], wide.data(), result.size() );
		}
		break;

	case DT_STRING_UTF8:
		result.resize( s.size() );
		memcpy( &result[0], s.data(), result.size() );
		break;

	case DT_BOOL:
		result.resize( sizeof( bool ) );
		*(bool*) ( &result[0] ) = 0 != strtol( s.c_str(), 0, 10 );
		break;

	case DT_BLOB:
		result = Base64::decode(s);
		break;

	default:
		assert( false ); // you are using an undefined datatype
	}

	return result;
}

template<>
std::string ToRaw4<std::wstring>( const std::wstring& value )
{
	std::string returnRaw;

	size_t size = value.size() * sizeof( value[0] );

	returnRaw.resize( size );
	returnRaw.assign( (char*) value.data(), size );
	return returnRaw;
}

template<>
std::string ToRaw4<MpBlob>( const MpBlob& value )
{
	std::string returnRaw;

	returnRaw.resize( value.getSize() );
	returnRaw.assign( (char*) value.getData(), value.getSize() );
	return returnRaw;
}

std::string RawToUtf8B(int datatype, const void* data, size_t size)
{
	std::string result;

	switch (datatype)
	{
	case DT_FSAMPLE:
	case DT_FLOAT:
		result = RawToUtf8<float>(data, size);
		break;

	case DT_DOUBLE:
		result = RawToUtf8<double>(data, size);
		break;

	case DT_ENUM:
		result = RawToUtf8<short>(data, size);
		break;

	case DT_INT:
		result = RawToUtf8<int32_t>(data, size);
		break;

		case DT_TEXT:
		{
			std::wstring v((wchar_t*)data, size / sizeof(wchar_t));
			result = WStringToUtf8(v);
		}
		break;

	case DT_STRING_UTF8:
		result.assign((const char*)data, (size_t) size);
		break;

	case DT_BOOL:
		result = RawToUtf8<bool>(data, size);
		break;

	case DT_BLOB:
		{
			std::string s((char*)data, size);
			result = Base64::encode(s);
		}
		break;

	default:
		assert(false); // you are using an undefined datatype
	}

	return result;
}

/*
template<>
void* RawData<wide_char_ptr>(const wide_char_ptr& value)
{
//	std::wstring temp( value );
// caller must free memory
//	int size = sizeof(wchar_t) * ( temp.length() + 1);
size_t size = sizeof(wchar_t) * wcslen( value );
#if defined( _MSC_VER )
void* temp2 = _malloc_dbg( size, _NORMAL_BLOCK, THIS_FILE, __LINE__ );
#else
void* temp2 = malloc( size );
#endif
memcpy( temp2, value, size );
return temp2;
};
template<>
void* RawData<std::wstring>( const std::wstring& value)
{
// caller must free memory
size_t size = sizeof(wchar_t) * value.length();
#if defined( _MSC_VER )
void* temp2 = _malloc_dbg( size, _NORMAL_BLOCK, THIS_FILE, __LINE__ );
#else
void* temp2 = malloc( size );
#endif
memcpy( temp2, value.data(), size );
return temp2;
};
// specialised for Blobs. Allocate mem, copy data, and return it. Caller free()s.
template<>
void* RawData<struct MpBlob>(const struct MpBlob& value)
{
//	void* temp2 = _malloc_dbg( value.getSize(), _NORMAL_BLOCK, THIS_FILE, __LINE__ );
#if defined( _MSC_VER )
void* temp2 = _malloc_dbg( value.getSize(), _NORMAL_BLOCK, THIS_FILE, __LINE__ );
#else
void* temp2 = malloc( value.getSize() );
#endif
memcpy( temp2, value.getData(), value.getSize() );
return temp2;
}
*/