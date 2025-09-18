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

void QueuedUsers::startMultiPartSend(QueClient* client, my_output_stream& outStream, int requestedMessageLength)
{
	int totalMessageLength = requestedMessageLength + headerLength;

	// write message to buffer
	{
		oversize_data.resize(totalMessageLength);
		memory_output_stream memstream(oversize_data);

		client->getQueMessage(memstream, requestedMessageLength);
	}

	// send multipart message header.
	{
		outStream << -99; // handle
		outStream << id_to_long("MPAR"); // multipart message start
		outStream << (int32_t) sizeof(int32_t);

		outStream << (int32_t) totalMessageLength; // multipart message length
	}

	multipartChunkIndex = 1;

//	_RPTN(0, "FIFO sending oversize message in chunks (%d bytes)\n", totalMessageLength);
}

bool QueuedUsers::MultiPart_send(my_output_stream& outStream, int freeSpace)
{
	// if queue is full or nearly, just rety later.
	if (freeSpace <= safetyZoneLength * 2)
		return false;

	const auto chunksize = (std::min)(oversize_data.size() - multipartReadIndex, (size_t)(freeSpace - safetyZoneLength));

//	_RPTN(0, "FIFO sending chunk %d (%d bytes)\n", multipartChunkIndex, chunksize);

	// send header for chunk.
	outStream << -99; // handle
	outStream << id_to_long("MPCK"); // multipart chunk
	outStream << (int32_t)chunksize;

	// send chunk data
	outStream.Write(oversize_data.data() + multipartReadIndex, (unsigned int)chunksize);
	multipartReadIndex += chunksize;

	if (multipartReadIndex >= oversize_data.size())
	{
		// end of multipart message.
		multipartChunkIndex = 0;
		multipartReadIndex = 0;
		oversize_data.clear();

//		_RPT0(0, "FIFO oversize DONE\n");
	}
	else
	{
		++multipartChunkIndex;
	}

	return true;
}


