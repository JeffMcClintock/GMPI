#pragma once

#include "my_input_stream.h"
#include "./lock_free_fifo.h"

class my_msg_que_input_stream :
	public my_input_stream
{
public:

    my_msg_que_input_stream(InterThreadQueBase* p_que) :
    m_que(p_que)
    {
    };

	virtual void Read( const void* lpBuf, unsigned int nMax )
    {
        m_que->popString( nMax, lpBuf );
    }

private:
	InterThreadQueBase* m_que;
};

