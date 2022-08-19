#pragma once

/*
  GMPI - Generalized Music Plugin Interface specification.
  Copyright 2007-2022 Jeff McClintock.

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "GmpiApiCommon.h"

// Helper for comparing GUIDs
inline bool operator==(const gmpi2::Guid& left, const gmpi2::Guid& right)
{
	return 0 == std::memcmp(&left, &right, sizeof(left));
}

namespace gmpi2_sdk
{
// Helper for managing lifetime of reference-counted interface pointer
template<class wrappedObjT>
class shared_ptr
{
	wrappedObjT* obj = {};

public:
	shared_ptr(){}

	explicit shared_ptr(wrappedObjT* newobj) : obj(0)
	{
		Assign(newobj);
	}
	shared_ptr(const shared_ptr<wrappedObjT>& value) : obj(0)
	{
		Assign(value.obj);
	}
	// Attach object without incrementing ref count. For objects created with new.
	void Attach(wrappedObjT* newobj)
	{
		wrappedObjT* old = obj;
		obj = newobj;

		if( old )
		{
			old->release();
		}
	}

	~shared_ptr()
	{
		if( obj )
		{
			obj->release();
		}
	}
	operator wrappedObjT*( )
	{
		return obj;
	}
	const wrappedObjT* operator=( wrappedObjT* value )
	{
		Assign(value);
		return value;
	}
	shared_ptr<wrappedObjT>& operator=( shared_ptr<wrappedObjT>& value )
	{
		Assign(value.get());
		return *this;
	}
	bool operator==( const wrappedObjT* other ) const
	{
		return obj == other;
	}
	bool operator==( const shared_ptr<wrappedObjT>& other ) const
	{
		return obj == other.obj;
	}
	wrappedObjT* operator->( ) const
	{
		return obj;
	}

	wrappedObjT*& get()
	{
		return obj;
	}

	wrappedObjT** getAddressOf()
	{
		assert(obj == nullptr); // Free it before you re-use it!
		return &obj;
	}
	void** asIMpUnknownPtr()
	{
		assert(obj == 0); // Free it before you re-use it!
		return reinterpret_cast<void**>(&obj);
	}

	template<typename I>
	shared_ptr<I> As()
	{
		shared_ptr<I> returnInterface;
		if (obj)
		{
			obj->queryInterface(&I::guid, returnInterface.asIMpUnknownPtr());
		}
		return returnInterface;
	}

	bool isNull()
	{
		return obj == nullptr;
	}

private:
	// Attach object and increment ref count.
	void Assign(wrappedObjT* newobj)
	{
		Attach(newobj);
		if( newobj )
		{
			newobj->addRef();
		}
	}
};
} // namespace gmpi2_sdk
