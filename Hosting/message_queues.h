#pragma once
/*
#include "Hosting/message_queues.h"

gmpi::hosting::gmpi_processor plugin;
*/

#include <atomic>
#include <cmath>
#include "Common.h"
//#include "GmpiApiAudio.h"
//#include "GmpiSdkCommon.h"

namespace gmpi
{
namespace hosting
{

inline int32_t id_to_long(const char* id)
{
	assert(id);
	const auto length = strlen(id);

	return
		  (length > 0 ? ((int)id[0]) << 24 : 0)
		| (length > 1 ? ((int)id[1]) << 16 : 0)
		| (length > 2 ? ((int)id[2]) <<  8 : 0)
		| (length > 3 ?       id[3]        : 0);
}

class IWriteableQue
{
public:
	virtual int freeSpace() = 0;
	virtual void pushString(int p_length, const unsigned char* p_data) = 0;
	virtual void Send() = 0;
};

class WriteableQueSink : public IWriteableQue
{
	virtual int freeSpace() override
	{
		return (std::numeric_limits<int>::max)();
	}
	virtual void pushString(int /*p_length*/, const unsigned char* /*p_data*/) override
	{
	}
	virtual void Send() override
	{
	}
};

// Base class common to various FIFO's and ques.
class InterThreadQueBase : public IWriteableQue
{
public:
	virtual void popString(int p_length, const void* p_data) = 0;
	virtual bool isEmpty() = 0;
	virtual int readyBytes() = 0;

#if 0 //defined( _DEBUG )
	// check that this is always being called from the same thread
	void DebugCheckThread(const char* p_msg_id)
	{
#if defined( _WIN32 )
		int current_thread = GetCurrentThreadId();

		if (m_debug_thread_id != -1 && m_debug_thread_id != current_thread)
		{
			_RPT2(_CRT_WARN, "lock_free_fifo accessed from thread %x, last accessed from thread %x\n", current_thread, m_debug_thread_id);
			//		_RPTW2(_CRT_WARN, L"  last msg '%s', this '%s'\n", long_to_id(debug_last_msg_id), p_msg_id;
			//		assert(m_debug_thread_id == GetCurrentThreadId());
		}

		m_debug_thread_id = current_thread;
		/*
			if( p_msg_id )
				debug_last_msg_id = id_to_long(p_msg_id);
			else
				debug_last_msg_id = 0;
		*/

#endif
	}
#endif
#if defined( _DEBUG )
	virtual bool isUncomitted() = 0;
	int m_debug_thread_id{};
#endif
};


class lock_free_fifo : public InterThreadQueBase
{
public:
	bool isFull()
	{
		// snapshot with acceptance of that this comparison operation is not atomic
		return read_ptr.load(std::memory_order_relaxed) == (m_uncommited_write_ptr % m_que_size) + 1;
	}

	bool isEmpty() override
	{
		// snapshot with acceptance of that this comparison operation is not atomic
		return read_ptr.load(std::memory_order_relaxed) == m_committed_write_ptr.load(std::memory_order_relaxed);
	}

	// Empty and reset the FIFO.
	virtual void Reset()
	{
		read_ptr = m_committed_write_ptr = m_uncommited_write_ptr = 0;
	}

#if defined( _DEBUG )
	bool isUncomitted() override
	{
		return m_committed_write_ptr.load(std::memory_order_relaxed) != m_uncommited_write_ptr;
	}
	int debugGetReadPtr()
	{
		return read_ptr.load(std::memory_order_relaxed);
	}
	int debugGetWritePtr()
	{
		return m_uncommited_write_ptr;
	}
#endif

	lock_free_fifo(int p_que_size) :
		m_que_size(p_que_size)
		, read_ptr(0)
		, m_committed_write_ptr(0)
	{
		m_que = new unsigned char[p_que_size];
	}

	~lock_free_fifo()
	{
		delete[] m_que;
	}

	// 'Sender' methods.
	// Push data onto the FIFO, but won't be transmitted until you call Send().
	void pushString(int p_length, const unsigned char* p_data) override
	{
#if defined( _DEBUG )
		// Attempt to send messages is bigger than buffer to DSP will hang
		// As interrupt never gets triggered.
		if (p_length > freeSpace())
		{
#if defined( _WIN32 )
			_RPT1(_CRT_WARN, "pushString BIGGER than BUFFER (POTENTIAL DEADLOCK) size=%d\n", m_que_size);
#endif
			assert(false && "too much data");
			return;
		}
#endif

		assert(p_length <= freeSpace());
		assert(m_uncommited_write_ptr < m_que_size);

		const unsigned char* source = p_data;
		unsigned char* dest = &(m_que[m_uncommited_write_ptr]);
		int todo = p_length;
		int contiguous = m_que_size - m_uncommited_write_ptr;

		if (todo > contiguous)
		{
			memcpy(dest, source, contiguous);
			todo -= contiguous;
			dest = &(m_que[0]);
			m_uncommited_write_ptr = 0;
			source += contiguous;
		}

		memcpy(dest, source, todo);

		m_uncommited_write_ptr = (m_uncommited_write_ptr + todo) % m_que_size;

		assert(m_uncommited_write_ptr != read_ptr);
	}

