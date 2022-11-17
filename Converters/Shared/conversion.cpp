//#include "pch.h"
#include <algorithm>
#include <string>
// Fix for <sstream> on Mac (sstream uses undefined int_64t)
#include "mp_api.h"
#include <sstream>
#include <iomanip>
#include <math.h>
#include <assert.h>
#include <cctype>
#include <sys/stat.h> // mkdir
#include "conversion.h"
//#include "ug_event.h"
#include "it_enum_list.h"
#include "midi_defs.h"
#include "IPluginGui.h"
#include "xp_simd.h"
#include "se_datatypes.h" // kill this

// XCODE can't handle this yet.
#if _MSC_VER >= 1600 // Not Avail in VS2005.
//#include <cvt/wstring>
#include <codecvt>
#endif

using namespace std;

//VST ID
// Preferred method. ASCII characters.
int32_t id_to_long(const std::string id)
{
	std::string tid = id + std::string("    ", 4); // pad with spaces to prevent crash on shorter IDs
	return (tid[3]) + (tid[2] << 8) + (tid[1] << 16) + (tid[0] << 24);
}

int id_to_long( const std::wstring& id ) // !! should not be wide strings, may contain UTF16!!
{
	std::wstring tid = id + (L"    "); // pad to prevent crash on shorter IDs
	return (tid[3]) + (tid[2] << 8 ) + (tid[1] << 16 ) + (tid[0] << 24 );
}

int id_to_long( const char* id ) // !! don't cope with les than 4 chars !!
{
	assert((strlen(id) == 3 && id[3] == 0) || strlen(id) == 4);
	return (((int)id[0]) << 24) + (((int)id[1]) << 16)+(((int)id[2]) << 8)+ id[3];
}

std::wstring long_to_id( int p_id )
{
	wchar_t i[5];
	i[3] = p_id & 0xff;
	i[2] = (p_id >> 8) & 0xff;
	i[1] = (p_id >> 16) & 0xff;
	i[0] = (p_id >> 24) & 0xff;
	i[4] = 0;
	return std::wstring(i);
}

double StringToDouble(const std::wstring& s)
{
	wchar_t* temp;	// convert string to SAMPLE
	return wcstod( s.c_str(), &temp );
}

float StringToFloat(const wstring& string)
{
	wchar_t* temp;	// convert string to SAMPLE
	return (float) wcstod( string.c_str(), &temp );
}

int StringToInt(const wstring& string, int p_base)
{
	assert( string.size() < 11 || string.find(L',') != wstring::npos  ); // else too many digits overflows sizeof int
	wchar_t* temp;
	return wcstol( string.c_str(), &temp, p_base );
}

int StringToInt(const char* string, int p_base)
{
 //   assert( string.size() < 11 || string.find(L',') != string::npos  ); // else too many digits overflows sizeof int
    char* temp;
    return strtol( string, &temp, p_base );
}

std::wstring DoubleToString(double val)
{
	//	s.Format((L"%f"),val);
	std::wostringstream oss;
	oss << val;
	std::wstring s = oss.str();
	/*
	// remove pesky trailing zeros (and period)
	int last = s.size() - 1;
	while( s[last] == (L'0') )
		last--;
	if( s[last] == (L'.') )
		last--;
	s = Left(s,last+1);
	*/
	return s;
}

