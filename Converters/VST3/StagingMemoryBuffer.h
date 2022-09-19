#pragma once
#include <vector>
#include "lock_free_fifo.h"

/*
#include "StagingMemoryBuffer.h"
*/

class StagingMemoryBuffer : public IWriteableQue
{
public:
	StagingMemoryBuffer(size_t bufferSize = 1000000) : writePos_( 0 )
	{
		storage_.assign(bufferSize, 0);
	}

	int writePos_;
	std::vector<char> storage_;

	int size(){ return writePos_; }
	inline char* data(){ return storage_.data(); }
	inline void clear(){ writePos_ = 0; }
	inline int freeSpace() override { return (int)(storage_.size() - writePos_); }
	inline int totalSize(){ return (int) storage_.size(); }
	inline bool empty(){ return writePos_ == 0; }

	// IWriteableQue
	void pushString(int p_length, const unsigned char* p_data) override
	{
		if( p_length <= totalSize() - writePos_ )
		{
			memcpy( data() + writePos_, p_data, p_length );
			writePos_ += p_length;
		}
		else
		{
			// Buffer full.
			assert( false );
		}
	}
	void Send() override
	{
		 // for compatibility only.
	}
};