	// Make pending data visible to the receiver.
	void Send() override
	{
		m_committed_write_ptr.store(m_uncommited_write_ptr, std::memory_order_release);
	}

	void popString(int p_length, const void* p_data) override
	{
		auto current_head = read_ptr.load(std::memory_order_relaxed);

		assert(current_head != m_committed_write_ptr.load(std::memory_order_relaxed) || p_length == 0);
		assert(readyBytes() >= p_length);

		unsigned char* dest = (unsigned char*)p_data;
		unsigned char* source = &(m_que[current_head]);
		int todo = p_length;
		int contiguous = m_que_size - current_head;

		if (todo > contiguous)
		{
			memcpy(dest, source, contiguous);
			todo -= contiguous;
			source = &(m_que[0]);
			current_head = 0;
			dest += contiguous;
		}

		// memcpy, or just a direct loop, usually only small volumes. !!!!
		memcpy(dest, source, todo);
		read_ptr.store((current_head + todo) % m_que_size, std::memory_order_release);
	}

#if defined( _DEBUG )
	// peeks ahead without updating read pointer.
	void debugPopString(int& readPtr, int p_length, const void* p_data)
	{
		unsigned char* dest = (unsigned char*)p_data;
		unsigned char* source = &(m_que[readPtr]);
		int todo = p_length;
		int contiguous = m_que_size - readPtr;

		if (todo > contiguous)
		{
			memcpy(dest, source, contiguous);
			todo -= contiguous;
			source = &(m_que[0]);
			readPtr = 0;
			dest += contiguous;
		}

		// memcpy, or just a direct loop, usually only small volumes. !!!!
		memcpy(dest, source, todo);
		readPtr = (readPtr + todo) % m_que_size;
	}
#endif

	int freeSpace() override
	{
		auto freeSpace = read_ptr.load(std::memory_order_relaxed) - m_uncommited_write_ptr;

		// decide if wraped and adjust (using local variable, not volatiles).
		if (freeSpace <= 0)
		{
			freeSpace += m_que_size;
		}

		// que can't be absolutely full, else pointers wrap back to 'empty'. So report 1 less.
		return freeSpace - 1;
	}

	int totalSize() const
	{
		return m_que_size - 1;
	}

	// 'Receiver' methods.
	// Query how much data is available to read.
	int readyBytes() override
	{
		// access volatiles only once.
		int readyCount = m_committed_write_ptr.load(std::memory_order_relaxed) - read_ptr.load(std::memory_order_relaxed);

		// decide if wraped and adjust (using local variable, not volatiles).
		if (readyCount < 0)
		{
			readyCount += m_que_size;
		}

		return readyCount;
	}

	// Read data into annother FIFO.
	void Siphon(InterThreadQueBase* receiver, int totalSize)
	{
		// !!! could this split-up a message such that it ends up sliced with only the first half committed? (for a short time).
		assert(totalSize >= 0); // else throws off pointers.

		auto current_head = read_ptr.load(std::memory_order_relaxed);
		const int remain = totalSize - receiver->readyBytes();
		const int writepos = m_committed_write_ptr.load(std::memory_order_relaxed);	// copy it before comparing it.

		int contiguous;
		if (writepos >= current_head)
			contiguous = writepos - current_head;
		else
			contiguous = m_que_size - current_head;

		if (contiguous > remain)
			contiguous = remain;

		receiver->pushString(contiguous, &(m_que[current_head]));

		read_ptr.store((current_head + contiguous) % m_que_size, std::memory_order_release); // must update atomic in one go.
	}

	// Provide a pointer to contiguous data, let caller copy it (caller will then call 'advance()' to move read ptr)
	// for VST3
	int siphon2(void** dest) const
	{
		const int writepos = m_committed_write_ptr.load(std::memory_order_relaxed);	// copy it before comparing it.

		int contiguous;
		if (writepos >= read_ptr)
			contiguous = writepos - read_ptr;
		else
			contiguous = m_que_size - read_ptr;

		*dest = &(m_que[read_ptr]);
		return contiguous;
	}

	void siphon2_advance(int size)
	{
		auto current_head = read_ptr.load(std::memory_order_relaxed);
		read_ptr.store((current_head + size) % m_que_size, std::memory_order_release); // must update volatile in one go.
	}

private:
	int m_que_size;
	unsigned char* m_que = {};
	int m_uncommited_write_ptr = 0;
#ifdef _WIN32 // Apple don't support this yet
	alignas(std::hardware_destructive_interference_size) std::atomic<int> read_ptr;
	alignas(std::hardware_destructive_interference_size) std::atomic<int> m_committed_write_ptr;
#else
	[[maybe_unused]] char cachelinePad[64 - sizeof(std::atomic<int>)];
	std::atomic<int> read_ptr;
	[[maybe_unused]] char cachelinePad2[64 - sizeof(std::atomic<int>)];
	std::atomic<int> m_committed_write_ptr;
#endif
};

class my_input_stream
{
public:
	my_input_stream() {}
	virtual ~my_input_stream() {}
	virtual void Read(const void* lpBuf, unsigned int nMax) = 0;

