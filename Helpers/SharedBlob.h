#pragma once

// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.

#include <cstdint>
#include "GmpiApiCommon.h"
#include "RefCountMacros.h"

/*
#include "Helpers/SharedBlob.h"

A lightweight, reference-counted view onto a block of memory, implementing
gmpi::api::ISharedBlob. Send it out a pin of the 'Object' datatype.

The view does NOT own the bytes it points at - the caller keeps the storage
alive (typically in a small pool of these views) for as long as the blob is
still referenced by a downstream module. Use inUse() to test whether a view
can be recycled before pointing it at fresh data.

Reference counting is pool-style: release() does NOT delete the object at zero,
because the view's lifetime is owned by the pool, not the reference count.
*/

namespace gmpi
{

class SharedBlobView : public api::ISharedBlob
{
	const uint8_t* data_ = nullptr;
	int64_t size_ = 0;

public:
	SharedBlobView() = default;
	SharedBlobView(const uint8_t* data, int64_t size) : data_(data), size_(size) {}

	ReturnCode read(const uint8_t** returnData, int64_t* returnSize) override
	{
		*returnData = data_;
		*returnSize = size_;
		return ReturnCode::Ok;
	}

	// Point the view at new data. Refused while the blob is still referenced downstream.
	ReturnCode set(const uint8_t* data, int64_t size)
	{
		if (inUse())
			return ReturnCode::Fail; // can't modify a blob that's still 'in flight'.

		data_ = data;
		size_ = size;
		return ReturnCode::Ok;
	}
	ReturnCode set(const char* data, int64_t size)
	{
		return set(reinterpret_cast<const uint8_t*>(data), size);
	}

	// true while a downstream module still holds a reference (and the storage must not be reused).
	bool inUse() const { return refCount2_ > 1; }

	GMPI_QUERYINTERFACE_METHOD(api::ISharedBlob);

	// Reference counted, but pool-managed: does NOT delete itself at refcount 0.
	int32_t refCount2_ = 1;
	int32_t addRef() override { return ++refCount2_; }
	int32_t release() override { return --refCount2_; }
};

} // namespace gmpi
