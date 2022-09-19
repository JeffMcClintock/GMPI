//#include "pch.h"

#include "my_input_stream.h"
#include "mp_sdk_common.h"
//#include "mfc_emulation.h"


my_input_stream& my_input_stream::operator>>(struct MpBlob& val)
{
    int size;
    Read(&size,sizeof(size));
    char* dat = new char[size];
    Read(dat,size);
    val.setValueRaw(size,dat);
    delete [] dat;
    return *this;
}

my_output_stream::my_output_stream()
{
}

my_output_stream::~my_output_stream()
{
}

my_output_stream& my_output_stream::operator<<( const std::string& val )
{
    int32_t size = (int32_t) val.size();
    Write(&size,sizeof(size));
    Write( (void*) val.data(), size * sizeof(val[0]));
    return *this;
}

my_output_stream& my_output_stream::operator<<( const std::wstring& val )
{
    int32_t size = (int32_t) val.size();
    Write(&size,sizeof(size));
    Write( (void*) val.data(), size * sizeof(val[0]));
    return *this;
}

my_output_stream& my_output_stream::operator<<(const MpBlob& val)
{
    int size = val.getSize();
    Write(&size,sizeof(size));
    Write(val.getData(),size);
    return *this;
};