#if 0
bool QueuedUsers::ServiceWaiters(my_output_stream& outStream, int freeSpace, int maximumBytes)
{
	assert(maximumBytes > headerLength + safetyZoneLength);

	bool sentData = false;

	// if an oversize message is being sent, concentrate on that..
	if (multiPartMode())
		return MultiPart_send(outStream, freeSpace);

	QueClient* client = waitingClientsHead;

	if (client)
	{
		QueClient* tail = waitingClientsTail;

		while (client)
		{
			int requestedMessageLength = client->queryQueMessageLength(maximumBytes - headerLength); // NOT including header.
			int totalMessageLength = requestedMessageLength + headerLength;

			// in the rare case that a message is bigger than maximumBytes, use the alternate method.
			if (totalMessageLength + safetyZoneLength > maximumBytes)
			{
				if (safetyZoneLength >= freeSpace) // check there is a little room for the "MPAR" header.
					break;

				startMultiPartSend(client, outStream, requestedMessageLength);
			}
			else if (totalMessageLength + safetyZoneLength > freeSpace)
			{
				// not enough spare capacity atm? try again later.
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
#endif

// pass the number of sampleframes in this processblock, or just a large number (100000) if not audio related.
bool QueuedUsers::ServiceWaitersIncremental(IWriteableQue* que, int sampleFrames)
{
	my_msg_que_output_stream outStream(que);
	auto freeSpace = que->freeSpace();
	auto maximumBytes = que->totalSpace();

	// if an oversize message is being sent, concentrate on that..
	if (multiPartMode())
		return MultiPart_send(outStream, freeSpace);

	bool sentData = false;
	cumulativesampleFrames += sampleFrames;

	// Process a number proportional to time elapsed within graphics frame.
	sfAccumulator += sampleFrames;
	assert(sampleFramesPerClient > 0);

	int count = sfAccumulator / sampleFramesPerClient;
	sfAccumulator -= count * sampleFramesPerClient;

	constexpr int headerLength = sizeof(int) * 3;
	constexpr int safetyZoneLength = headerLength * 8; // not all que users check size first. Allow some slack for the odd miscelaneous message.
	while (currentClient && count > 0)
	{
		int requestedMessageLength = currentClient->queryQueMessageLength(freeSpace - headerLength); // NOT including header.
		int totalMessageLength = requestedMessageLength + headerLength;

		// in the rare case that a message is bigger than maximumBytes, use the alternate method.
		if (totalMessageLength + safetyZoneLength > maximumBytes)
		{
			if (safetyZoneLength >= freeSpace) // check there is a little room for the "MPAR" header.
				break;

			startMultiPartSend(currentClient, outStream, requestedMessageLength);
		}
		else
		{
			if (totalMessageLength + safetyZoneLength > freeSpace)
			{
				// not enough spare capacity atm? try again later.
				break;
			}

			currentClient->getQueMessage(outStream, requestedMessageLength);
		}

		currentClient->dirty_ = false;
		sentData = true;
		freeSpace -= totalMessageLength;
		currentClient = currentClient->next_;
		--count;
		++servicedCount;
		//			_RPT0(_CRT_WARN, "*");
	}
	//		while (count-- > 0)
	////		{
	//			_RPT0(_CRT_WARN, "-");
	//		}
	//		_RPT0(_CRT_WARN, "\n");

	// End of GUI frame? Clear clients ready for next frame.
	if (cumulativesampleFrames > sampleFramesPerCycle)
	{
		cumulativesampleFrames -= sampleFramesPerCycle;

		int size = 0;
		//			_RPT0(_CRT_WARN, "_________\n");
		// Remove all processed clients from list, leaving tail (if any).
		while (servicedCount > 0)
		{
			auto waiter = waitingClientsHead;

			// remove it.
			waitingClientsHead->inQue_ = false;
			waitingClientsHead = waitingClientsHead->next_;

			// if we processed them all, null the tail. List must be consistent before we try to add it back again below.
			if (waitingClientsHead == nullptr)
				waitingClientsTail = nullptr;

			// if it was updated after being serviced but before being removed, re-add it.
			if (waiter->dirty_)
			{
				AddWaiter(waiter);
			}

			++size;
			--servicedCount;
		}
		// count remainder.
		auto client = waitingClientsHead;
		while (client)
		{
			client = client->next_;
			++size;
		}

		if (currentClient == nullptr)
			currentClient = waitingClientsHead;

		// Calculate a running estimate of client service period to achieve speading them out evenly over successive process() calls.
		int new_sampleFramesPerClient = (std::max)(1, sampleFramesPerCycle / (std::max)(10, size));

		sampleFramesPerClient = (sampleFramesPerClient + new_sampleFramesPerClient) / 2;
		assert(sampleFramesPerClient > 0);
	}

	if (sentData)
	{
		que->Send();
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
	if (-99 == recievingHandle) // special case for multi-part messages
	{
		if (recievingMessageId == id_to_long("MPAR")) // multipart message start
		{
			int32_t totalMessageLength{};
			strm >> totalMessageLength;

			oversize_data.resize(totalMessageLength);
			multipartReadIndex = 0;
			multipartChunkIndex = 1;

			_RPTN(0, "FIFO recieving oversize message in chunks (%d bytes)\n", totalMessageLength);
		}
		else if (recievingMessageId == id_to_long("MPCK")) // multipart chunk
		{
			const auto payloadSize = recievingMessageLength;// -sizeof(int32_t);

			_RPTN(0, "FIFO recieving chunk %d (%d bytes)\n", multipartChunkIndex, payloadSize);

			if(payloadSize + multipartReadIndex > oversize_data.size())
			{
				_RPT0(0, "FIFO oversize read FAIL\n");

				// failed. ignore all further data.
				assert(false); // overflow

				oversize_data.clear();
				return;
			}

			strm.Read(oversize_data.data() + multipartReadIndex, payloadSize);

			multipartReadIndex += payloadSize;
			multipartChunkIndex++;

			if (multipartReadIndex >= oversize_data.size())
			{
				_RPT0(0, "FIFO oversize recieve DONE\n");

				int32_t handle{};
				int32_t msg_id{};
				int32_t msg_len{};

				memory_input_stream instream(oversize_data);

				instream >> handle;
				instream >> msg_id;
				instream >> msg_len;

				client->onQueMessageReady(handle, msg_id, instream);

				// end of multipart message.
				multipartChunkIndex = 0;
				multipartReadIndex = 0;
				oversize_data.clear();
			}
		}
		else
		{
			assert(false);
		}
		return;
	}
	else
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
