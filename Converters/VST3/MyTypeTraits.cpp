#include "pch.h"
#include <string>
#include "Base64.h"
#include "MyTypeTraits.h"

using namespace std;

template<>
void MyTypeTraits<std::wstring>::parse(const wchar_t *stringValue, std::wstring &returnValue)
{
	returnValue.assign(stringValue);
}

template<>
void MyTypeTraits<MpBlob>::parse(const wchar_t *stringValue, MpBlob &returnValue)
{

	// returnValue = (stringValue);
}

template<>
void MyTypeTraits<float>::parse(const wchar_t *stringValue, float &returnValue)
{
	returnValue = StringToFloat(stringValue);
}

template<>
void MyTypeTraits<int>::parse(const wchar_t *stringValue, int &returnValue)
{
	returnValue = StringToInt(std::wstring(stringValue));
}

template<>
void MyTypeTraits<short>::parse(const wchar_t *stringValue, short &returnValue)
{
	returnValue = (short) StringToInt(std::wstring(stringValue));
}

template<>
void MyTypeTraits<bool>::parse(const wchar_t *stringValue, bool &returnValue)
{
	returnValue = 0 != StringToInt(std::wstring(stringValue));
}

template<>
void MyTypeTraits<double>::parse(const wchar_t *stringValue, double &returnValue)
{
	returnValue = StringToDouble(stringValue);
}

//
// =======================================================================================================================
//    UTF8 STRINGS.
// =======================================================================================================================
//
template<>
void MyTypeTraits<std::wstring>::parse(const char *stringValue, std::wstring &returnValue)
{
	returnValue = Utf8ToWstring(stringValue);
}

template<>
void MyTypeTraits<MpBlob>::parse(const char *stringValue, MpBlob &returnValue)
{
	string binaryData = Base64::decode(string(stringValue));
	returnValue.setValueRaw((int) binaryData.size(), binaryData.data());
}

template<>
void MyTypeTraits<float>::parse(const char *stringValue, float &returnValue)
{

	// returnValue = (float) stod( stringValue );
	// // VS2010
	returnValue = (float) atof(stringValue);	// VS2005
}

template<>
void MyTypeTraits<int>::parse(const char *stringValue, int &returnValue)
{

	// returnValue = stol( stringValue );
	returnValue = atoi(stringValue);
}

template<>
void MyTypeTraits<short>::parse(const char *stringValue, short &returnValue)
{

	// returnValue = (short) stol( stringValue );
	returnValue = (short) atoi(stringValue);
}

template<>
void MyTypeTraits<bool>::parse(const char *stringValue, bool &returnValue)
{

	// returnValue = ( 0 != stol( stringValue ) );
	returnValue = (0 != atoi(stringValue));
}

template<>
void MyTypeTraits<double>::parse(const char *stringValue, double &returnValue)
{

	// returnValue = stod( stringValue );
	returnValue = atof(stringValue);
}

template<>
std::string MyTypeTraits<MpBlob>::toXML( const MpBlob& value )
{
	std::string binaryData((const char*)value.getData(), value.getSize() );
	return Base64::encode(binaryData);
}

template<>
std::string MyTypeTraits<std::wstring>::toXML( const std::wstring& value )
{
	return WStringToUtf8(value);
}

template<>
std::string MyTypeTraits<float>::toXML(const  float& value )
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

template<>
std::string MyTypeTraits<int>::toXML( const int& value )
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

template<>
std::string MyTypeTraits<short>::toXML( const short& value )
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

template<>
std::string MyTypeTraits<bool>::toXML( const bool& value )
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

template<>
std::string MyTypeTraits<double>::toXML( const double& value )
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}