std::wstring FloatToString(float val, int p_decimal_places) // better becuase it removes trailing zeros. -1 for auto decimal places
{
	bool auto_decimal = p_decimal_places < 0; // remember auto (as decimal places is modified)
	std::wostringstream oss;

	if( auto_decimal )
	{
		oss << setprecision( 3 ) << val; // 3 significant digits regardless of point.
		/*
				float absolute = fabsf(val);
				p_decimal_places = 2;
				if( absolute < 0.1f )
				{
					if( absolute == 0.0f )
						p_decimal_places = 1;
					else
						p_decimal_places = 4;
				}
				else
					if( absolute > 10.f )
						p_decimal_places = 1;
					else
						if( absolute > 100.f )
							p_decimal_places = 0;
			*/
	}
	else
	{
		oss << setiosflags(ios_base::fixed) << setprecision( p_decimal_places ) << val; // x significant digits AFTER point.
	}

	//std::wstring s;
	//s.Format( (L"%.*f"), p_decimal_places, (double)val );
	std::wstring s = oss.str();

	// Replace -0.0 with 0.0 ( same for -0.00 and -0.000 etc).
	// deliberate 'feature' of printf is to round small negative numbers to -0.0
	if( s[0] == L'-' && val > -1.0f )
	{
		int i = (int) s.size() - 1; // Not using unsigned size_t else fails <0 test below.

		while( i > 0 )
		{
			if( s[i] != L'0' && s[i] != L'.' )
			{
				break;
			}

			--i;
		}

		if( i == 0 ) // nothing but zeros (or dot).
		{
			s = Right(s, s.size() - 1 );
		}
	}

	// remove pesky trailing zeros (and period)
	/*	 causes display to jump about, very hard to read moving value
		if( auto_decimal && p_decimal_places > 0 )
		{
			int last = s.size() - 1;
			while( s[last] == (L'0') && last > 0 )
				last--;
			if( s[last] == (L'.') )
				last--;
			s = Left( s, last+1);
		}
	*/
	return s;
}

// floating-point value to UTF8 string, avoids scientific notation of regular float-to-string.
std::string DoubleToXmlString(double value, int p_decimal_places)
{
	std::ostringstream oss;

	if( p_decimal_places >= 0 )
	{
		oss << std::setprecision(p_decimal_places) << value;
	}
	else
	{
		// Use default format (removes trailing zeros)
		oss << value;
		p_decimal_places = 6; // Incase it outputs scientific notation.
	}

	// Unless it outputs scientific notation (on small numbers)
	// Then switch to fixed style.
	if( oss.str().find_first_of('e') != string::npos )
	{
		oss.str("");
		oss << setiosflags(std::ios_base::fixed) << std::setprecision(p_decimal_places) << value;
	}

	return oss.str();
}

// bigdog  -> bigdog
// big dog -> 'big dog'
// <empty> -> ''
std::string QuoteStringIfSpaces(const std::string& s)
{
	//TODO: What if string contains quotes?? !!
	// Any spaces?
	if (!s.empty() && std::find(s.begin(), s.end(), L' ') == s.end())
	{
		return s;
	}

	// needs quotes.
	std::ostringstream oss;
	oss << "'" << s << "'";
	return oss.str();
}

std::string UnQuoteStringIfSpaces(const std::string& s)
{
	// quoted empty string?
	if (s == "''")
		return {};

	// Any spaces? If no spaces, return.
	if (s.size() < 2 || std::find(s.begin(), s.end(), L' ') == s.end())
	{
		return s;
	}

	const char SINGLE_QUOTE = '\'';
	if (s[0] != SINGLE_QUOTE || s[s.size()-1] != SINGLE_QUOTE)
	{
		return s;
	}

	return s.substr(1, s.size() - 2);
}

std::wstring IntToString(int val)
{
	std::wostringstream oss;
	oss << val;
	return oss.str();
}

// Capitalise first letters.
std::wstring MakeCamelCase(std::wstring word )
{
	bool first = true;
	for( int i = 0 ; i < word.size() ; ++ i )
	{
		if( first )
		{
			word[i] = std::toupper(word[i]);
		}
		else
		{
			word[i] = std::tolower(word[i]);
		}
		if( word[i] == L' ' )
		{
			first = true;
		}
		else
		{
			first = false;
		}
	}

	return word;
}

std::wstring Uppercase(std::wstring s)
{
	transform(s.begin(), s.end(), s.begin(), towupper);
	return s;
}

std::wstring Lowercase(std::wstring s)
{
	transform(s.begin(), s.end(), s.begin(), towlower);
	return s;
}

std::wstring& replacein(std::wstring& s, const std::wstring& sub,
						const std::wstring& other)
{
	assert(!sub.empty());
	size_t b = 0;

	for (;;)
	{
		b = s.find(sub, b);

		if (b == s.npos) break;

		s.replace(b, sub.size(), other);
		b += other.size();
	}

	return s;
}

