//#include "pch.h"
#include "./interThreadQue.h"
#include "./my_msg_que_output_stream.h"
#include "my_msg_que_input_stream.h"
#include "QueClient.h"
#include "assert.h"

#undef max
#undef min


interThreadQue::interThreadQue(int p_que_size ) :
	fifo_(p_que_size)
	,partialMessageReceived(false)
{
#if defined( _DEBUG )
	isFromGui = false; // to make breakpoints on specific que easier;
#endif
}

void interThreadQue::Send()
{
	fifo_.Send();
}

void interThreadQue::Reset()
{
	fifo_.Reset();
	partialMessageReceived = false;
	recievingHandle = recievingMessageId = recievingMessageLength = 0;
}

void interThreadQue::pushString( int p_length, const unsigned char* p_data )
{
	//	_RPT1(_CRT_WARN, "interThreadQue::pushString l=%d", p_length );
	const int freeSpace = fifo_.freeSpace(); // volatile remember. Have to take snapshot and use that.

	// overflow queue no longer works, must have broken at some point. No point trying to use it.
	assert(p_length <= freeSpace);

	//_RPT0(_CRT_WARN, " to FIFO\n" );
	fifo_.pushString( p_length, p_data );
}

void interThreadQue::popString( int p_length, const void* p_data )
{
	assert( false ); // never called.
	fifo_.popString(p_length, p_data);
}

bool interThreadQue::isEmpty()
{
	return fifo_.isEmpty();
}

int interThreadQue::readyBytes()
{
	return fifo_.readyBytes();
}

void interThreadQue::pollMessage(interThreadQueUser* client)
{
MORE_DATA:

	if( !partialMessageReceived )
	{
		const int sizeofHeader = sizeof(int) * 3;

		for(;;)
		{
			if( fifo_.readyBytes() < sizeofHeader )
				return;

			my_msg_que_input_stream strm( &fifo_ );
			strm >> recievingHandle;
			strm >> recievingMessageId;
			strm >> recievingMessageLength;

			assert(recievingMessageId  != 0);
			assert(recievingMessageLength  >= 0);

			if( fifo_.readyBytes() >= recievingMessageLength )
			{
				#if defined( _DEBUG )
					int initialReadPos = fifo_.debugGetReadPtr();
				#endif

				ProcessMessage( client, strm );

				#if defined( _DEBUG )
					int ReadBytes = fifo_.debugGetReadPtr() - initialReadPos;
					assert( ( ReadBytes < 0 || ReadBytes == recievingMessageLength ) && "Client Read wrong amnt" );
				#endif
			}
			else
			{
				// _RPT2(_CRT_WARN, "pollMessage - partial message %d/%d bytes .\n", fifo_.readyBytes(), recievingMessageLength );
				partialMessageReceived = true;
				return;
			}
		}
	}

	// If FIFO big enough for messsage, just wait till enough data available.
	assert(fifo_.totalSize() > recievingMessageLength);

	if( fifo_.readyBytes() >= recievingMessageLength )
	{
		my_msg_que_input_stream strm( &fifo_ );
		ProcessMessage( client, strm );

		partialMessageReceived = false;
		// we've popped only 1 message this cycle. check for more data.
		goto MORE_DATA;
	}

	// enough room in FIFO, but not enough data yet. Siphon more...
}

void interThreadQue::ProcessMessage( interThreadQueUser* client, my_msg_que_input_stream& strm )
{
	if( !client->onQueMessageReady( recievingHandle, recievingMessageId, strm ) )
	{
		// If client couldn't handle message (e.g. module muted) discard message to free up que.
		char temp[8];
		int todo = recievingMessageLength;
		while( todo > 0 )
		{
			fifo_.popString( std::min( todo, (int)sizeof(temp) ), &temp );
			todo -= sizeof(temp);
		}
	}
}

#if defined( _DEBUG )
bool interThreadQue::isUncomitted()
{
	return fifo_.isUncomitted();
}
#endif