	// POD types.
	template<typename T>
	my_input_stream& operator>>(T& val)
	{
		Read(&val, sizeof(val));
		return *this;
	}

	my_input_stream& operator>>(std::wstring& val)
	{
		int size = 0;
		Read(&size, sizeof(size));
		wchar_t* str = new wchar_t[size];
		Read(str, size * sizeof(wchar_t));
		val.assign(str, size);
		delete[] str;
		return *this;
	}

	my_input_stream& operator>>(std::string& val)
	{
		int32_t size = 0;
		Read(&size, sizeof(size));
		val.resize(size);
		Read(val.data(), size);
		return *this;
	}

	my_input_stream& operator>>(gmpi::Blob& val);
};

class my_output_stream
{
public:
	my_output_stream() = default;
	virtual ~my_output_stream() = default;
	virtual void Write(const void* lpBuf, unsigned int nMax) = 0;

	// POD types.
	template<typename T>
	my_output_stream& operator<<(T val)
	{
		Write(&val, sizeof(val));
		return *this;
	}

	my_output_stream& operator<<(const std::string& val);
	my_output_stream& operator<<(const std::wstring& val);
	my_output_stream& operator<<(const gmpi::Blob& val);
};

class my_msg_que_input_stream :
	public my_input_stream
{
public:

	my_msg_que_input_stream(InterThreadQueBase* p_que) :
		m_que(p_que)
	{
	};

	virtual void Read(const void* lpBuf, unsigned int nMax)
	{
		m_que->popString(nMax, lpBuf);
	}

private:
	InterThreadQueBase* m_que;
};

class my_msg_que_output_stream : public my_output_stream
{
public:
	my_msg_que_output_stream(IWriteableQue* p_que, int p_handle = 0, const char* p_msg_id = 0) : m_que(p_que)
	{
#if 0 //defined( _DEBUG )
		p_que->DebugCheckThread(p_msg_id);
#endif
		//	    assert( ! p_que->isUncomitted() ); // did last user of que flush it?  Are more than 1 thread acessing que?
		assert(p_handle != 0xcdcdcdcd);
		//_RPT1(_CRT_WARN, "p_msg_id=%s\n", p_msg_id );

		if (p_msg_id)
		{
			*this << p_handle;
			*this << id_to_long(p_msg_id);
		}
	}
	virtual void Write(const void* lpBuf, unsigned int nMax)
	{
		m_que->pushString(nMax, (unsigned char*)lpBuf);
	}
	void Send()
	{
		m_que->Send();
	}

private:
	IWriteableQue* m_que;
};

class QueClient
{
public:
	QueClient() = default;

	virtual int queryQueMessageLength(int availableBytes) = 0;
	virtual void getQueMessage(my_output_stream& outStream, int messageLength) = 0;

	QueClient* next_ = nullptr;
	bool inQue_ = false;
	bool dirty_ = false;
};

class interThreadQueUser
{
public:
	virtual bool onQueMessageReady(int handle, int msg_id, my_input_stream& p_stream) = 0;
};

class QueuedUsers
{
	int sampleFramesPerCycle = 500;

public:
	QueuedUsers() :
		waitingClientsHead(0)
		, waitingClientsTail(0)
	{
	}

	void AddWaiter(QueClient* client)
	{
		client->dirty_ = true; // flag it updated, even if it's already in que.

		if (client->inQue_)
		{
			// Sometimes DSP parameter changes twice (or more) between GUI updates.
			// Since it's already waiting on a prior update, or already procesed once this frame, nothing to do except flag it dirty.
			return;
		}

		//_RPT2(_CRT_WARN, "RequestQue: %s (%x)\n", typeid(*client).name(), client);
		if (waitingClientsTail)
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

	bool ServiceWaiters(my_output_stream& outStream, int freeSpace, int maximumBytes)
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

				// avoid overflow by leaving decent capacity, unless message is too big for an empty que anyhow.
				if (freeSpace < totalMessageLength + safetyZoneLength) // && fifo_.totalSize() > totalMessageLength )
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

	void setSampleRate(float s)
	{
		static const int guiFrameRate = 60;
		sampleFramesPerCycle = static_cast<int>(std::round(s / guiFrameRate));
	}

	bool ServiceWaitersIncremental(IWriteableQue* que, int sampleFrames) //, int sampleFramesPerCycle)
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

		if (sentData)
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
	virtual void pushString(int p_length, const unsigned char* p_data) override;
	virtual void popString(int p_length, const void* p_data) override;
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
	void ProcessMessage(interThreadQueUser* client, class my_msg_que_input_stream& strm);

	lock_free_fifo fifo_;
	bool partialMessageReceived;
	int recievingHandle;
	int recievingMessageId;
	int recievingMessageLength;
};

} // namespace hosting
} // namespace gmpi
