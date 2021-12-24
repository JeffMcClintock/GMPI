/* Copyright (c) 2007-2021 SynthEdit Ltd
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name SEM, nor SynthEdit, nor 'Music Plugin Interface' nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY SynthEdit Ltd ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL SynthEdit Ltd BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define	GMPI_SDK_REVISION 9000 // 0.90
/* Version History
* 	 12/25/2021 - V0.90 : Draft release

*/

#include <memory.h>
#include <sstream>
#include <map>
#include "mp_sdk_common.h"

using namespace gmpi;

// Only standalone plugins need entrypoint.
#if defined( SE_EDIT_SUPPORT ) || defined( SE_TARGET_PLUGIN )
#define COMPILE_HOST_SUPPORT
#endif

#ifndef COMPILE_HOST_SUPPORT

//---------------FACTORY --------------------

// MpFactory - a singleton object.  The plugin registers it's ID with the factory.

class MpFactory: public IMpPluginFactory
{
public:
	MpFactory( void ){}
	virtual ~MpFactory( void ){}

	// IMpPluginFactory methods
	int32_t createInstance2(
		const char* uniqueId,
		int32_t subType,
		void** returnInterface ) override;

	int32_t RegisterPlugin( const char* uniqueId, int32_t subType, MP_CreateFunc2 create );

	// IMpUnknown methods
	GMPI_QUERYINTERFACE1(MP_IID_PLUGIN_FACTORY, IMpPluginFactory);
	GMPI_REFCOUNT_NO_DELETE
private:
	// a map of all registered IIDs.
	std::map< std::pair<int32_t, std::string>, MP_CreateFunc2> m_pluginMap;
};


MpFactory* Factory()
{
	static MpFactory theFactory;
	return &theFactory;
}

#if !defined(_M_X64) && defined(_MSC_VER)
	// Export additional symbol without name decoration.
	#pragma comment( linker, "/export:MP_GetFactory=_MP_GetFactory@4" )
#endif

// This is the DLL's main entry point.  It returns the factory.
extern "C"

#ifdef _WIN32
	#define GMPI_EXPORT __declspec (dllexport)
#else

#if defined (__GNUC__)
	#define GMPI_EXPORT	__attribute__ ((visibility ("default")))
#else
	#define GMPI_EXPORT
#endif

#endif


GMPI_EXPORT
int32_t MP_GetFactory( void** returnInterface )
{
	// call queryInterface() to keep refcounting in sync
	return Factory()->queryInterface( MP_IID_UNKNOWN, returnInterface );
}

namespace gmpi
{
	// register a DSP plugin with the factory
	int32_t RegisterPlugin(int subType, const char* uniqueId, MP_CreateFunc2 create)
	{
		return Factory()->RegisterPlugin(uniqueId, subType, create);
	}
}

// Factory methods
int32_t MpFactory::RegisterPlugin( const char* uniqueId, int32_t subType, MP_CreateFunc2 create )
{
	// already registered this plugin?
	assert( m_pluginMap.find({ subType, uniqueId }) == m_pluginMap.end());

	m_pluginMap[{ subType, uniqueId }] = create;

	return gmpi::MP_OK;
}

int32_t MpFactory::createInstance2( const char* uniqueId, int32_t subType,
	void** returnInterface )
{
	*returnInterface = 0; // if we fail for any reason, default return-val to NULL

	/* search m_pluginMap for the requested IID */
	auto it = m_pluginMap.find({ subType, uniqueId });
	if( it == m_pluginMap.end( ) )
	{
		return MP_NOSUPPORT;
	}

	auto create = (*it).second;

	if( create == 0 )
	{
		return MP_NOSUPPORT;
	}

	try
	{
		auto m = create();
		*returnInterface = m;
#ifdef _DEBUG
		{
			m->addRef();
			int refcount = m->release();
			assert(refcount == 1);
		}
#endif
	}

	// the new function will throw a std::bad_alloc exception if the memory allocation fails.
	// the constructor will throw a char* if host don't support required interfaces.
	catch( ... )
	{
		return MP_FAIL;
	}

	return gmpi::MP_OK;
}

#endif	// COMPILE_HOST_SUPPORT

// BLOB datatype

MpBlob::MpBlob() :
	size_(0),
	data_(0)
{
}

void MpBlob::setValueRaw(size_t size, const void* data)
{
	if (size_ != size)
	{
		delete[] data_;
		size_ = size;
		data_ = new char[size_];
	}

	if (size_ > 0)
	{
		memcpy(data_, data, size_);
	}
}

void MpBlob::resize( int size )
{
	if( size_ < (size_t) size )
	{
		delete [] data_;
		if( size > 0 )
		{
			data_ = new char[size];
		}
		else
		{
			data_ = 0;
		}
	}

	size_ = size;
}

int32_t MpBlob::getSize() const
{
	return (int32_t) size_;
}

char* MpBlob::getData() const
{
	return data_;
}

const MpBlob& MpBlob::operator=( const MpBlob &other )
{
	setValueRaw( other.size_, other.data_ );
	return other;
}

bool MpBlob::operator==( const MpBlob& other ) const
{
	if( size_ != other.size_ )
		return false;

	for( size_t i = 0 ; i < size_ ; ++i )
	{
		if( data_[i] != other.data_[i] )
			return false;
	}
	return true;
}

bool MpBlob::compare( char* data, int size )
{
	if( size_ != size )
		return false;

	for( size_t i = 0 ; i < size_ ; ++i )
	{
		if( data_[i] != data[i] )
			return false;
	}
	return true;
}

MpBlob::~MpBlob()
{
	delete [] data_;
}

bool MpBlob::operator!=( const MpBlob& other )
{
	return !operator==(other);
}
