#pragma once
#include <string>
#include <vector>
//#include "se_datatypes.h"
#include "string_utilities.h"
#include "se_types.h"

// collection of handy general purpose routines

#define CLEAR_BITS(flag,bit_mask) ((flag) &= (0xffffffff ^(bit_mask)))
#define SET_BITS(flag,bit_mask) ((flag) |= (bit_mask))

#define SET_FLAG_STATE(flag,bit_mask,state) (state) ? (flag |= (bit_mask)) : (flag &= (0xffffffff ^(bit_mask)))

double StringToDouble(const std::wstring& string);
int StringToInt(const std::wstring& string, int p_base = 10);
int StringToInt(const char* string, int p_base = 10);
std::wstring DoubleToString(double val);
std::string DoubleToXmlString(double val, int p_decimal_places = -1);
std::string QuoteStringIfSpaces(const std::string& s);
std::string UnQuoteStringIfSpaces(const std::string& s);
std::wstring FloatToString(float val, int p_decimal_places = -1); // better
std::wstring IntToString(int val);
std::wstring MakeCamelCase(std::wstring word);
std::wstring ReplaceUnderscores(std::wstring word);
std::wstring RemoveSpaces(std::wstring word);
std::wstring long_to_id( int p_id );

std::wstring Trim(std::wstring s);
std::wstring TrimRight(std::wstring s);
std::wstring TrimLeft(std::wstring s);
std::wstring Uppercase(std::wstring s);
std::wstring Lowercase(std::wstring s);
std::wstring& replacein(std::wstring& s, const std::wstring& sub, const std::wstring& other);
void MakeLegalVariableName( std::wstring& s );
void MakeLegalXml( std::wstring& s );
std::string EscapeXmlSpecialCharacters(const std::string& s);

float StringToFloat(const std::wstring& string);

std::wstring Utf8ToWstring( char const* p_string );
std::wstring Utf8ToWstring( const std::string& p_string );
std::string WStringToUtf8( const std::wstring& p_cstring );

#if defined( _WIN32 )
void CStringToAnsi(const std::wstring& p_cstring, char* p_ansi, int p_max_char = 20 );
std::string WstringToString(const std::wstring p_cstring);
#endif

void WStringToWchars(const std::wstring& string, wchar_t* dest, int p_max_char);
std::wstring ToWstring(const std::string p_cstring);
std::wstring ToWstring(const char* p_string);
inline std::wstring ToWstring(const std::wstring s)
{
	return s;
}

std::wstring ToWstring(const char16_t* s);

bool SetBit(int& flags, int bitmask, bool new_state);
#define FlagSet( flags, bitmask) ( ((flags) & (bitmask)) != 0 )

//bool AreCompatible( EPlugDataType d1, EPlugDataType d2 );

// Safe preferred option.
inline uint32_t idToInt32(std::string id)
{
	id += "\0\0\0\0"; // prevent crash on shorter IDs
	return ((uint32_t) id[3]) + ((uint32_t)id[2] << 8) + ((uint32_t)id[1] << 16) + ((uint32_t)id[0] << 24);
}

int32_t id_to_long(const std::string id);
int id_to_long( const std::wstring& id );
int id_to_long( const char* id );
// faster macro, don't require function call, can be used in switch statements
#define code_to_long( C1,C2,C3,C4 ) (((C1) << 24) + ((C2) << 16) + ((C3) << 8) + (C4))
// faster constexpr
constexpr int32_t id_to_long2(const char* id)
{
	return (((int32_t)id[0]) << 24) + (((int32_t)id[1]) << 16)+(((int32_t)id[2]) << 8) + id[3];
}

// files
bool FileExists(const std::wstring& filename);
std::wstring GetExtension(std::wstring p_filename);
std::string GetExtension(std::string p_filename);
void decompose_filename( const std::wstring& p_filename, std::wstring& r_file, std::wstring& r_path, std::wstring& r_extension );

// combines path and file,
// handles tricky situations like both having slashes, or not.
template<typename T>
inline T combine_path_and_file_imp( const T& p_path, const T& p_file )
{
    // ensure path ends in slash
    auto first_bit = p_path;
    
    if( !first_bit.empty() && first_bit[first_bit.size()-1] != L'\\' && first_bit[first_bit.size()-1] != L'/' )
    {
        first_bit += PLATFORM_PATH_SLASH_L;
    }
    
    // ensure file does not begin with slash
    auto last_bit = p_file;
    
    if( !last_bit.empty() && last_bit[0] == L'\\' && last_bit[0] == L'/' )
    {
        last_bit = Right( p_file,p_file.size() - 1 );
    }
    
    return first_bit + last_bit;
}

inline std::wstring combine_path_and_file(const std::wstring& p_path, const std::wstring& p_file)
{
	return combine_path_and_file_imp<std::wstring>(p_path, p_file);
}

inline std::string combine_path_and_file(const std::string& p_path, const std::string& p_file)
{
	return combine_path_and_file_imp<std::string>(p_path, p_file);
}

int SampleToMs( timestamp_t s, int sample_rate);
timestamp_t msToSamples( int ms , int sample_rate);

int get_enum_range_lo(const std::wstring& p_enum_list);
int get_enum_range_hi(const std::wstring& p_enum_list);
std::wstring enum_get_substring ( const std::wstring& enum_list, int p_value );
bool enums_are_compatible( const std::wstring& enum1, const std::wstring& enum2 );
float enum_to_normalised( const std::wstring& p_enum_list, int p_value );
int normalised_to_enum( const std::wstring& p_enum_list, float p_normalised );

bool is_denormal( float f );

void reverse(void* p_ptr,int count );

inline void clamp_normalised(float& f)
{
	if(f < 0.f) f = 0.f;

	if( f > 1.f ) f = 1.f;
}

bool XmlStringToDatatype(std::string s, int& returnValue );
bool XmlStringToController(std::string s, int& returnValue );
bool XmlStringToParameterField( std::string s, int& returnValue );
std::string XmlStringFromDatatype( int v );
std::string XmlStringFromController( int v );
std::string XmlStringFromParameterField( int v );
void XmlSplitString(const char* s, std::vector<std::string>& returnValue);
// Avoid crashes caused by null pointers when a char string expected.
const char* FixNullCharPtr( const char* c );

bool CreateFolderRecursive( std::wstring folderPath );