// change all underscores '_' to spaces
std::wstring ReplaceUnderscores(std::wstring word )
{
	replacein( word, (L"_"), (L" ") );
	return word;
}

// remove all spaces
std::wstring RemoveSpaces(std::wstring word )
{
	std::wstring dest;

	for(int i = 0; i < word.size(); i++)
	{
		if( word[i] != ' ' )
		{
			dest += word[i];
		}
	}

	return dest;
}


std::wstring Trim(std::wstring s)
{
	return TrimLeft(TrimRight(s));
}

std::wstring TrimRight(std::wstring s)
{
	//if( !s.empty() && s[s.size()-1] == L' ' )
	//{
	//	int i = s.find_last_not_of(' ');

	//	if( i != string::npos )
	//	{
	//		return s.substr( 0, i + 1 );
	//	}
	//}
	if( s.empty() )
	{
		return s;
	}

	int i = (int) s.size() - 1; // Not using unsigned size_t else fails <0 test below.
	while( i >= 0 && isspace( s[i] ) )
	{
		--i;
	}

	return s.substr( 0, i + 1 );
}

std::wstring TrimLeft(std::wstring s)
{
	//int i = s.find_first_not_of(' ');

	//if( i > 0 ) // ignore no space(0) AND empty string (-1)
	//{
	//	return s.substr( i );
	//}

	for( std::wstring::iterator it = s.begin() ; it != s.end() ; ++it )
	{
		if( !isspace( *it ) )
		{
			return std::wstring( it, s.end() );
		}
	}

	return s;
}

// sets or clears a flag, return true if flag changed
bool SetBit(int& flags, int bitmask, bool new_state)
{
	int old_flags = flags;

	if( new_state )
	{
		SET_BITS( flags, bitmask );
	}
	else
	{
		CLEAR_BITS( flags, bitmask );
	}

	return (flags == old_flags);
}

bool FileExists(const std::wstring& filename)
{
	if( filename.empty() ) // prevent fopen crash on empty name
		return false;

	FILE* stream;

	/* Open for read (will fail if file "data" does not exist) */
#if defined( _WIN32 )
	_wfopen_s(&stream, filename.c_str(), (L"r"));
	if (stream != NULL)
#else
	if( (stream  = fopen( WStringToUtf8(filename).c_str(), "r")) != NULL )
#endif
	{
		fclose(stream);
		return true;
	}

	return false;
}

void decompose_filename( const std::wstring& p_filename, std::wstring& r_file, std::wstring& r_path, std::wstring& r_extension )
{
	r_extension = GetExtension(p_filename);
	std::wstring temp = StripExtension( p_filename );
	r_file = StripPath( temp );
	r_path = Left( temp,temp.size() - r_file.size() );
}

// returns file extension in lowercase
std::wstring GetExtension(std::wstring p_filename)
{
	// p_filename.MakeLower();
	transform(p_filename.begin(), p_filename.end(), p_filename.begin(), towlower );
	int p = (int) p_filename.size() - 1; // Not using unsigned size_t else fails <0 test below. Could be re-written to fix this.

	while( p > 0 )
	{
		wchar_t c = p_filename[p];

		// path seperator?
		if( c == (L'\\') || c == (L'/') ) // ignore folder names with dots
			break;

		if( c == (L'.') )
			return Right( p_filename,p_filename.size() - p - 1 );

		p--;
	}

	return (L"");
	/*

		int p = p_filename.find_last_of(' .');
		if( p > -1 )
		{
			p_filename = Right( p_filename,  p_filename.size() - p - 1);
		}
		else
		{
			p_filename = "";
		}
		return p_filename;
	*/
}

// returns file extension in lowercase
std::string GetExtension(std::string p_filename)
{
	// p_filename.MakeLower();
	transform(p_filename.begin(), p_filename.end(), p_filename.begin(), towlower);
	int p = (int)p_filename.size() - 1; // Not using unsigned size_t else fails <0 test below. Could be re-written to fix this.

	while (p > 0)
	{
		wchar_t c = p_filename[p];

		// path seperator?
		if (c == '\\' || c == '/') // ignore folder names with dots
			break;

		if (c == '.')
			return Right(p_filename, p_filename.size() - p - 1);

		p--;
	}

	return "";
}

