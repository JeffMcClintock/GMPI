#pragma once

#include <algorithm>
#include "lock_free_fifo.h"
#include "my_msg_que_output_stream.h"
#include "QueClient.h"

class interThreadQueUser
{
public:
	virtual bool onQueMessageReady( int handle, int msg_id, class my_input_stream& p_stream ) = 0;
};

class QueuedUsers
{
public:
	QueuedUsers() :
		waitingClientsHead(0)
		,waitingClientsTail(0)
	{
	}

	void AddWaiter( QueClient* client )
	{
		client->dirty_ = true; // flag it updated, even if it's already in que.

		if( client->inQue_ )
		{
			// Sometimes DSP parameter changes twice (or more) between GUI updates.
			// Since it's already waiting on a prior update, or already procesed once this frame, nothing to do except flag it dirty.
			return;
		}

		//_RPT2(_CRT_WARN, "RequestQue: %s (%x)\n", typeid(*client).name(), client);
		if( waitingClientsTail )
		{
			waitingClientsTail->next_ = client;
		}
		else
		{
			waitingClientsHead = client;
			currentClient = client;
		}

		waitingClientsTail = client;
		client->next_ = nullptr;
		client->inQue_ = true;
	}

	bool ServiceWaiters( my_output_stream& outStream, int freeSpace, int maximumBytes )
	{
		bool sentData = false;

		QueClient* client = waitingClientsHead;

		if( client )
		{
			QueClient* tail = waitingClientsTail;

			while( client )
			{
				const int headerLength = sizeof(int) * 3;
				const int safetyZoneLength = headerLength * 8; // not all que users check size first. Allow some slack for the odd miscelaneous message.
				int requestedMessageLength = client->queryQueMessageLength( maximumBytes - headerLength ); // NOT including header.
				int totalMessageLength = requestedMessageLength + headerLength;

				// avoid overflow by leaving decent capacity, unless message is too big for an empty que anyhow.
				if( freeSpace < totalMessageLength + safetyZoneLength ) // && fifo_.totalSize() > totalMessageLength )
				{
					break;
				}

				//_RPT2(_CRT_WARN, "ServiceWaiter: %s (%x)\n", typeid(*waitingClientsHead).name(), waitingClientsHead);
				waitingClientsHead = client->next_;

				if( waitingClientsTail == client )
				{
					waitingClientsTail = 0;
				}

				client->inQue_ = false;
				client->dirty_ = false;
				client->getQueMessage(outStream, requestedMessageLength );

				sentData = true;

				freeSpace -= totalMessageLength;

				// only go as far as original tail (else objects might que themselves over and over)
				if( client == tail )
					client = 0;
				else
					client = waitingClientsHead;
			}
		}
		return sentData;
	}
	
	bool ServiceWaitersIncremental(IWriteableQue* que, int sampleFrames, int sampleFramesPerCycle)
	{
		my_msg_que_output_stream outStream(que);
		auto freeSpace = que->freeSpace();

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

			// avoid overflow by leaving decent capacity, unless message is too big for an empty que anyhow.
			if (freeSpace < totalMessageLength + safetyZoneLength)
			{
				break;
			}

			currentClient->dirty_ = false;
			currentClient->getQueMessage(outStream, requestedMessageLength);

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

		if(sentData)
		{
			que->Send();
		}

		return sentData;
	}

	void Reset()
	{
		/* currently, Processor is deleted before here ( see SynthRuntime::initialize() ). So don't try to access deleted objects.

		auto client = waitingClientsHead;
		while (client)
		{
			client->inQue_ = false;
			client = client->next_;
		}
		*/

		currentClient = waitingClientsTail = waitingClientsHead = nullptr;
		servicedCount = cumulativesampleFrames = 0;
	}

private:
	class QueClient* waitingClientsHead;
	QueClient* waitingClientsTail;
	QueClient* currentClient = nullptr;

	// Load-ballancing.
	int cumulativesampleFrames = 0;
	int sampleFramesPerClient = 100; // Adaptive estimate for load-balancing.
	int sfAccumulator = 0;
	int servicedCount = 0;
};

class interThreadQue : public InterThreadQueBase
{
	friend class ui_que_monitor;
public:
	interThreadQue(int p_que_size);

	// InterThreadQueBase overrides.
	virtual void pushString( int p_length, const unsigned char* p_data ) override;
	virtual void popString( int p_length, const void* p_data ) override;
	virtual bool isEmpty() override;
	virtual int readyBytes() override;
	int freeSpace() override
	{
		return fifo_.freeSpace();
	}
	int maxMessageSize()
	{
		return fifo_.totalSize();
	}
	void pollMessage(interThreadQueUser* app);
#if defined( _DEBUG )
	virtual bool isUncomitted() override;
	bool isFromGui;
#endif

	virtual void Send() override;
	virtual void Reset();

private:
	void ProcessMessage( interThreadQueUser* client, class my_msg_que_input_stream& strm );

	lock_free_fifo fifo_;
	bool partialMessageReceived;
	int recievingHandle;
	int recievingMessageId;
	int recievingMessageLength;
};

