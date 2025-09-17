#include "Hosting/message_queues.h"

#include <string>
#include <vector>
#include <algorithm>

namespace gmpi
{
namespace hosting
{

my_input_stream& my_input_stream::operator>>(gmpi::Blob& val)
{
    int size;
    Read(&size, sizeof(size));
    val.resize(size);
    Read(val.data(), size);
    return *this;
}

my_output_stream& my_output_stream::operator<<(const std::string& val)
{
    int32_t size = (int32_t)val.size();
    Write(&size, sizeof(size));
    Write((void*)val.data(), size * sizeof(val[0]));
    return *this;
}

my_output_stream& my_output_stream::operator<<(const std::wstring& val)
{
    int32_t size = (int32_t)val.size();
    Write(&size, sizeof(size));
    Write((void*)val.data(), size * sizeof(val[0]));
    return *this;
}

my_output_stream& my_output_stream::operator<<(const gmpi::Blob& val)
{
    int size = val.size();
    Write(&size, sizeof(size));
    Write(val.data(), size);
    return *this;
};

bool QueuedUsers::ServiceWaiters(my_output_stream& outStream, int freeSpace, int maximumBytes)
{
	bool sentData = false;

	QueClient* client = waitingClientsHead;

	if (client)
	{
		QueClient* tail = waitingClientsTail;

		while (client)
		{
			const int headerLength = sizeof(int) * 3;
			const int safetyZoneLength = headerLength * 8; // not all que users check size first. Allow some slack for the odd miscelaneous message.
			int requestedMessageLength = client->queryQueMessageLength(maximumBytes - headerLength); // NOT including header.
			int totalMessageLength = requestedMessageLength + headerLength;

			// in the rare case that a message is bigger than maximumBytes, use the alternate method.
			if (totalMessageLength + safetyZoneLength > maximumBytes)
			{
				std::vector<std::byte> oversize_data(requestedMessageLength + headerLength);
				memory_output_stream ourstream(oversize_data);

				client->getQueMessage(outStream, requestedMessageLength);
				break;
			}

			// not enough spare capacity atm? try again later.
			if (totalMessageLength + safetyZoneLength > freeSpace)
			{
				break;
			}

			//_RPT2(_CRT_WARN, "ServiceWaiter: %s (%x)\n", typeid(*waitingClientsHead).name(), waitingClientsHead);
			waitingClientsHead = client->next_;

			if (waitingClientsTail == client)
			{
				waitingClientsTail = 0;
			}

			client->inQue_ = false;
			client->dirty_ = false;
			client->getQueMessage(outStream, requestedMessageLength);

			sentData = true;

			freeSpace -= totalMessageLength;

			// only go as far as original tail (else objects might que themselves over and over)
			if (client == tail)
				client = 0;
			else
				client = waitingClientsHead;
		}
	}
	return sentData;
}
/////////////////////////////////////////////////////////////////////////////////
interThreadQue::interThreadQue(int p_que_size) :
	fifo_(p_que_size)
	, partialMessageReceived(false)
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

void interThreadQue::pushString(int p_length, const unsigned char* p_data)
{
	//	_RPT1(_CRT_WARN, "interThreadQue::pushString l=%d", p_length );
	const int freeSpace = fifo_.freeSpace(); // volatile remember. Have to take snapshot and use that.

	// overflow queue no longer works, must have broken at some point. No point trying to use it.
	assert(p_length <= freeSpace);

	//_RPT0(_CRT_WARN, " to FIFO\n" );
	fifo_.pushString(p_length, p_data);
}

void interThreadQue::popString(int p_length, const void* p_data)
{
	assert(false); // never called.
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

	if (!partialMessageReceived)
	{
		const int sizeofHeader = sizeof(int) * 3;

		for (;;)
		{
			if (fifo_.readyBytes() < sizeofHeader)
				return;

			my_msg_que_input_stream strm(&fifo_);
			strm >> recievingHandle;
			strm >> recievingMessageId;
			strm >> recievingMessageLength;

			assert(recievingMessageId != 0);
			assert(recievingMessageLength >= 0);

			if (fifo_.readyBytes() >= recievingMessageLength)
			{
#if defined( _DEBUG )
				int initialReadPos = fifo_.debugGetReadPtr();
#endif

				ProcessMessage(client, strm);

#if defined( _DEBUG )
				int ReadBytes = fifo_.debugGetReadPtr() - initialReadPos;
				assert((ReadBytes < 0 || ReadBytes == recievingMessageLength) && "Client Read wrong amnt");
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
	assert(fifo_.totalSpace() > recievingMessageLength);

	if (fifo_.readyBytes() >= recievingMessageLength)
	{
		my_msg_que_input_stream strm(&fifo_);
		ProcessMessage(client, strm);

		partialMessageReceived = false;
		// we've popped only 1 message this cycle. check for more data.
		goto MORE_DATA;
	}

	// enough room in FIFO, but not enough data yet. Siphon more...
}

void interThreadQue::ProcessMessage(interThreadQueUser* client, my_msg_que_input_stream& strm)
{
	if (!client->onQueMessageReady(recievingHandle, recievingMessageId, strm))
	{
		// If client couldn't handle message (e.g. module muted) discard message to free up que.
		char temp[8];
		int todo = recievingMessageLength;
		while (todo > 0)
		{
			fifo_.popString(std::min(todo, (int)sizeof(temp)), &temp);
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
/////////////////////////////////////////////////////////////////////////////////

} // namespace hosting
} // namespace gmpi