// Make string suitable for use as a C++ variable name.
void MakeLegalVariableName( std::wstring& s )
{
	s = Trim( s );

	wchar_t remove[] = {L'!', L'#', L'$', L'%', L'&', L'(', L')', L'*', L'+', L',', L'.', L'/', L':', L';', L'<', L'=', L'>', L'?', L'@', L'[', L'\\', L']', L'^', L'{', L'|', L'}', L'~'};

	for( int i = 0 ; i < sizeof(remove) / sizeof(wchar_t) ; ++i )
	{
		replacein( s, wstring(remove + i, 1), L"" );
	}

	// replace with underscore.
	wchar_t replace[] = {L' ', L'-' };

	for( int i = 0 ; i < sizeof(replace) / sizeof(wchar_t) ; ++i )
	{
		replacein( s, wstring(replace + i,1), L"_" );
	}
}

// Remove double-quotes and slashes (escapes) within quoted XML text.
void MakeLegalXml( std::wstring& s )
{
	wchar_t remove[] = {L'"', L'\\'};

	for( int i = 0 ; i < sizeof(remove) / sizeof(wchar_t) ; ++i )
	{
		replacein( s, wstring(remove + i, 1), L"" );
	}
}

std::string EscapeXmlSpecialCharacters(const std::string& s)
{
	static std::pair<const char*, char> entities[] = {
		{ "&quot;",	'"'  },
		{ "&amp;", 	'&'  },
		{ "&apos;",	'\'' },
//		{ "&rsquo;",'’' }, // not an ASCII character, it's multi byte
		{ "&lt;", 	'<'	 },
		{ "&gt;",	'>'	 }
	};

	std::string escaped;
	for (auto c : s)
	{
		for (const auto& sub : entities)
		{
			if (c == sub.second)
			{
				escaped += sub.first;
				c = 0;
				break;
			}
		}

		if (c)
		{
			escaped += c;
		}
	}

	return escaped;
}

int SampleToMs( timestamp_t s, int sample_rate)   // being carefull to avoid numeric overflow
{
	timestamp_t total_seconds = s / (timestamp_t) sample_rate;
	timestamp_t total_ms  = s % (timestamp_t) sample_rate;
	total_ms = 1000 * total_seconds + ( 1000 * total_ms + sample_rate / 2) / sample_rate;
	return (int) total_ms;
}

timestamp_t msToSamples( int ms , int sample_rate)
{
	timestamp_t total_seconds = ms / 1000;
	timestamp_t total_sp  = ms % 1000;
	total_sp = sample_rate * total_seconds + ( sample_rate * total_sp ) / 1000;
	return total_sp;
}

/*
void get_enum_range( const std::wstring &enum_list, int &lo, int &hi )
{
	hi = -99999999;
	lo = 99999999;

	std::wstring list = enum_list;

	// two formats are allowed, a list of values or a range
	if( Left( list, 5) == std::wstring(L"range") )
	{
		list = Right( list,  list.size() - 5);

		int p = list.Find(',');
		assert( p != -1 );	// no range specified

		lo = StringToInt( list );
		list = Right( list,  list.size() - p -1);
		hi = StringToInt(list );
	}
	else
	{
		// ! returns index range (not value range) not much use!!!!!!!
		it_enum_list itr(enum_list);
		for( itr.First(); !itr.IsDone(); itr.Next() )
		{
			enum_entry *e = itr.CurrentItem();
			hi = max( hi, e->index );
			lo = min( lo, e->index );
		}
/ *
		int id = 0;
		do
		{
			int p = list.Find(',');
			if( p == -1 )
			{
				p = list.size();
			}
			std::wstring list_item = Left( list,  p );
			p = min(p,list.size()-1);
			list = Right( list,  list.size() - p - 1);
			int p_eq = list_item.Find('=');
			if( p_eq > 0 )
			{
				std::wstring id_str = Right( list_item,  list_item.size() - p_eq -1 );
				list_item = Left( list_item,  p_eq );
				id = StringToInt(id_str);
			}
			list_item.TrimRight();
			list_item.TrimLeft();
			hi = max( hi, id );
			lo = min( lo, id );
			id++;

		}while( list.size() > 0 );
* /
	}
}*/

