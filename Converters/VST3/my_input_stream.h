#pragma once
#include <string>

class my_input_stream
{
public:
	my_input_stream() {}
	virtual ~my_input_stream() {}
	virtual void Read( const void* lpBuf, unsigned int nMax ) = 0;

	// POD types.
	template<typename T>
	my_input_stream& operator>>(T& val)
	{
		Read(&val,sizeof(val));
		return *this;
	}

	my_input_stream& operator>>(std::wstring& val)
	{
		int size = 0;
		Read(&size,sizeof(size));
		wchar_t* str = new wchar_t[size];
		Read(str, size * sizeof(wchar_t));
		val.assign(str, size);
		delete [] str;
		return *this;
	}

	my_input_stream& operator>>(std::string& val)
	{
		int32_t size = 0;
		Read(&size,sizeof(size));
		val.resize(size);
		Read(val.data(), size);
		return *this;
	}

	my_input_stream& operator>>(struct MpBlob& val);
};

class my_output_stream
{
public:
    my_output_stream();
    virtual ~my_output_stream();
    virtual void Write( const void* lpBuf, unsigned int nMax ) = 0;

    // POD types.
    template<typename T>
    my_output_stream& operator<<(T val)
    {
        Write(&val,sizeof(val));
        return *this;
    }

    my_output_stream& operator<<(const std::string& val);
    my_output_stream& operator<<(const std::wstring& val);
    my_output_stream& operator<<(const struct MpBlob& val);
};

