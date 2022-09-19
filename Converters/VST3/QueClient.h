#pragma once

class QueClient

{
public:
	QueClient(){}

	virtual int queryQueMessageLength( int availableBytes ) = 0;
	virtual void getQueMessage( class my_output_stream& outStream, int messageLength ) = 0;

	QueClient* next_ = nullptr;
	bool inQue_ = false;
	bool dirty_ = false;
};