// returns string associated with value (NOT index)
std::wstring enum_get_substring (const std::wstring& enum_list, int p_value )
{
	// two formats are allowed, a list of values or a range
	std::wstring list = enum_list;

	if( Left(list,5) == std::wstring(L"range") )
	{
		/*
				list = Right( list,  list.size() - 5);

				int p = list.Find(',');
				assert( p != -1 );	// no range specified

				int lo = StringToInt( list );
				list = Right( list,  list.size() - p -1);
				int hi = StringToInt(list);
		*/
		return IntToString( p_value );
	}
	else
	{
		it_enum_list itr(enum_list);

		if( itr.FindValue(p_value) )
			return itr.CurrentItem()->text;

		/*
				for( itr.First(); !itr.IsDone(); itr.Next() )
				{
					enum_entry *e = itr.CurrentItem();
					if( e->value == p_idx )
					{
						return e->text;
					}
				}
		*/
		/*
				int id = 0;
				do
				{
					int p = list.Find(',');
					if( p == -1 )
					{
						p = list.size();
					}
					std::wstring list_item = Left( list,  p );
					p = min(p,list.size()-1);
					list = Right( list,  list.size() - p - 1);
					int p_eq = list_item.Find('=');
					if( p_eq > 0 )
					{
						std::wstring id_str = Right( list_item,  list_item.size() - p_eq -1 );
						list_item = Left( list_item,  p_eq );
						id = StringToInt(id_str);
					}
					if( id == p_idx )
					{
						list_item.TrimRight();
						list_item.TrimLeft();
						return list_item;
					}

					id++;

				}while( list.size() > 0 );
		*/
	}

	return L"";
}

// compares enums, ignoring spaces, extra "=0" bits etc
bool enums_are_compatible( const std::wstring& enum1, const std::wstring& enum2 )
{
	it_enum_list itr1( enum1 );
	it_enum_list itr2( enum2 );
	itr2.First();

	for( itr1.First() ; !itr1.IsDone() && !itr2.IsDone(); itr1.Next() )
	{
		if( itr1.CurrentItem()->value != itr2.CurrentItem()->value )
			return false;

		itr2.Next();
	}

	return itr1.IsDone() && itr2.IsDone();
}

int get_enum_range_hi(const std::wstring& p_enum_list)
{
	it_enum_list itr( p_enum_list );

	if( itr.IsRange() )
		return itr.RangeHi();

	int v = 0;
	itr.First();

	while(! itr.IsDone() )
	{
		v = itr.CurrentItem()->value;
		itr.Next();
	}

	return v;
}

int get_enum_range_lo(const std::wstring& p_enum_list)
{
	it_enum_list itr( p_enum_list );
	itr.First();

	if( itr.IsDone() )
		return 0;
	else
		return itr.CurrentItem()->value;
}

float enum_to_normalised( const std::wstring& p_enum_list, int p_value )
{
	it_enum_list itr( p_enum_list );
	float number_of_values = (float) itr.size();
	itr.FindValue( p_value );

	if( number_of_values < 2.f || itr.IsDone() )
	{
		return 0.f;
	}

	assert( !itr.IsDone() );
	return ( (float) itr.CurrentItem()->index ) / ( number_of_values - 1.f);
}

int normalised_to_enum( const std::wstring& p_enum_list, float p_normalised )
{
	it_enum_list itr( p_enum_list );
	int number_of_values = itr.size();

	if( number_of_values == 0 )
	{
		return 0;
	}

	const int maxindex = number_of_values - 1;
	int enumIndex = FastRealToIntFloor(0.5 + p_normalised * static_cast<float>(maxindex));

	if(enumIndex >= maxindex)
		enumIndex = maxindex;

	itr.FindIndex(enumIndex);
	assert( !itr.IsDone() );
	return itr.CurrentItem()->value;
}

