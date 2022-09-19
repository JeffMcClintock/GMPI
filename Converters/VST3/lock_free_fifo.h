#pragma once
#include <string.h>
#include <assert.h>
#include <atomic>
#include <new>

#pragma warning(disable:4324) // structure was padded due to __declspec(align())

class IWriteableQue
{
public:
	virtual int freeSpace() = 0;
	virtual void pushString( int p_length, const unsigned char* p_data ) = 0;
	virtual void Send() = 0;
};

class WriteableQueSink : public IWriteableQue
{
    virtual int freeSpace() override
    {
        return (std::numeric_limits<int>::max)();
    }
    virtual void pushString( int /*p_length*/, const unsigned char* /*p_data*/ ) override
    {}
    virtual void Send() override
    {}
};

// Base class common to various FIFO's and ques.
class InterThreadQueBase : public IWriteableQue
{
public:
	virtual void popString( int p_length, const void* p_data ) = 0;
	virtual bool isEmpty() = 0;
	virtual int readyBytes() = 0;

	#if 0 //defined( _DEBUG )
	// check that this is always being called from the same thread
	void DebugCheckThread(const char* p_msg_id)
	{
	#if defined( _WIN32 )
		int current_thread = GetCurrentThreadId();

		if( m_debug_thread_id != -1 && m_debug_thread_id != current_thread )
		{
			_RPT2(_CRT_WARN, "lock_free_fifo accessed from thread %x, last accessed from thread %x\n", current_thread,m_debug_thread_id );
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
	int m_debug_thread_id;
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
		,read_ptr(0)
		,m_committed_write_ptr(0)
	{
		m_que = new unsigned char[p_que_size];
	}

	~lock_free_fifo()
	{
		delete [] m_que;
	}

	// 'Sender' methods.
	// Push data onto the FIFO, but won't be transmitted until you call Send().
	void pushString( int p_length, const unsigned char* p_data ) override
	{
		#if defined( _DEBUG )
			// Attempt to send messages is bigger than buffer to DSP will hang
			// As interrupt never gets triggered.
			if( p_length > freeSpace() )
			{
				#if defined( _WIN32 )
					_RPT1(_CRT_WARN, "pushString BIGGER than BUFFER (POTENTIAL DEADLOCK) size=%d\n", m_que_size );
				#endif
				assert( false && "too much data" );
				return;
			}
		#endif

		assert( p_length <= freeSpace() );
		assert( m_uncommited_write_ptr < m_que_size );

		const unsigned char* source = p_data;
		unsigned char* dest = &( m_que[m_uncommited_write_ptr] );
		int todo = p_length;
		int contiguous = m_que_size - m_uncommited_write_ptr;

		if( todo > contiguous )
		{
			memcpy( dest, source, contiguous );
			todo -= contiguous;
			dest = & (m_que[0]);
			m_uncommited_write_ptr = 0;
			source += contiguous;
		}

		memcpy( dest, source, todo );

		m_uncommited_write_ptr = (m_uncommited_write_ptr + todo) % m_que_size;

		assert( m_uncommited_write_ptr != read_ptr );
	}

	// Make pending data visible to the receiver.
	void Send() override
	{
		m_committed_write_ptr.store(m_uncommited_write_ptr, std::memory_order_release);
	}

	void popString( int p_length, const void* p_data ) override
	{
		auto current_head = read_ptr.load(std::memory_order_relaxed);

		assert( current_head != m_committed_write_ptr.load(std::memory_order_relaxed) || p_length == 0 );
		assert( readyBytes() >= p_length );

		unsigned char* dest = (unsigned char*) p_data;
		unsigned char* source = &(m_que[current_head]);
		int todo = p_length;
		int contiguous = m_que_size - current_head;

		if( todo > contiguous )
		{
			memcpy( dest, source, contiguous );
			todo -= contiguous;
			source = &(m_que[0]);
			current_head = 0;
			dest += contiguous;
		}

		// memcpy, or just a direct loop, usually only small volumes. !!!!
		memcpy( dest, source, todo );
		read_ptr.store((current_head + todo) % m_que_size, std::memory_order_release);
	}

	#if defined( _DEBUG )
	// peeks ahead without updating read pointer.
	void debugPopString( int& readPtr, int p_length, const void* p_data )
	{
		unsigned char* dest = (unsigned char*) p_data;
		unsigned char* source = &(m_que[readPtr]);
		int todo = p_length;
		int contiguous = m_que_size - readPtr;

		if( todo > contiguous )
		{
			memcpy( dest, source, contiguous );
			todo -= contiguous;
			source = & (m_que[0]);
			readPtr = 0;
			dest += contiguous;
		}

		// memcpy, or just a direct loop, usually only small volumes. !!!!
		memcpy( dest, source, todo );
		readPtr = (readPtr + todo) % m_que_size;
	}
	#endif

	int freeSpace() override
	{
		auto freeSpace = read_ptr.load(std::memory_order_relaxed) - m_uncommited_write_ptr;

		// decide if wraped and adjust (using local variable, not volatiles).
		if( freeSpace <= 0 )
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
		if( readyCount < 0 )
		{
			readyCount += m_que_size;
		}

		return readyCount;
	}

	// Read data into annother FIFO.
	void Siphon( InterThreadQueBase* receiver, int totalSize )
	{
		// !!! could this split-up a message such that it ends up sliced with only the first half committed? (for a short time).
		assert( totalSize >= 0 ); // else throws off pointers.

		auto current_head = read_ptr.load(std::memory_order_relaxed);
		const int remain = totalSize - receiver->readyBytes();
		const int writepos = m_committed_write_ptr.load(std::memory_order_relaxed);	// copy it before comparing it.

		int contiguous;
		if( writepos >= current_head )
			contiguous = writepos - current_head;
		else
			contiguous = m_que_size - current_head;

		if( contiguous > remain )
			contiguous = remain;

		receiver->pushString( contiguous, &(m_que[current_head]) );

		read_ptr.store((current_head + contiguous ) % m_que_size, std::memory_order_release); // must update atomic in one go.
	}

	// Provide a pointer to contiguous data, let caller copy it (caller will then call 'advance()' to move read ptr)
	// for VST3
	int siphon2(void** dest) const
	{
		const int writepos = m_committed_write_ptr.load(std::memory_order_relaxed);	// copy it before comparing it.

		int contiguous;
		if( writepos >= read_ptr )
			contiguous = writepos - read_ptr;
		else
			contiguous = m_que_size - read_ptr;

		*dest = &(m_que[read_ptr]);
		return contiguous;
	}

	void siphon2_advance(int size)
	{
		auto current_head = read_ptr.load(std::memory_order_relaxed);
		read_ptr.store((current_head + size ) % m_que_size, std::memory_order_release); // must update volatile in one go.
	}

private:
	int m_que_size;
	unsigned char* m_que = {};
	int m_uncommited_write_ptr = 0;
#ifdef _WIN32 // Apple don't support this yet
	alignas(std::hardware_destructive_interference_size) std::atomic<int> read_ptr;
	alignas(std::hardware_destructive_interference_size) std::atomic<int> m_committed_write_ptr;
#else
    [[maybe_unused]]char cachelinePad[64 - sizeof(std::atomic<int>)];
    std::atomic<int> read_ptr;
    [[maybe_unused]]char cachelinePad2[64 - sizeof(std::atomic<int>)];
    std::atomic<int> m_committed_write_ptr;
#endif
};