void WStringToWchars( const std::wstring& string, wchar_t* dest, int p_max_char )
{
	// If input is null then just return the same.
	if (string.empty())
	{
		*dest = {};
		return;
	}

	std::string::size_type string_length = string.size() + 1; // + 1 for NULL terminator
	string_length = min( (int) string_length, p_max_char );
	#if defined( _MSC_VER )
		wcsncpy_s( dest, string_length, string.data(), string_length );
	#else
		wcsncpy( dest, string.data(), string_length );
	#endif
}

#if defined( _WIN32 )

void CStringToAnsi(const std::wstring& p_cstring, char* p_ansi, int p_max_char )
{
	// If input is null then just return the same.
	if (p_cstring.empty())
	{
		*p_ansi = NULL;
		return;
	}

	int bytes_required = 1 + WideCharToMultiByte( CP_ACP, 0, p_cstring.c_str(), -1, 0, 0, NULL, NULL);

	if( bytes_required > p_max_char )
	{
		bytes_required = p_max_char - 1;
		p_ansi[bytes_required] = 0; // add terminator (because only part of string will be copied)
	}

	WideCharToMultiByte(CP_ACP, 0, p_cstring.c_str(), -1, p_ansi, bytes_required, NULL, NULL);
}

std::string WstringToString(const std::wstring p_cstring)
{
	int bytes_required = 1 + WideCharToMultiByte( CP_ACP, 0, p_cstring.c_str(), -1, 0, 0, NULL, NULL);
	char* temp = new char[bytes_required];
	WideCharToMultiByte( CP_ACP, 0, p_cstring.c_str(), -1, temp, bytes_required, NULL, NULL);
	string res(temp);
	delete [] temp;
	return res;
}
#endif

std::wstring ToWstring(const string p_string)
{
	return ToWstring( p_string.c_str() );
}

// same but copes with null pointer.
std::wstring ToWstring(const char* p_string)
{
#if defined(_WIN32)
	const int codepage = CP_ACP;
	int length = 1 + MultiByteToWideChar(codepage, 0, p_string, -1, (LPWSTR)0, 0 );
	wchar_t* wide = new wchar_t[length];
	wide[0] = 0; // Handle null input pointers.
	MultiByteToWideChar( codepage, 0, p_string, -1, (LPWSTR)wide, length );
	std::wstring temp(wide);
	delete [] wide;
#else
	// Lame conversion ASCII only.
	std::wstring temp;
	if( p_string != 0 )
	{
		temp.assign( strlen(p_string), 0 );

		for(int i = 0 ; i < temp.size(); ++i )
			temp[i] = (wchar_t)p_string[i];
	}
#endif
	return temp;
}

// for VST3 SDK 'TChar'
std::wstring ToWstring(const char16_t* p_string)
{
	std::wstring temp;
	if( p_string )
	{
		int length = 0;
		while (p_string[length])
			++length;

		temp.assign( length, 0 );

		for(int i = 0 ; i < temp.size(); ++i )
			temp[i] = (wchar_t)p_string[i];
	}

	return temp;
}


std::wstring Utf8ToWstring( const std::string& p_string )
{
	return JmUnicodeConversions::Utf8ToWstring(p_string);
}

std::wstring Utf8ToWstring( char const* p_string )
{
	if (p_string)
	{
		return JmUnicodeConversions::Utf8ToWstring(p_string);
	}
	return {};
}

#if defined( SE_SUPPORT_MFC )
std::wstring CStringToWstring(const CString& s)
{
	std::wstring temp(s.GetString(), (size_t) s.GetLength());
	return temp;
}

CString WstringToCString(std::wstring s)
{
	return CString( s.c_str() );
}
#endif

void reverse(void* p_ptr,int count )
{
	int32_t* p_id = (int32_t*) p_ptr;

	for(int i = 0 ; i < count ; i++ )
	{
		char* c = (char*) &p_id[i];
		int32_t result;
		char* c2 = (char*) &result;
		c2[0] = c[3];
		c2[1] = c[2];
		c2[2] = c[1];
		c2[3] = c[0];
		p_id[i] = result;
	}
}

bool is_denormal( float f )
{
	unsigned int l = *((unsigned int*)&f);
	return( f != 0.f && (l & 0x7FF00000) == 0 && (l & 0x000FFFFF) != 0 ); // anything less than approx 1E-38 excluding +ve and -ve zero (two distinct values)
}

struct TextToIntStruct // holds XML -> enum info
{
	const wchar_t* display;
	int id;
};

struct TextToIntStruct2 // holds XML -> enum info (when string known to be ASCII compatible)
{
	const char* display;
	int id;
};

bool LookupEnum( const std::string& txt, const TextToIntStruct2* table, int size, int defaultValue , int& returnValue )
{
	returnValue = defaultValue;

	if( txt.compare( "" ) != 0 )
	{
		int j;

		for( j = 0 ; j < size ; j++ )
		{
			if( txt.compare( table[j].display ) == 0  )
			{
				returnValue = table[j].id;
				break;
			}
		}

		if( j == size )
		{
			return false;
		}
	}

	return true;
}

const char* LookupEnum( const TextToIntStruct2* table, int size, int v )
{
	for( int j = 0 ; j < size ; j++ )
	{
		if( v == table[j].id )
		{
			return table[j].display;
			break;
		}
	}
	assert( false );
	return "";
}

static const TextToIntStruct2 datatypeInfo[] =
{
	("float")			,DT_FLOAT, // This must be first, as looking up "float" should not return DT_FSAMPLE.
	("float")			,DT_FSAMPLE,
	("int")				,DT_INT,
	("string")			,DT_TEXT,
	"string_utf8"		,DT_STRING_UTF8,
	("blob")			,DT_BLOB,
	("midi")			,DT_MIDI,
	("bool")			,DT_BOOL,
	("enum")			,DT_ENUM,
	("double")			,DT_DOUBLE,
	("int64")			,DT_INT64,
	("class")			,DT_CLASS,	
};

bool XmlStringToDatatype( string s, int& returnValue )
{
	if (LookupEnum(s, datatypeInfo, std::size(datatypeInfo), DT_FLOAT, returnValue))
		return true;

	if (s.find("class:") == 0)
	{
		returnValue = DT_CLASS;
		return true;
	}

	return false;
}

std::string XmlStringFromDatatype( int v )
{
	return LookupEnum( datatypeInfo, std::size(datatypeInfo), v );
}

static const TextToIntStruct2 controllerInfo[] =
{
	//			(L"<none>")					,ControllerType::None,
	("CC")					,ControllerType::CC,
	("RPN")					,ControllerType::RPN,
	("NRPN")				,ControllerType::NRPN,
	("Voice/Trigger")		,ControllerType::Trigger,
	("Voice/Gate")			,ControllerType::Gate,
	("Voice/Pitch")			,ControllerType::Pitch,
	("Voice/VelocityKeyOn")	,ControllerType::VelocityOn,
	("Voice/VelocityKeyOff"),ControllerType::VelocityOff,
	("Voice/Aftertouch")	,ControllerType::PolyAftertouch,
	("Voice/Active")		,ControllerType::Active,  // really a Controller???
	("Bender")				,ControllerType::Bender,
	"ChannelPressure"		,ControllerType::ChannelPressure,
	"Voice/GlideStartPitch"	, ControllerType::GlideStartPitch,
};

const int CONTROLLER_INFO_COUNT = (sizeof(controllerInfo) / sizeof(TextToIntStruct));
bool XmlStringToController( string s, int& returnValue )
{
	return LookupEnum( s, controllerInfo, CONTROLLER_INFO_COUNT, ControllerType::None, returnValue );
}
std::string XmlStringFromController( int v )
{
	return LookupEnum( controllerInfo, CONTROLLER_INFO_COUNT, v );
}


static const TextToIntStruct2 parameterFieldInfo[] =
{
	"Value",				FT_VALUE
	// writeable only in SynthEdit environment
	, "ShortName",			FT_SHORT_NAME
	, "MenuItems",			FT_MENU_ITEMS
	, "MenuSelection",		FT_MENU_SELECTION
	, "RangeMinimum",		FT_RANGE_LO
	, "RangeMaximum",		FT_RANGE_HI
	, "EnumList",			FT_ENUM_LIST
	, "FileExtension",		FT_FILE_EXTENSION
	, "IgnoreProgramChange",FT_IGNORE_PROGRAM_CHANGE
	, "Private",			FT_PRIVATE
	, "Automation",			FT_AUTOMATION
	, "Automation Sysex",	FT_AUTOMATION_SYSEX
	, "Default",			FT_DEFAULT
	, "Grab",				FT_GRAB
	, "Normalized",			FT_NORMALIZED
};
const int PFI_COUNT = (sizeof(parameterFieldInfo) / sizeof(TextToIntStruct));
bool XmlStringToParameterField( string s, int& returnValue )
{
	return LookupEnum( s, parameterFieldInfo, PFI_COUNT, FT_VALUE, returnValue );
}
std::string XmlStringFromParameterField( int v )
{
	return LookupEnum( parameterFieldInfo, PFI_COUNT, v );
}

// FAILS BADLY on single apostrophes, avoid.
void XmlSplitString(const char* pText, std::vector<std::string>& returnValue)
{
	if (pText)
	{
		const char SINGLE_QUOTE = '\'';
		const char DOUBLE_QUOTE = '"';
		const char* begin = pText;

		char terminator;

		while (true)
		{
			// eat spaces.
			while (isspace(*begin))
			{
				++begin;
			}

			if (*begin == 0) // end of string?
				break;

			const char* end = begin + 1;

			if (*begin == SINGLE_QUOTE || *begin == DOUBLE_QUOTE)
			{
				terminator = *begin;
				++begin;
				end = begin;
				while (*end != terminator && *end != 0)
				{
					++end;
				}
			}
			else
			{
				terminator = ' '; // String might be terminated by space or by quote starting next item.
//	isspace includes CR/LF		while (!isspace(*end) && *end != SINGLE_QUOTE && *end != DOUBLE_QUOTE && *end != 0)
				while (*end != terminator && *end != SINGLE_QUOTE && *end != DOUBLE_QUOTE && *end != 0)
				{
					++end;
				}
			}

			// We have now isolated a 'word'.
			returnValue.push_back( std::string(begin, end) );

			// Skip closing quote if nesc.
			if (*end == terminator && terminator != ' ')
			{
				++end;
			}

			begin = end;
		}
	}
}


#if defined( SE_SUPPORT_MFC )

// serialise the easy way by converting to std::wstring
CArchive& AFXAPI operator>>(CArchive& ar, std::wstring& pOb)
{
	CString temp;
	ar >> temp;
	pOb = CStringToWstring(temp);
	return ar;
}

CArchive& AFXAPI operator<<(CArchive& ar, std::wstring& pOb)
{
	ar << CString( pOb.c_str() );
	return ar;
}

CArchive& AFXAPI operator>>(CArchive& ar, std::string& pOb)
{
	int size;
	ar >> size;
	pOb.resize(size);
	ar.Read( (void*) pOb.data(), size );
	return ar;
}

CArchive& AFXAPI operator<<(CArchive& ar, std::string& pOb)
{
	ar << (int) pOb.size();
	ar.Write( pOb.data(), pOb.size() );
	return ar;
}

#endif

std::string WStringToUtf8(const std::wstring& p_cstring )
{
	return JmUnicodeConversions::WStringToUtf8(p_cstring);
}

// Works only if user has permissions.
bool CreateFolderRecursive(std::wstring folderPath)
{
	vector<wstring> paths;

	while (folderPath.size() > 2)  // C:
	{
		paths.push_back(folderPath);

		auto p = folderPath.find_last_of(L"\\/");

		if (p != string::npos)
		{
			folderPath = Left(folderPath, p);
		}
	}

	for (auto it = paths.rbegin(); it != paths.rend(); ++it)
	{
#ifdef _WIN32
		// Create folder if not already.
		const int r =_wmkdir((*it).c_str());
		if (r)
		{
			if (GetLastError() != ERROR_ALREADY_EXISTS)
				return false;
		}
#else
        mkdir(WStringToUtf8(*it).c_str(), 0775);
#endif
	}

	return true;
}

const char* FixNullCharPtr( const char* c )
{
	if( c )
		return c;
	return "";
}
